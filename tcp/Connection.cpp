#include "Connection.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"
// #pragma execution_character_set("utf-8")
Connection::Connection(EventLoop *loop, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
	: loop_(loop), state_(StateE::kConnecting), socket_(std::make_unique<Socket>(sockfd)), channel_(std::make_unique<Channel>(sockfd, loop_)), localAddr_(localAddr), peerAddr_(peerAddr)
{
	// 设置好 读，写，关闭连接 的回调函数
	channel_->SetReadCallback([this]()
							  { HandleRead(); });
	channel_->SetWriteCallback([this]()
							   { HandleWrite(); });
	channel_->SetCloseCallback([this]()
							   { HandleClose(); });
	channel_->SetErrorCallback([this]()
							   { HandleError(); });
	channel_->SetET();

	channel_->EnableReading();
}

Connection::~Connection()
{
	// std::cout << "connection 析构" << std::endl;
}

void Connection::SetOnMessageCallback(const OnMessageCallback &cb)
{
	messageCallback_ = cb;
}

void Connection::SetOnClosedCallback(const OnClosedCallback &cb)
{
	closedCallback_ = cb;
}

void Connection::SetOnSentCallback(const OnSentCallback &cb)
{
	sentCallback_ = cb;
}

void Connection::SetOnConnectedCallback(const OnConnectedCallback &cb)
{
	connectedCallback_ = cb;
}

void Connection::Send(Buffer *message)
{
	Send(message->Peek(), message->ReadableBytes());
	message->RetrieveAll();
}

void Connection::Send(const void *message, size_t len)
{
	if (state_ == StateE::kDisconnected)
		return;

	if (loop_->IsInLoop())
	{
		SendInLoop(message, len);
	}
	else
	{
		auto data_copy = std::make_shared<std::string>(static_cast<const char *>(message), len);
		loop_->AddLoopQueue([this, data_copy]()
							{ SendInLoop(data_copy->data(), data_copy->size()); });
	}
}

void Connection::Send(const std::string &message)
{
	Send(message.data(), message.size());
}

void Connection::SendInLoop(const void *message, size_t len)
{
	if (state_ == StateE::kDisconnected)
		return;

	size_t remaining = len;
	ssize_t nwrote = 0;
	bool fault_error = false;

	// 如果channel当前没有关注写事件，并且发送缓冲区为空，尝试直接发送数据
	if (!channel_->IsWrite() && output_buffer_.ReadableBytes() == 0)
	{
		nwrote = ::write(fd(), message, len);
		if (nwrote >= 0)
		{
			remaining = len - nwrote;
			// 如果全部数据已发送完成，触发回调
			if (remaining == 0 && sentCallback_)
			{
				sentCallback_(shared_from_this());
			}
		}
		else
		{
			nwrote = 0;
			if (errno != EWOULDBLOCK && errno != EAGAIN)
			{
				if (errno == EPIPE || errno == ECONNRESET)
				{
					fault_error = true;
				}
			}
		}
	}

	// 如果还有未发送的数据，添加到发送缓冲区
	if (!fault_error && remaining > 0)
	{
		output_buffer_.Append(static_cast<const char *>(message) + nwrote, remaining);
		if (!channel_->IsWrite())
		{
			channel_->EnableWriting(); // 打开写事件关注
		}
	}
}

void Connection::ConnectEstablished()
{
	assert(state_ == StateE::kConnecting);
	SetState(StateE::kConnected);
	channel_->Tie(shared_from_this());
	channel_->EnableReading();
	// 连接成功后业务处理
	// std::cout << "成功连接" << std::endl;
	connectedCallback_(shared_from_this());
}

void Connection::ConnectDestroyed()
{
	if (state_ == StateE::kConnected)
	{
		SetState(StateE::kDisconnected);
		channel_->DisableAll();
		// 调用用户设置的连接成功或断开的回调函数
		// std::cout << "断开连接" << std::endl;
		closedCallback_(shared_from_this());
	}
	channel_->Remove();
}

bool Connection::Timeout(time_t now, int val)
{
	return now - last_time_.TimeToInt() > val;
}

void Connection::ShutDown()
{
	if (state_ == StateE::kConnected)
	{
		SetState(StateE::kDisconnecting);
		if (!channel_->IsWrite())
		{
			shutdown(fd(), SHUT_WR);
		}
	}
}

void Connection::ForceClose()
{
	if (state_ == StateE::kConnected || state_ == StateE::kDisconnecting)
	{
		SetState(StateE::kDisconnecting);
		HandleClose();
	}
}

void Connection::HandleRead()
{
	int savedErrno = 0;
	auto n = input_buffer_.ReadFd(fd(), &savedErrno);
	std::string msg = input_buffer_.ReadBufferAll();
	if (n > 0)
	{
		last_time_ = TimeStamp::Now();
		messageCallback_(shared_from_this(), msg); // 这个是用户设置好的函数
	}
	else if (n == 0)
	{
		// 表示客户端关闭了连接
		HandleClose();
	}
	else
	{
		// 表示出错了，现在不处理
		printf("readFd error\n");
	}
}

void Connection::HandleWrite()
{
	if (!channel_->IsWrite())
	{
		return;
	}

	size_t n = ::write(fd(), output_buffer_.Peek(), output_buffer_.ReadableBytes());
	if (n > 0)
	{
		output_buffer_.Retrieve(n);
		// 如果发送缓冲区清空，关闭写事件并触发回调
		if (output_buffer_.ReadableBytes() == 0)
		{
			channel_->DisableWriting();
			if (sentCallback_)
			{
				sentCallback_(shared_from_this());
			}

			// 如果正在关闭连接，发送完最后的数据后关闭写端
			if (state_ == StateE::kDisconnecting)
			{
				ShutdownInLoop();
			}
		}
	}
	else
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			HandleError();
		}
	}
}

void Connection::HandleClose()
{
	assert(state_ == StateE::kConnected || state_ == StateE::kDisconnecting);
	SetState(StateE::kDisconnected);
	channel_->DisableAll();
	ConnectionPtr guardThis(shared_from_this());
	// printf("Connection::handleClose() guardThis(shared_from_this())后 user_count= %ld\n", guardThis.use_count());
	closeCallback_(guardThis);
	loop_->RemoveLoopConn(fd());
	// closeCallback_就是Server::removeConnection()函数
}

void Connection::HandleError()
{
}

// 新增函数，用于在IO线程中安全关闭连接
void Connection::ShutdownInLoop()
{
	if (state_ == StateE::kDisconnected || state_ == StateE::kDisconnecting)
	{
		return;
	}

	SetState(StateE::kDisconnecting);

	// 只有当没有待写数据时才关闭写端
	if (!channel_->IsWrite() && output_buffer_.ReadableBytes() == 0)
	{
		if (::shutdown(fd(), SHUT_WR) < 0)
		{
			if (errno != ENOTCONN && errno != EINVAL && errno != EPIPE)
			{
				HandleError();
			}
		}
	}
}