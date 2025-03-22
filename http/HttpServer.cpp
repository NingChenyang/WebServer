#include "HttpServer.h"
#include <any>
// 默认的回调函数
void defaultHttpCallback(const HttpRequest &req, HttpResponse *resp)
{

    resp->SetStatusCode(HttpStatusCode::k404NotFound);
    resp->SetStatusMessage("Not Found");
    resp->SetCloseConnection(true);
    req.GetVersion();
}
HttpServer::HttpServer(InetAddress &listenAddr, int numThreads, int worker_nums)
    : server_(listenAddr, numThreads),
      workers_pool_(worker_nums, "worker"), http_callback_([](const HttpRequest &req, HttpResponse *resp)
                                                           { defaultHttpCallback(req, resp); })
{
    server_.SetOnConnectedCallback(std::bind(&HttpServer::HandleOnConnection, this, std::placeholders::_1));
    server_.SetOnMessageCallback(std::bind(&HttpServer::HandleOnMessage, this, std::placeholders::_1, std::placeholders::_2));
    server_.SetOnSentCallback(std::bind(&HttpServer::HandleOnWriteComplete, this, std::placeholders::_1));
}

HttpServer::~HttpServer()
{
}

void HttpServer::Start()
{
    server_.Run();
}

void HttpServer::SetHttpCallback(const HttpCallback &cb)
{
    http_callback_ = cb;
}

EventLoop *HttpServer::GetLoop()
{
    return nullptr;
}

void HttpServer::HandleOnConnection(const ConnectionPtr &conn)
{
    if (conn->Connected())
    {
        LOG_INFO << "New connection established";
        try
        {
            conn->SetContext(HttpContext());
            LOG_INFO << "HttpContext created successfully";
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Failed to create HttpContext: " << e.what();
        }
    }
}

void HttpServer::HandleOnMessage(const ConnectionPtr &conn, Buffer *buf)
{

    OnMessage(conn, buf);
}

void HttpServer::OnMessage(const ConnectionPtr &conn, Buffer *buf)
{
    HttpContext *context = nullptr;
    try
    {
        size_t size = buf->ReadableBytes();
        LOG_INFO << "Received message from " << conn->peerAddress().ToIpPort()
                 << ", size=" << size;

        if (size == 0)
        {
            LOG_WARN << "Empty message received, ignoring";
            return;
        }

        LOG_INFO << "Raw message:\n"
                 << std::string(buf->Peek(), size);

        LOG_INFO << "Received message, size=" << (int)(buf->ReadableBytes());
        LOG_INFO << "Message content: \n"
                 << std::string(buf->Peek(), buf->ReadableBytes());

        auto &ctx = std::any_cast<HttpContext &>(conn->GetMutableContext());
        context = &ctx;

        if (!context->ParseRequest(buf))
        {
            LOG_ERROR << "Failed to parse request";
            conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->ShutDown();
            return;
        }

        if (context->GotAll())
        {
            LOG_INFO << "Got complete request, path=" << context->GetRequest().GetPath();
            OnRequest(conn, context->GetRequest());
            context->Reset();
        }
    }
    catch (const std::bad_any_cast &e)
    {
        LOG_ERROR << "Context cast failed: " << e.what();
        try
        {
            conn->SetContext(HttpContext());
            auto &ctx = std::any_cast<HttpContext &>(conn->GetMutableContext());
            context = &ctx;

            // 重试解析请求
            if (!context->ParseRequest(buf))
            {
                conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
                conn->ShutDown();
                return;
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Failed to create context: " << e.what();
            conn->Send("HTTP/1.1 500 Internal Server Error\r\n\r\n");
            conn->ShutDown();
        }
    }
}

void HttpServer::OnRequest(const ConnectionPtr &conn, const HttpRequest &req)
{
    if (!conn || !conn->Connected())
    {
        LOG_WARN << "Connection already closed, skip sending response";
        return;
    }

    const std::string &connection = req.GetHeader("Connection");
    bool close = connection == "close" || (req.GetVersion() == Version::kHttp10 && connection != "Keep-Alive");

    HttpResponse resp(close);
    http_callback_(req, &resp);
    Buffer buf;
    resp.AppendToBuffer(&buf);

    LOG_INFO << "Sending response, size=" << (int)buf.ReadableBytes();
    LOG_INFO << "Response content: \n"
             << std::string(buf.Peek(), buf.ReadableBytes());

    // 发送前再次检查连接状态
    if (conn->Connected())
    {
        conn->Send(&buf);
    }
    else
    {
        LOG_WARN << "Connection closed before sending response";
    }

    if (resp.GetCloseConnection())
    {
        LOG_INFO << "Closing connection after response";
        conn->ShutDown();
    }
}

void HttpServer::HandleOnWriteComplete(const ConnectionPtr &conn)
{
    // 这里可以进行一些清理工作
    LOG_INFO << conn->peerAddress().ToIpPort() << " write complete";
}
