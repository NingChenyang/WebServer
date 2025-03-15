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
    FixedBuffer()
        : cur_(data_)
    {
    }
    ~FixedBuffer() {}

    void Append(const char *buf, size_t len)
    {
        if (Available() > len)
        {
            memcpy(cur_, buf, len);
            cur_ += len; // 修改：直接移动指针
        }
    }

    void AppendComplete(size_t len)
    {
        cur_ += len; // 修改：使用 += 运算符
    }

    // 清空数据
    void MenSet()
    {
        memset(data_, 0, sizeof(data_)); // 修复：data_ 而不是 data
    }

    // 重置数据
    void ReSet()
    {
        cur_ = data_;
    }

    // 修复：返回类型声明
    const char *Data() const
    {
        return data_;
    }

    // 数据长度
    size_t Length() const
    {
        return static_cast<size_t>(cur_ - data_);
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
        return data_ + SIZE; // 修复：使用 SIZE 而不是 sizeof data_
    }

    char data_[SIZE];
    char *cur_;
};
