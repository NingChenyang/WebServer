#include "Connection.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"
// #pragma execution_character_set("utf-8")
Connection::Connection(EventLoop *loop, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
	: loop_(loop), state_(StateE::kConnecting), socket_(std::make_unique<Socket>(sockfd)), channel_(std::make_unique<Channel>(sockfd, loop_)), localAddr_(localAddr), peerAddr_(peerAddr), context_() // 初始化context_
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
}

Connection::~Connection()
{
	// 在析构时我们只需要清理资源,不需要触发回调
	if (state_ == StateE::kConnected || state_ == StateE::kDisconnecting)
	{
		SetState(StateE::kDisconnected);
		channel_->DisableAll();
		channel_->Remove();
	}
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
	message->RetrieveAll(); // 还没发送
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
			// 及时更新
			//  output_buffer_.Retrieve(nwrote);
			//  如果全部数据已发送完成，触发回调
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
					HandleError();
				}
				return;
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
	auto tie = shared_from_this();
	if (tie == nullptr)
	{
		std::cout << "tie_过期了，说明这个Channel已经不需要了，直接返回" << std::endl;
		return;
	}
	else
	{
		channel_->Tie(tie);
	}
	channel_->EnableReading();
	last_time_ = TimeStamp::Now();
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
	loop_->RemoveLoopConn(fd());
}

bool Connection::Timeout(time_t now, int val)
{
	return now - last_time_.TimeToInt() > val;
}

void Connection::Shutdown()
{

	if (state_ == StateE::kConnected)
	{
		SetState(StateE::kDisconnecting);
		loop_->AddLoopQueue([this]()
							{ ShutdownInLoop(); });
	}
}

void Connection::ForceClose()
{
	if (state_ == StateE::kConnected || state_ == StateE::kDisconnecting)
	{
		SetState(StateE::kDisconnecting);
		if (loop_->IsInLoop())
		{
			ForceCloseInLoop();
		}
		else
		{
			loop_->RunInLoop([this]()
							 { ForceCloseInLoop(); });
		}
	}
}

void Connection::HandleRead()
{
	int savedErrno = 0;
	ssize_t n = input_buffer_.ReadFd(fd(), &savedErrno);

	if (n > 0)
	{
		last_time_ = TimeStamp::Now();

		if (messageCallback_)
		{
			messageCallback_(shared_from_this(), &input_buffer_);
		}
		// 在消息处理完后清空buffer
		input_buffer_.RetrieveAll();
	}
	else if (n == 0)
	{
		HandleClose();
	}
	else
	{
		LOG_ERROR << "ReadFd error: " << strerror(savedErrno);
		HandleError();
	}
}

void Connection::HandleWrite()
{
	if (!channel_->IsWrite())
	{
		return;
	}

	ssize_t n = ::write(fd(), output_buffer_.Peek(), output_buffer_.ReadableBytes());
	if (n > 0)
	{
		last_time_ = TimeStamp::Now();
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
				Shutdown();
			}
		}
	}
	else
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			LOG_ERROR << "HandleWrite error: " << strerror(errno);
			if (errno == EPIPE || errno == ECONNRESET)
			{
				HandleError();
			}
		}
	}
}

void Connection::HandleClose()
{
	// HandleClose只能在Connection对象还活着的时候调用
	// 即只能通过shared_ptr调用,不能在析构函数中调用

	SetState(StateE::kDisconnected);
	channel_->Remove();

	// 获取shared_from_this必须在调用HandleClose时对象仍然存活
	ConnectionPtr guardThis(shared_from_this());

	// 按顺序通知关闭连接
	if (closedCallback_)
	{
		closedCallback_(guardThis);
	}

	// Remove from loop must be done after callbacks
	loop_->RunInLoop([this]()
					 { loop_->RemoveLoopConn(fd()); });

	if (RemoveLoopConnCallback_)
	{
		RemoveLoopConnCallback_(guardThis);
	}
}

void Connection::HandleError()
{
	int err = sockets::GetSocketError(fd());
	int cur_errno = errno; // 保存当前的 errno

	std::string errorMsg;
	if (err != 0)
	{
		errorMsg = strerror(err);
	}
	else if (cur_errno != 0)
	{
		errorMsg = strerror(cur_errno);
	}
	else
	{
		errorMsg = "Connection reset by peer or timeout";
	}

	LOG_ERROR << "Connection::HandleError [" << peerAddr_.ToIpPort()
			  << "] - errno = " << cur_errno
			  << ", SO_ERROR = " << err
			  << " (" << errorMsg << ")";

	// 发生错误时强制关闭连接
	ForceClose();
}

// 新增函数，用于在IO线程中安全关闭连接
void Connection::ShutdownInLoop()
{
	// 确保在IO线程中调用
	assert(loop_->IsInLoop());
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

void Connection::ForceCloseInLoop()
{
	// 确保在IO线程中调用
	assert(loop_->IsInLoop());
	if (state_ == StateE::kConnected || state_ == StateE::kDisconnecting)
	{
		// 关闭连接
		HandleClose();
		// SetState(StateE::kDisconnected);
	}
}

void Connection::SetContext(const std::any &context)
{
	context_ = context;
}

const std::any &Connection::GetContext() const
{
	return context_;
}

std::any &Connection::GetMutableContext()
{
	return context_;
}