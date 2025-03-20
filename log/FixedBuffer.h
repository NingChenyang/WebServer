#pragma once
#include <iostream>
#include <string>
#include <cstring>

const int KSmallBuffer = 1024;
const int KLargeBuffer = 1024 * 1000;

template <int SIZE>
class FixedBuffer
{
public:
    FixedBuffer() : cur_(data_)
    {
        memset(data_, 0, SIZE); // 在构造时初始化缓冲区
    }

    ~FixedBuffer() = default;

    void Append(const char *buf, size_t len)
    {
        if (!buf || len == 0 || len > SIZE)
            return;
        if (Available() >= len)
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    void AppendComplete(size_t len)
    {
        if (len <= Available() && cur_ + len <= End())
        {
            cur_ += len;
        }
    }

    // 修改：移除 ReSet() 函数，使用 ReSet() 替代
    void ReSet()
    {
        cur_ = data_;
        memset(data_, 0, SIZE);
    }

    // 确保返回有效指针
    const char *Data() const
    {
        return data_;
    }

    int Length() const
    {
        return cur_ - data_;
    }

    char *Current()
    {
        return cur_;
    }

    size_t Available() const
    {
        return static_cast<size_t>(End() - cur_);
    }

private:
    const char *End() const
    {
        return data_ + SIZE;
    }

    char data_[SIZE];
    char *cur_;
};
