#pragma once
#include"Epoll.h"
#include"Callbacks.h"
#include"Channel.h"
#include<memory>
#include "Connection.h"
#include<atomic>
#include<vector>
#include<functional>
#include<queue>
#include<sys/syscall.h>
#include<mutex>
#include<iostream>
#include<sys/eventfd.h>
#include<sys/timerfd.h>
#include<cstring>
#include<unordered_map>
class Epoll;
class Channel;
class EventLoop
{
private:
	std::unique_ptr<Epoll> ep_;
	std::vector<Channel*> activechannels;
	std::atomic_bool quit_;

	std::mutex qmutex_;
	std::queue<std::function<void()>> task_queue_;

	int wake_up_fd_;
	std::unique_ptr<Channel> wake_channel_;

	int timer_fd_;
	std::unique_ptr<Channel> timer_channel_;

	std::mutex cmutex_;
	std::unordered_map<int, ConnectionPtr> loop_conns_;

	int time_tvl_;
	int time_out_;
	pid_t thread_id_;
	bool is_main_loop_;

	RemoveLoopConnCallback RemoveLoopConnCallback_;
	void HandleWakeUp();
	void HandleTimer();
public:
	EventLoop(bool is_main_loop, int time_tvl,int time_out);
	~EventLoop();
	void Run();
	void Stop();
	bool IsInLoop();
	void AddLoopQueue(std::function<void()> fn);
	void RunInLoop(std::function<void()> fn);
	void WakeUp();

	void UpdateChannel(Channel* ch);
	void DeleteChannel(Channel* ch);

	void NewLoopConntion(ConnectionPtr conn);
	//删除conns
	void RemoveLoopConn(int fd);

	void SetRemoveLoopConnCallback(RemoveLoopConnCallback fn);

};

