#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <cstring>
#include <cstddef>   // for size_t
#include <sys/uio.h> // for struct iovec
#include <algorithm> // for std::copy, std::min
#include <cassert>   // for assert
#include <unistd.h>  // for read, write, readv
#include <fcntl.h>   // for open
#include <sys/uio.h> // for struct iovec
#include <mutex>     // for std::mutex, std::lock_guard
#include "../log/Logger.h"
#include "../http/HttpUtil.h"

class Buffer
{
public:
    const size_t InitialSize = 1024;        // 初始缓冲区大小
    const size_t MaxSize = 1024 * 1024 * 8; // 最大缓冲区大小（8MB）
    const size_t PrependSize = 8;           // prependable 区域大小（8 字节）

    Buffer();


    // 返回可读数据的起始地址
    const char *Peek() const;

    // 返回可读数据的长度
    size_t ReadableBytes() const;

    // 返回可写空间的起始地址
    char *BeginWrite();
    const char *BeginWrite() const;

    // 返回可写空间的长度
    size_t WritableBytes() const;

    // 返回 prependable 区域的起始地址
    char *BeginPrepend();

    // 返回 prependable 区域的长度
    size_t PrependableBytes() const;

    // 在 prependable 区域写入数据
    void Prepend(const void *data, size_t len);

    // 追加数据到缓冲区
    void Append(const char *data, size_t len);
    void Append(const std::string &str);
    void Append(const char *data);

    // 确保有足够的可写空间
    void EnsureWritableBytes(size_t len);

    // 读取数据后移动读指针
    void Retrieve(size_t len);
    void RetrieveAll();
    void RetrieveUntil(const char *end);

    // 清空缓冲区
    void
    Clear();

    //
    std::string ReadBuffer(size_t len);
    std::string ReadBufferAll();

    // 从文件描述符读取数据到缓冲区
    ssize_t ReadFd(int fd, int *saveErrno);

    // 将缓冲区数据写入文件描述符
    ssize_t WriteFd(int fd);
    // 查找\r\n
    const char *FindCRLF() const;
    // const char* FindCRLF(const char* start) const;

private:
    std::vector<char> Buffer_; // 数据缓冲区
    size_t ReadIndex_;         // 读指针
    size_t WriteIndex_;        // 写指针
    mutable std::mutex mutex_; // 互斥锁
};

#endif // BUFFER_H