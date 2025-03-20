#pragma once

#include "FixedBuffer.h"

class LogStream
{
public:
    using LogBuffer = FixedBuffer<KSmallBuffer>;

    LogStream();
    ~LogStream();
    // 重载流输出操作符
    LogStream &operator<<(bool v)
    {
        buffer_.Append(v ? "1" : "0", 1);
        return *this;
    }
    LogStream &operator<<(short);
    LogStream &operator<<(int);
    LogStream &operator<<(size_t v);
    LogStream &operator<<(long);
    LogStream &operator<<(long long);
    LogStream &operator<<(float);
    LogStream &operator<<(double);
    LogStream &operator<<(char);
    LogStream &operator<<(const char *);
    LogStream &operator<<(const std::string &);

    // 把数据输出到缓冲区
    void Append(const char *data, int len) { buffer_.Append(data, len); }

    // 获取缓冲区的对象
    const LogBuffer &buffer() const { return buffer_; }

private:
    // 格式化数字类型为字符串并输出到缓冲区
    template <class T>
    void FormatInteger(T);

    // 缓冲区 自己定义的Buffer模板类
    LogBuffer buffer_;

    // 数字转为字符串后可以占用的最大字节数
    static const int KMaxNumbericSize = 48;
};
