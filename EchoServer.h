#pragma once
#include "tcp/Server.h"
#include "tcp/ThreadPool.h"
#include "tcp/InetAddress.h"
#include <sys/syscall.h>
#include"tcp/TimeStamp.h"
class EchoServer
{
public:
    EchoServer(InetAddress &listenAddr, int numThreads, int worker_nums);
    void start();
    void Stop();

private:
    void HandleOnMessage(ConnectionPtr conn, const std::string &buf);
    void OnMessage(ConnectionPtr conn, const std::string &buf);
    void HandleOnConnected(ConnectionPtr conn);
    void HandleOnClosed(ConnectionPtr conn);
    void HandleOnSent(ConnectionPtr conn);

private:
    Server server_;
    ThreadPool worker_pool_;
};
