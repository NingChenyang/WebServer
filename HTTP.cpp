#include "http/HttpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/HttpUtil.h"
#include <iostream>
#include <filesystem>
#include "mysql/MysqlConnPool.h"
#include <signal.h>
#include "jsoncpp/json/json.h"
#include "routes.hpp"
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
        MysqlConnPool::GetInstance()->ShutDown();
    }
    exit(0);
}

void onRequest(const HttpRequest &req, HttpResponse *resp)
{
    std::string path = req.GetPath();

    // 处理API请求
    // 修改后的处理逻辑
    if (req.GetMethod() == Method::kPost)
    {
        auto it = postRoutes.find(path);
        if (it != postRoutes.end())
        {
            it->second(req, resp);
            return;
        }
    }

    if (req.GetMethod() == Method::kGet)
    {
        if (path == "/")
        {
            path = "/login.html";
        }
        // 对home.html进行访问控制
        if (path != "/login.html"&&path.ends_with(".html"))
        {
            // 检查Cookie是否存在
            std::string cookie = req.GetHeader("Cookie");
            if (cookie.find("auth_token=valid") == std::string::npos)
            {
                // 未授权访问,重定向到登录页
                resp->SetStatusCode(HttpStatusCode::k302Found);
                resp->SetStatusMessage("Found");
                resp->AddHeader("Location", "/login.html");
                resp->SetBody("");
                return;

            }
        }

        

        std::string filepath = ROOT_DIR + path;
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
            resp->SetStatusCode(HttpStatusCode::k200Ok);
            resp->SetStatusMessage("OK");
            resp->SetContentType(GetFileType(filepath));
            resp->SetBody(std::move(content));
        }

        // 设置通用响应头
        resp->AddHeader("Server", "MyWebServer/1.0");
        if (!resp->GetCloseConnection())
        {
            // 在返回静态文件的地方添加
            resp->AddHeader("Cache-Control", "no-cache, no-store, must-revalidate");
            resp->AddHeader("Pragma", "no-cache");
            resp->AddHeader("Expires", "0");
            resp->AddHeader("Connection", "Keep-Alive");
            resp->AddHeader("Keep-Alive", "timeout=60");
        }
    }
}

int main()
{
    // 忽略SIGPIPE信号
    signal(SIGPIPE, SIG_IGN);

    // 注册SIGINT和SIGTERM的处理函数
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandler;
    sigaction(SIGINT, &sa, NULL);  // Ctrl+C
    sigaction(SIGTERM, &sa, NULL); // kill

    try
    {
        std::filesystem::current_path("./ws");
        InetAddress listenAddr("0.0.0.0", 8888); // 修改监听地址为0.0.0.0，接受所有网卡的连接
        HttpServer server(listenAddr, 1, 1);
        server.SetHttpCallback(onRequest);
        g_server = &server; // 设置全局指针

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
