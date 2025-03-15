#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <condition_variable>
#include <queue>
#include <future>
#include <functional>
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>
class ThreadPool
{
private:
    std::vector<std::thread> threads_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<std::function<void()>> taskqueue_;
    std::atomic_bool stop_;
    std::string thread_type_;

public:
    ThreadPool(size_t nums, std::string thread_type);
    void AddTask(std::function<void()>task);
    ~ThreadPool();
    size_t Size();
    //停止线程
    void Stop();

private:
};
