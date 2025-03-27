#include "http/HttpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/HttpUtil.h"
#include <iostream>
#include <filesystem>
#include <signal.h>

extern const std::string ROOT_DIR = ".";                   // 定义网站根目录
std::string Logger::log_file_basename_ = "../logs/server"; // 修改默认日志路径

// 全局指针,用于信号处理函数访问
HttpServer *g_server = nullptr;

// 信号处理函数
void SignalHandler(int sig)
{
    // LOG_INFO << "Received signal " << sig << ", stopping server...";
    if (g_server)
    {
        g_server->Stop();
    }
    exit(0);
}

void onRequest(const HttpRequest &req, HttpResponse *resp)
{
    std::string path = req.GetPath();
    // LOG_INFO << "Handling request for path: " << path;

    if (path == "/")
    {
        path = "/index.html";
    }

    std::string filepath = ROOT_DIR + path;
    // LOG_INFO << "Full filepath: " << filepath;

    std::string content = ReadFile(filepath);
    if (content.empty())
    {
        LOG_WARN << "File not found: " << filepath;
        resp->SetStatusCode(HttpStatusCode::k404NotFound);
        resp->SetStatusMessage("Not Found");
        resp->SetContentType("text/html");
        resp->SetBody("<html><body><h1>404 Not Found</h1></body></html>");
    }
    else
    {
        // LOG_INFO << "File found, size=" << (int)content.size();
        resp->SetStatusCode(HttpStatusCode::k200Ok);
        resp->SetStatusMessage("OK");
        resp->SetContentType(GetFileType(filepath));
        resp->SetBody(std::move(content));
    }

    // 设置通用响应头
    resp->AddHeader("Server", "MyWebServer/1.0");
    if (!resp->GetCloseConnection())
    {
        resp->AddHeader("Connection", "Keep-Alive");
        resp->AddHeader("Keep-Alive", "timeout=60");
    }
}

int main()
{
    // 忽略SIGPIPE信号
    // signal(SIGPIPE, SIG_IGN);

    // 注册SIGINT和SIGTERM的处理函数
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandler;
    sigaction(SIGINT, &sa, NULL);  // Ctrl+C
    sigaction(SIGTERM, &sa, NULL); // kill

    std::filesystem::current_path("./www");

    try
    {
        // 修改监听地址为0.0.0.0，接受所有网卡的连接
        InetAddress listenAddr("0.0.0.0", 8888);

        HttpServer server(listenAddr, 3, 4);
        g_server = &server; // 设置全局指针

        server.SetHttpCallback(onRequest);

        std::cout << "HTTP server starting on http://0.0.0.0:8888" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        server.Start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
