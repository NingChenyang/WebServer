#include "Server.h"
void SetNonblock(int sockfd)
{
	int flag = fcntl(sockfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(sockfd, F_SETFL, flag);
}

Server::Server(InetAddress &serv_addr, int thread_nums)
	: main_loop_(new EventLoop(true, 600000, 100000)), thread_nums_(thread_nums), io_pool_(new ThreadPool(thread_nums_, "IO")), acceptor_(std::make_unique<Acceptor>(serv_addr, main_loop_))
{
	auto cb = [this](int sockfd, const InetAddress &peerAddr)
	{ HandleNewConntion(sockfd, peerAddr); };
	acceptor_->SetNewConntionCallback(cb);
	for (int i = 0; i < thread_nums_; i++)
	{
		sub_loop_.emplace_back(new EventLoop(false, 600000, 100000));
		sub_loop_[i]->SetRemoveLoopConnCallback(std::bind(&Server::HandleRemoveConntion, this, std::placeholders::_1));

		io_pool_->AddTask(std::bind(&EventLoop::Run, sub_loop_[i].get()));
	}
}

Server::~Server()
{
	delete io_pool_;
}

void Server::Run()
{
	acceptor_->Listen();
	main_loop_->Run();
}

void Server::Stop()
{
	// 停止主事件循环
	main_loop_->Stop();
	std::cout << "停止主事件循环" << std::endl;
	// 停止从事件循环
	for (int i = 0; i < thread_nums_; i++)
	{
		sub_loop_[i]->Stop();
	}
	std::cout << "停止从事件循环" << std::endl;

	// 停止io线程
	io_pool_->Stop();
	std::cout << "停止io线程" << std::endl;
}

void Server::SetOnMessageCallback(const OnMessageCallback &cb)
{
	messageCallback_ = cb;
}

void Server::SetOnClosedCallback(const OnClosedCallback &cb)
{
	closedCallback_ = cb;
}

void Server::SetOnSentCallback(const OnSentCallback &cb)
{
	sentCallback_ = cb;
}

void Server::SetOnConnectedCallback(const OnConnectedCallback &cb)
{
	connectedCallback_ = cb;
}

void Server::HandleNewConntion(int cfd, const InetAddress &peerAddr)
{
	SetNonblock(cfd);
	InetAddress localAddr(sockets::GetLocalAddr(cfd));

	auto conn = std::make_shared<Connection>(sub_loop_[cfd % thread_nums_].get(), cfd, localAddr, peerAddr);
	// connectedCallback_(conn);
	sub_loop_[cfd % thread_nums_]->NewLoopConntion(conn);
	{
		std::lock_guard<std::mutex> lock(mutex_);
		connections_[cfd] = conn;
	}

	// 确保所有回调都被设置
	if (messageCallback_)
	{
		conn->SetOnMessageCallback(messageCallback_);
	}
	if (connectedCallback_)
	{
		conn->SetOnConnectedCallback(connectedCallback_);
	}
	if (closedCallback_)
	{
		conn->SetOnClosedCallback(closedCallback_);
	}
	if (sentCallback_)
	{
		conn->SetOnSentCallback(sentCallback_);
	}

	conn->SetRemoveConnCallback([this](const ConnectionPtr &conn)
								{ HandleRemoveConntion(conn); });
	conn->ConnectEstablished();
}

void Server::HandleRemoveConntion(const ConnectionPtr &conn)
{
	auto n = connections_.erase(conn->fd());
	assert(n == 1);
	conn->ConnectDestroyed();
}
