#include "Acceptor.h"

Acceptor::Acceptor(InetAddress &listenaddr, EventLoop *main_loop)
	: main_loop_(main_loop)
{
	acceptsock_ = new Socket();

	acceptsock_->SetKeepalive(true);
	acceptsock_->SetReuseaddr(true);
	acceptsock_->SetReuseport(true);
	acceptsock_->SetTcpnodelay(true);

	acceptsock_->Bind(listenaddr);
	acceptchannel_ = new Channel(acceptsock_->fd(), main_loop_);
	auto cb = [this]()
	{ HandleRead(); };
	acceptchannel_->SetReadCallback(cb);
	acceptchannel_->SetET();
	acceptchannel_->EnableReading(); // 确保 acceptchannel_ 启动监听
}

Acceptor::~Acceptor()
{
	delete acceptchannel_;
	delete acceptsock_;
}

void Acceptor::HandleRead()
{
	InetAddress clnt_addr;
	int cfd;
	while ((cfd = acceptsock_->Accecpt(clnt_addr)) > 0)
	{
		if (newConntionCallback_)
		{
			newConntionCallback_(cfd, clnt_addr);
		}
	}
	// 处理 EAGAIN 错误,表示当前没有新连接了
	if (cfd < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			perror("accept error");
		}
	}
}

void Acceptor::Listen()
{
	acceptsock_->Listen();
}

void Acceptor::SetNewConntionCallback(const NewConnectionCallback &cb)
{
	newConntionCallback_ = cb;
}
