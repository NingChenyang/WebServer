#include "Epoll.h"
const int maxevents = 1024;
Epoll::Epoll()
{
	epfd_ = epoll_create(1);
	perror_if(epfd_ < 0, "epoll_create error");
	events_ = new epoll_event[maxevents];
}

Epoll::~Epoll()
{
	if (epfd_ > 0)
	{
		close(epfd_);
		epfd_ = -1;
	}
	delete[] events_; // 添加这行释放内存
}

vector<Channel *> Epoll::Epoll_Wait(int timeout)
{
	vector<Channel *> activechannels;
	int nums = epoll_wait(epfd_, events_, maxevents, timeout);

	if (nums < 0)
	{
		if (errno == EINTR)
		{
		}
		else
		{
			perror_if(nums < 0, "epoll_wait error");
		}
	}
	for (int i = 0; i < nums; i++)
	{
		Channel *ch = static_cast<Channel *>(events_[i].data.ptr);
		ch->SetRevents(events_[i].events);
		activechannels.emplace_back(ch);
	}
	return activechannels; // 自动触发移动语义
}

void Epoll::UpdateChannel(Channel *ch)
{
	epoll_event ev;
	ev.data.ptr = ch;
	ev.events = ch->Events();
	if (ch->IsInEpoll())
	{
		int ret = epoll_ctl(epfd_, EPOLL_CTL_MOD, ch->fd(), &ev);
		perror_if(ret < 0, "EPOLL_CTL_MOD error");
	}
	else
	{
		int ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, ch->fd(), &ev);
		perror_if(ret < 0, "EPOLL_CTL_ADD error");
		ch->SetInEpoll(true);
	}
}

void Epoll::DeleteChannel(Channel *ch)
{
	if (ch->IsInEpoll())
	{
		int ret = epoll_ctl(epfd_, EPOLL_CTL_DEL, ch->fd(), nullptr);
		perror_if(ret < 0, "EPOLL_CTL_DEL error");
		ch->SetInEpoll(false);
	}
}
