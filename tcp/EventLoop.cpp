#include "EventLoop.h"
int CreateTimerfd(int sec = 30)
{
	int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	struct itimerspec timeout;
	memset(&timeout, 0, sizeof(struct itimerspec));
	timeout.it_value.tv_sec = sec;
	timeout.it_value.tv_nsec = 0;
	timerfd_settime(tfd, 0, &timeout, 0);
	return tfd;
}
void EventLoop::HandleWakeUp()
{
	uint64_t val;
	ssize_t ret = read(wake_up_fd_, &val, sizeof(uint64_t));
	(void)ret; // 标记变量已使用
	std::function<void()> fn;

	std::lock_guard<std::mutex> lock(qmutex_);
	while (!task_queue_.empty())
	{
		fn = move(task_queue_.front());
		task_queue_.pop();
		fn();
	}
}
void EventLoop::NewLoopConntion(ConnectionPtr conn)
{
	// std::lock_guard<std::mutex> gd(cmutex_);
	loop_conns_[conn->fd()] = conn;
	// for (auto &a : loop_conns_)
	// {
	// 	std::cout << "loop_conns " << a.first << std::endl;
	// }
}

void EventLoop::RemoveLoopConn(int fd)
{
	// std::lock_guard<std::mutex> lock(cmutex_);
	loop_conns_.erase(fd);
	
}
void EventLoop::SetRemoveLoopConnCallback(RemoveLoopConnCallback fn)
{
	RemoveLoopConnCallback_ = std::move(fn);
}
void EventLoop::HandleTimer()
{
	struct itimerspec timeout;
	memset(&timeout, 0, sizeof(struct itimerspec));
	timeout.it_value.tv_sec = time_tvl_;
	timeout.it_value.tv_nsec = 0;
	timerfd_settime(timer_fd_, 0, &timeout, 0);
	if (is_main_loop_)
	{
		// cout << "mainloop Timeout." << endl;
	}
	else
	{
		// cout << "subloop Timeout." << endl;
		// cout << syscall(SYS_gettid)<<endl;
		time_t now = time(0);

		// 加锁
		std::lock_guard<std::mutex> lock(cmutex_);

		for (auto it = loop_conns_.begin(); it != loop_conns_.end();)
		{
			if (it->second->Timeout(now, time_out_))
			{
				RemoveLoopConnCallback_(it->second);
				// cout << it->first << endl;

				it = loop_conns_.erase(it); // 使用 erase 的返回值更新迭代器
			}
			else
			{
				++it; // 仅在不删除元素时递增迭代器
			}
		}
	}
}

EventLoop::EventLoop(bool is_main_loop, int time_tvl, int time_out)
	: ep_(std::make_unique<Epoll>()), wake_up_fd_(eventfd(0, EFD_NONBLOCK)), wake_channel_(new Channel(wake_up_fd_, this)), timer_fd_(CreateTimerfd(time_tvl)), timer_channel_(new Channel(timer_fd_, this)), time_tvl_(time_tvl), time_out_(time_out)
{

	wake_channel_->SetReadCallback(std::bind(&EventLoop::HandleWakeUp, this));
	wake_channel_->EnableReading();
	
	if (!is_main_loop)
	{
		// 可以添加事件循环特有的初始化
		timer_channel_->SetReadCallback(std::bind(&EventLoop::HandleTimer, this));
		timer_channel_->EnableReading();
	}
}

EventLoop::~EventLoop()

{
	close(wake_up_fd_);
	close(timer_fd_);
	wake_channel_->Remove();
	timer_channel_->Remove();
}

void EventLoop::Run()
{
	quit_ = false;
	thread_id_ = static_cast<pid_t>(syscall(SYS_gettid));
	while (!quit_)
	{
		activechannels.clear();
		vector<Channel *> activechannels = ep_->Epoll_Wait();
		for (auto &ch : activechannels)
		{
			ch->HandleEvent();
		}
	}
}

void EventLoop::Stop()
{
	quit_ = true;
	WakeUp();
}

bool EventLoop::IsInLoop()
{
	return thread_id_ == syscall(SYS_gettid);
}

void EventLoop::AddLoopQueue(std::function<void()> fn)
{
	{
		std::lock_guard<std::mutex> lock(qmutex_);
		task_queue_.push(fn);
	}
	// 唤醒
	WakeUp();
}

void EventLoop::RunInLoop(std::function<void()> fn)
{
	if (IsInLoop())
	{
		fn();
	}
	else
	{
		AddLoopQueue(std::move(fn));
	}
	
}

void EventLoop::WakeUp()
{
	uint64_t val = 1;
	ssize_t ret = write(wake_up_fd_, &val, sizeof(uint64_t));
	(void)ret; // 标记变量已使用
}

void EventLoop::UpdateChannel(Channel *ch)
{
	ep_->UpdateChannel(ch);
}

void EventLoop::DeleteChannel(Channel *ch)
{
	ep_->DeleteChannel(ch);
}
