#pragma once
#include<sys/epoll.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include"InetAddress.h"
#include"util.h"
#include <netinet/tcp.h>

#include<unistd.h>
#include<fcntl.h>
#include<iostream>
class Socket
{
public:
	Socket();
	Socket(int fd);
	~Socket();
	// 设置SO_REUSEADDR
	void SetReuseaddr(bool on);
	// SO_REUSERPORT
	void SetReuseport(bool on);
	// TCP_NODELAY
	void SetTcpnodelay(bool on);
	// SO_KEEPALIVE
	void SetKeepalive(bool on);
	void Bind( InetAddress &ser_addr);
	void Listen();
	int Accecpt(InetAddress &cli_addr);
	int fd() const;
	void SetNonblocking();

private:
	int fd_;
};

