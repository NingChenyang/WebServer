#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t nums, std::string thread_type) : stop_(false), thread_type_(thread_type)
{
    for (size_t i = 0; i < nums; i++)
    {

        threads_.emplace_back([this]()
            {
                std::cout << "create:" << thread_type_ << "thread" << syscall(SYS_gettid) << std::endl;
                while (this->stop_ == false)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->mutex_);
                        this->cond_.wait(lock, [this]
                            { return ((this->stop_ == true) || this->taskqueue_.empty() == false); });
                        if (this->stop_ == true && this->taskqueue_.empty())
                            return;

                        task = std::move(this->taskqueue_.front());
                        this->taskqueue_.pop();

                    }
                    // if(thread_type_ == "worker")
                    //    std::cout << thread_type_ << "thread is " << "running:" << syscall(SYS_gettid) << std::endl;
                    task();
                } });
    }
}

void ThreadPool::AddTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskqueue_.emplace(task);
    }
    cond_.notify_one();
}

ThreadPool::~ThreadPool()
{
    Stop();
}

size_t ThreadPool::Size()
{
    return threads_.size();
}

void ThreadPool::Stop()
{
    if (stop_)
        return;
    stop_ = true;
    cond_.notify_all();
    for (auto& thread : threads_)
    {
        thread.join();
    }
}
