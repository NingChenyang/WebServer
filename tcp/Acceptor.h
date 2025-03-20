#pragma once
#include "Socket.h"
#include "Channel.h"
#include "InetAddress.h"
#include "Epoll.h"
#include "Server.h"
#include "EventLoop.h"
#include "Callbacks.h"
class Acceptor
{
private:
	EventLoop *main_loop_;
	Socket *acceptsock_;	 // 调整顺序
	Channel *acceptchannel_; // 调整顺序
	bool listen_;

	NewConnectionCallback newConntionCallback_;

	void HandleRead();

public:
	Acceptor(InetAddress &listenaddr, EventLoop *main_loop);
	~Acceptor();

	void Listen();
	void SetNewConntionCallback(const NewConnectionCallback &cb);
};
