#include "Channel.h"

Channel::Channel(int fd, EventLoop *loop)
	: fd_(fd), loop_(loop), events_(0), revents_(0), is_in_poll_(false), tied_(false)
{
}

Channel::~Channel()
{
}

void Channel::SetEvents(uint32_t event)
{
	events_ = event;
}

uint32_t Channel::Events()
{
	return events_;
}

void Channel::SetRevents(uint32_t revent)
{
	revents_ = revent;
}

uint32_t Channel::Revents()
{
	return revents_;
}

bool Channel::IsInEpoll()
{
	return is_in_poll_;
}

void Channel::SetInEpoll(bool in)
{
	is_in_poll_ = in;
}

int Channel::fd() const
{
	return fd_;
}

void Channel::SetReadCallback(EventCallback cb)

{
	readCallback_ = std::move(cb);
}

void Channel::SetWriteCallback(EventCallback cb)
{
	writeCallback_ = std::move(cb);
}

void Channel::SetCloseCallback(EventCallback cb)
{
	closeCallback_ = std::move(cb);
}

void Channel::SetErrorCallback(EventCallback cb)
{
	errorCallback_ = std::move(cb);
}

void Channel::SetET()
{
	events_ |= EPOLLET;
}

void Channel::EnableReading()
{
	events_ |= (EPOLLIN | EPOLLPRI);
	update();
}

void Channel::DisableReading()
{
	events_ &= ~(EPOLLIN | EPOLLPRI);
	update();
}

void Channel::EnableWriting()
{
	events_ |= EPOLLOUT;
	update();
}

void Channel::DisableWriting()
{
	events_ &= ~EPOLLOUT;
	update();
}

void Channel::DisableAll()
{
	events_ = 0;
	update();
}

bool Channel::IsNoneEvent() const
{
	return events_ == 0;
}

bool Channel::IsWrite() const
{
	return events_ & EPOLLOUT;
}

bool Channel::IsRead() const
{
	return events_ & (EPOLLIN | EPOLLPRI);
}

void Channel::Tie(const std::shared_ptr<void> &obj)
{
	tie_ = obj;
	tied_ = true;
}

void Channel::HandleEvent()
{
	if (tied_)
	{
		if(tie_.expired())
		{ // 如果tie_过期了，说明这个Channel已经不需要了，直接返回
			return;
		}
		else
		{ // 如果没有过期，说明这个Channel还在使用中，继续执行下面的代码
			auto tie = tie_.lock();
			if (tie == nullptr)
			{
				return;
			}
			else if (tie)
			{
				HandleEventWithGuard();
			}
		}
		
		
	}
	else
	{ // 这个else里面是用来建立连接的，因为开始建立连接的时候tied_是false,是连接建立后开始通信tied_才为true
		HandleEventWithGuard();
	}
}

void Channel::HandleEventWithGuard()
{
	if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
	{ // 当事件为挂起并没有可读事件时
		revents_ &= ~EPOLLHUP;
		if (closeCallback_)
			closeCallback_();
	}
	if (revents_ & EPOLLERR)
	{
		revents_ &= ~EPOLLERR;
		if (errorCallback_)
			errorCallback_();
	}
	if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
	{ // 关于读的事件
		if (readCallback_)
		{
			revents_ &= ~EPOLLIN;
			revents_ &= ~EPOLLPRI;
			revents_ &= ~EPOLLRDHUP;
			readCallback_();
		}
	}
	if (revents_ & EPOLLOUT)
	{ // 关于写的事件
		revents_ &= ~EPOLLOUT;
		if (writeCallback_)
			writeCallback_();
	}
}

void Channel::Remove()
{
	loop_->DeleteChannel(this);
}

void Channel::update()
{
	loop_->UpdateChannel(this);
}
