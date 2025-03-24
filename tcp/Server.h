#pragma once
#include"Acceptor.h"
#include"Buffer.h"
#include"Channel.h"
#include"EventLoop.h"
#include"InetAddress.h"
#include"Socket.h"
#include"util.h"
#include<iostream>
#include"Callbacks.h"
#include<memory>
#include"ThreadPool.h"
#include<unordered_map>
#include"Connection.h"
#include<mutex>
class Socket;
class Channel;
class EventLoop;
class Acceptor;
class InetAddress;
const int READ_BUFFER = 1024;

class Server
{
public:
	using ConnectionMap = std::unordered_map<int, ConnectionPtr>;
private:
	EventLoop* main_loop_;
	std::vector<std::unique_ptr<EventLoop>> sub_loop_;
	int thread_nums_;
	ThreadPool *io_pool_;
	std::unique_ptr<Acceptor> acceptor_;

	std::mutex mutex_;
	ConnectionMap connections_;
	//用户自定的函数
	//业务处理
	OnMessageCallback messageCallback_;
	OnClosedCallback closedCallback_;
	OnSentCallback sentCallback_;
	OnConnectedCallback connectedCallback_;

	void HandleNewConntion(int cfd, const InetAddress& peerAddr);
	void HandleRemoveConntion(const ConnectionPtr& conn);
	void HandleRemoveConntionInLoop(const ConnectionPtr &conn);

public:
	Server(InetAddress& serv_addr, int thread_nums);
	~Server();
	void Run();
	void Stop();
	void SetOnMessageCallback(const OnMessageCallback& cb);
	void SetOnClosedCallback(const OnClosedCallback& cb);
	void SetOnSentCallback(const OnSentCallback& cb);
	void SetOnConnectedCallback(const OnConnectedCallback& cb);
};

