#include "AsyncLogger.h"

AsyncLogger::AsyncLogger(std::string basename, off_t roll_size, int flush_interval)
    : basename_(basename), flush_interval_(flush_interval), roll_size_(roll_size), is_running_(false), current_buffer_(new Buffer()), next_buffer_(new Buffer())
{
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
    if (!logline || len == 0 || !is_running_)
        return;

    try
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (current_buffer_->Available() > len)
        {
            current_buffer_->Append(logline, len);
        }
        else
        {
            buffers_.emplace_back(std::move(current_buffer_));

            if (next_buffer_)
            {
                current_buffer_ = std::move(next_buffer_);
            }
            else
            {
                current_buffer_ = std::make_unique<Buffer>();
            }

            if (current_buffer_)
            {
                current_buffer_->Append(logline, len);
                cond_.notify_one();
            }
        }
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "AsyncLogger::Append failed: %s\n", e.what());
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
    try
    {
        LogFile output(basename_, roll_size_, flush_interval_);

        auto new_buffer_1 = std::make_unique<Buffer>();
        auto new_buffer_2 = std::make_unique<Buffer>();

        if (new_buffer_1)
            new_buffer_1->ReSet();
        if (new_buffer_2)
            new_buffer_2->ReSet();

        BufferVector buffer_to_write;
        buffer_to_write.reserve(16);

        while (is_running_)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (buffers_.empty())
                {
                    cond_.wait_for(lock, std::chrono::seconds(flush_interval_));
                }

                if (current_buffer_ && current_buffer_->Length() > 0)
                {
                    buffers_.emplace_back(std::move(current_buffer_));
                }

                current_buffer_ = std::move(new_buffer_1);
                buffer_to_write.swap(buffers_);

                if (!next_buffer_)
                {
                    next_buffer_ = std::move(new_buffer_2);
                }
            }

            // 限制缓冲区大小
            if (buffer_to_write.size() > 25)
            {
                buffer_to_write.resize(2);
            }

            // 写入文件
            for (const auto &buffer : buffer_to_write)
            {
                if (buffer && buffer->Data() && buffer->Length() > 0)
                {
                    output.Append(buffer->Data(), buffer->Length());
                }
            }

            // 恢复缓冲区
            if (!new_buffer_1)
            {
                if (!buffer_to_write.empty())
                {
                    new_buffer_1 = std::move(buffer_to_write.back());
                    buffer_to_write.pop_back();
                    if (new_buffer_1)
                        new_buffer_1->ReSet();
                }
                else
                {
                    new_buffer_1 = std::make_unique<Buffer>();
                }
            }

            if (!new_buffer_2)
            {
                if (!buffer_to_write.empty())
                {
                    new_buffer_2 = std::move(buffer_to_write.back());
                    buffer_to_write.pop_back();
                    if (new_buffer_2)
                        new_buffer_2->ReSet();
                }
                else
                {
                    new_buffer_2 = std::make_unique<Buffer>();
                }
            }

            buffer_to_write.clear();
            output.Flush();
        }
        output.Flush();
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "LogThreadFunction failed: %s\n", e.what());
    }
}
