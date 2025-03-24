#include "EchoServer.h"
#include <iostream>

EchoServer::EchoServer(InetAddress &listenAddr, int numThreads, int worker_nums)
    : server_(listenAddr, numThreads), worker_pool_(worker_nums, "worker")
{
    server_.SetOnMessageCallback([this](ConnectionPtr conn, std::string buf)
                                 { HandleOnMessage(conn, buf); });
    server_.SetOnConnectedCallback([this](ConnectionPtr conn)
                                   { HandleOnConnected(conn); });
    server_.SetOnSentCallback([this](ConnectionPtr conn)
                              { HandleOnSent(conn); });
    server_.SetOnClosedCallback([this](ConnectionPtr conn)
                                { HandleOnClosed(conn); });
}

void EchoServer::HandleOnMessage(ConnectionPtr conn, const std::string &buf)
{
    // printf(" %ld bytes received: %s\n", buf.size(), buf.c_str());
    std::cout << "[" << TimeStamp::NowToString() << "]" << buf << std::endl;
    if (worker_pool_.Size() == 0)
    {
        OnMessage(conn, buf);
    }
    else
    {
        worker_pool_.AddTask([this, conn, buf]()
                             { OnMessage(conn, buf); });
    }
}

void EchoServer::OnMessage(ConnectionPtr conn, const std::string &buf)
{
    conn->Send(buf);
}

void EchoServer::HandleOnConnected(ConnectionPtr conn)
{
    std::cout << conn->peerAddress().ToIpPort() << "已连接" << std::endl;
}

void EchoServer::HandleOnClosed(ConnectionPtr conn)
{
    // std::cout << conn->peerAddress().ToIpPort() << "已断开" << std::endl;
}

void EchoServer::HandleOnSent(ConnectionPtr conn)
{
    // std::cout << conn->peerAddress().ToIpPort() << "已发送完成: " << "thread" << syscall(SYS_gettid) << std::endl;
}

void EchoServer::start()
{
    server_.Run();
}

void EchoServer::Stop()
{
    // 停止工作线程
    worker_pool_.Stop();
    std::cout << "工作线程停止" << std::endl;
    // 停止io线程
    server_.Stop();
}
