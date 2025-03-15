#pragma once
#include "LogStream.h"
#include <vector>
#include <string>
#include"LogFile.h"
#include <memory>
#include <condition_variable>
#include <thread>
class AsyncLogger
{
public:
    AsyncLogger(std::string basename,off_t roll_size, int flush_interval = 3);
    ~AsyncLogger();
    void Append(const char *logline, size_t len);
    void Start();
    void Stop();

private:
    void LogThreadFunction();
    using Buffer = FixedBuffer<KLargeBuffer>;
    using BufferPtr = std::unique_ptr<Buffer>;
    using BufferVector = std::vector<BufferPtr>;
    //冲刷间隔时间 默认3s
    const int flush_interval_;
    const off_t roll_size_;
    bool is_running_;
    // Log后端线程
    std::thread log_thread_;
    std::string basename_;
    std::mutex mutex_;
    std::condition_variable cond_;
    BufferPtr current_buffer_;
    BufferPtr next_buffer_;
    BufferVector buffers_;
};
