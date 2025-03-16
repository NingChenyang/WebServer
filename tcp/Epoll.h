#pragma once
#include<netinet/in.h>
#include<sys/epoll.h>
#include"Channel.h"
#include"util.h"
#include<vector>
#include<unistd.h>
using std::vector;
class Channel;
class Epoll
{
public:
	Epoll();
	~Epoll();
	vector<Channel*> Epoll_Wait(int timeout=-1) ;
	void UpdateChannel(Channel* ch);
	void DeleteChannel(Channel* ch);
private:
	int epfd_;
	epoll_event* events_;
};

