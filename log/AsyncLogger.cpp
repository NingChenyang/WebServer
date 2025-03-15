#include "AsyncLogger.h"

AsyncLogger::AsyncLogger(std::string basename, off_t roll_size, int flush_interval)
    : basename_(basename), flush_interval_(flush_interval), roll_size_(roll_size),is_running_(false), current_buffer_(std::make_unique<Buffer>()), next_buffer_(std::make_unique<Buffer>())
{
    current_buffer_->MenSet();
    next_buffer_->MenSet();
    buffers_.reserve(16);
}

AsyncLogger::~AsyncLogger()
{
    if (is_running_)
    {
        Stop();
    }
}

void AsyncLogger::Append(const char *logline, size_t len)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (current_buffer_->Available() > len)
    {
        current_buffer_->Append(logline, len);
    }
    else
    {
        buffers_.emplace_back(std::move(current_buffer_));
        // current_buffer_.reset();
        if (next_buffer_)
        {
            current_buffer_ = std::move(next_buffer_);
        }
        else
        {
            current_buffer_.reset(new Buffer);
        }
        current_buffer_->Append(logline, len);
        cond_.notify_one();
    }
}

void AsyncLogger::Start()
{
    is_running_ = true;
    log_thread_ = std::thread([this]
                              { LogThreadFunction(); });
}

void AsyncLogger::Stop()
{
    is_running_ = false;
    cond_.notify_one();
    if (log_thread_.joinable())
    {
        log_thread_.join();
    }
}

void AsyncLogger::LogThreadFunction()
{
    printf("AsyncLogger::ThreadFunc()\n");
    // 打开磁盘日志文件,即是创建LogFile对象
    LogFile output(basename_,roll_size_);

    auto new_buffer_1 = std::make_unique<Buffer>();
    auto new_buffer_2 = std::make_unique<Buffer>();
    new_buffer_1->MenSet();
    new_buffer_2->MenSet();

    // 用于存放写满的缓冲区，用来读，写入文件
    BufferVector buffer_to_write;
    buffer_to_write.reserve(16);

    // 开始写入循环

    while (is_running_)
    {
        { // 进行容器交换
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty())
            {
                cond_.wait_for(lock, std::chrono::seconds(flush_interval_));
            }
            if (current_buffer_->Length() > 0)
            {
                buffers_.emplace_back(std::move(current_buffer_));
            }

            current_buffer_ = std::move(new_buffer_1);
            buffer_to_write.swap(buffers_); // 跟存放写满的缓冲区的容器交换

            if (!next_buffer_)
            {
                next_buffer_ = std::move(new_buffer_2);
            }
        }
        // 当缓冲区过多时，，，，，先丢弃
        if (buffer_to_write.size() > 25)
        {
            buffer_to_write.erase(buffer_to_write.begin() + 2, buffer_to_write.end());
        }
        // 开始从缓冲区读取写入
        for (const auto &buffer : buffer_to_write)
        {
            output.Append(buffer->Data(), buffer->Length());
        }
        // 丢弃多余的空间
        if (buffer_to_write.size() > 2)
        {
            buffer_to_write.reserve(2);
        }
        // 回复备用缓存空间new_buffer_1,new_buffer_2
        if (!new_buffer_1)
        {
            new_buffer_1 = std::move(buffer_to_write.back());
            buffer_to_write.pop_back();
            new_buffer_1->MenSet();
        }
        if (!new_buffer_2)
        {
            new_buffer_2 = std::move(buffer_to_write.back());
            buffer_to_write.pop_back();
            new_buffer_2->MenSet();
        }
        buffer_to_write.clear();
        output.Flush();
    }
    output.Flush();
}
