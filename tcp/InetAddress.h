#pragma once
#include<arpa/inet.h>
#include<string>
#include<cstring>
class InetAddress
{
public:
	InetAddress();
	InetAddress(sockaddr_in& addr);
	explicit InetAddress(const struct sockaddr_in& addr)
		: addr_(addr)
	{
	}

	//const sockaddr_in& getAddr()const { return addr_; }
	const struct sockaddr_in* getSockAddr() const { return &addr_; }

	InetAddress(const char*ip, unsigned short port);
	const sockaddr_in& getAddr();
	void SetAddr(sockaddr_in& addr_);
	std::string ToIp()const;
	uint16_t ToPort()const;
	std::string ToIpPort()const;
private:
	sockaddr_in addr_;

};

