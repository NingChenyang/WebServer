#pragma once
#include "../tcp/Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <functional>
#include "HttpContext.h"
#include "../tcp/ThreadPool.h"
#include "../log/Logger.h"
class HttpServer
{

public:
    using HttpCallback = std::function<void(const HttpRequest &, HttpResponse *)>;
    HttpServer(InetAddress &listenAddr, int numThreads, int worker_nums);
    ~HttpServer();
    void Start();
    void SetHttpCallback(const HttpCallback &cb);
    // 无用
    EventLoop *GetLoop();

private:
    void HandleOnConnection(const ConnectionPtr &conn);
    void HandleOnMessage(const ConnectionPtr &conn, Buffer *buf);
    // void OnMessage(const ConnectionPtr &conn, Buffer *buf, TimeStamp receiveTime);
    void OnRequest(const ConnectionPtr &conn, const HttpRequest &req);
    void HandleOnWriteComplete(const ConnectionPtr &conn);
    Server server_;
    ThreadPool workers_pool_;
    HttpCallback http_callback_;
};
