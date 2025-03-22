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
    const size_t InitialSize = 1024;        // ��ʼ��������С
    const size_t MaxSize = 1024 * 1024 * 8; // ��󻺳�����С��8MB��
    const size_t PrependSize = 8;           // prependable �����С��8 �ֽڣ�

    Buffer();


    // ���ؿɶ����ݵ���ʼ��ַ
    const char *Peek() const;

    // ���ؿɶ����ݵĳ���
    size_t ReadableBytes() const;

    // ���ؿ�д�ռ����ʼ��ַ
    char *BeginWrite();
    const char *BeginWrite() const;

    // ���ؿ�д�ռ�ĳ���
    size_t WritableBytes() const;

    // ���� prependable �������ʼ��ַ
    char *BeginPrepend();

    // ���� prependable ����ĳ���
    size_t PrependableBytes() const;

    // �� prependable ����д������
    void Prepend(const void *data, size_t len);

    // ׷�����ݵ�������
    void Append(const char *data, size_t len);
    void Append(const std::string &str);
    void Append(const char *data);

    // ȷ�����㹻�Ŀ�д�ռ�
    void EnsureWritableBytes(size_t len);

    // ��ȡ���ݺ��ƶ���ָ��
    void Retrieve(size_t len);
    void RetrieveAll();
    void RetrieveUntil(const char *end);

    // ��ջ�����
    void
    Clear();

    //
    std::string ReadBuffer(size_t len);
    std::string ReadBufferAll();

    // ���ļ���������ȡ���ݵ�������
    ssize_t ReadFd(int fd, int *saveErrno);

    // ������������д���ļ�������
    ssize_t WriteFd(int fd);
    // ����\r\n
    const char *FindCRLF() const;
    // const char* FindCRLF(const char* start) const;

private:
    std::vector<char> Buffer_; // ���ݻ�����
    size_t ReadIndex_;         // ��ָ��
    size_t WriteIndex_;        // дָ��
    mutable std::mutex mutex_; // ������
};

#endif // BUFFER_H