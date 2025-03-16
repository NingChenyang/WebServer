#include "Buffer.h"


Buffer::Buffer() : Buffer_(InitialSize + PrependSize), ReadIndex_(PrependSize), WriteIndex_(PrependSize) {}

const char* Buffer::Peek() const {
    return Buffer_.data() + ReadIndex_;
}

size_t Buffer::ReadableBytes() const {
    return WriteIndex_ - ReadIndex_;
}

char* Buffer::BeginWrite() {
    return Buffer_.data() + WriteIndex_;
}

size_t Buffer::WritableBytes() const {
    return Buffer_.size() - WriteIndex_;
}

char* Buffer::BeginPrepend() {
    return Buffer_.data() + PrependSize - PrependableBytes();
}

size_t Buffer::PrependableBytes() const {
    return ReadIndex_;
}

void Buffer::Prepend(const void* data, size_t len) {
    assert(len <= PrependableBytes());
    ReadIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, BeginPrepend());
}

void Buffer::Append(const char* data, size_t len) {
    EnsureWritableBytes(len);
    std::copy(data, data + len, BeginWrite());
    WriteIndex_ += len;
}

void Buffer::Append(const std::string &str)
{
    Append(str.data(), str.size());
}

void Buffer::Append(const char *data)
{
    Append(data, strlen(data));
}

void Buffer::EnsureWritableBytes(size_t len) {
    if (WritableBytes() < len) {
        if (PrependableBytes() + WritableBytes() >= len + PrependSize) {
            // 如果缓冲区前面有空间，移动数据到前面
            size_t readable = ReadableBytes();
            std::copy(Peek(), Peek() + readable, Buffer_.data() + PrependSize);
            ReadIndex_ = PrependSize;
            WriteIndex_ = ReadIndex_ + readable;
        }
        else {
            // 否则扩容
            size_t newSize = std::min(Buffer_.size() * 2, MaxSize);
            while (newSize < WriteIndex_ + len) {
                newSize = std::min(newSize * 2, MaxSize);
            }
            Buffer_.resize(newSize);
        }
    }
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    ReadIndex_ += len;
    if (ReadIndex_ == WriteIndex_) {
        // 如果所有数据都被读取，重置指针
        ReadIndex_ = PrependSize;
        WriteIndex_ = PrependSize;
    }
}

void Buffer::RetrieveAll()
{
    Retrieve(ReadableBytes());
}

void Buffer::Clear() {
    ReadIndex_ = PrependSize;
    WriteIndex_ = PrependSize;
}

std::string Buffer::ReadBuffer(size_t len)
{
    std::string packet(Peek(), len);
    Retrieve(len);
    return packet;
}

std::string Buffer::ReadBufferAll()
{
    return ReadBuffer(ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    char extrabuf[65536];  // 64KB 额外缓冲区
    struct iovec vec[2];
    const size_t writable = WritableBytes();
    vec[0].iov_base = BeginWrite();
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 使用 readv 读取数据
    const ssize_t n = ::readv(fd, vec, 2);
    if (n < 0) {
        *saveErrno = errno;
        return n;  // 读取错误
    }
    else if (static_cast<size_t>(n) <= writable) {
        WriteIndex_ += n;
    }
    else {
        WriteIndex_ = Buffer_.size();
        Append(extrabuf, n - writable);
    }
    return n;
}

ssize_t Buffer::WriteFd(int fd) {
    const size_t readable = ReadableBytes();
    const ssize_t n = ::write(fd, Peek(), readable);
    if (n > 0) {
        Retrieve(n);  // 移动读指针
    }
    return n;
}