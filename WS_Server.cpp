#include "websocket/WebSocketServer.h"
#include "log/Logger.h"
#include <signal.h>
#include <iostream>
#include <string>
#include "websocket/Room.h"
#include "jsoncpp/json/json.h"
#include "mysql/MysqlConnPool.h"
#include    "handle.h"
// 全局指针用于信号处理
WebSocketServer *g_server = nullptr;
std::string Logger::log_file_basename_ = "./logs/ws_server"; // 修改日志路径为相对路径
// 信号处理函数
void SignalHandler(int sig)
{
    if (g_server)
    {
        std::cout << "正在停止WebSocket服务器..." << std::endl;
        g_server->Stop();
    }
    exit(0);
}

void onMessage(const Buffer *input, Buffer *output, const ConnectionPtr &conn)
{
    std::string msg(input->Peek(), input->ReadableBytes());

    // 解析JSON消息
    Json::Value jsonMsg;
    Json::Reader reader;
    if (!reader.parse(msg, jsonMsg))
    {
        LOG_ERROR << "JSON解析失败";
        return;
    }

    std::string msgType = jsonMsg["type"].asString();

    // 处理心跳消息
    if (msgType == "heartbeat")
    {
        Json::Value response;
        response["type"] = "heartbeat_ack";
        response["timestamp"] = jsonMsg["timestamp"];

        std::string responseStr = response.toStyledString();
        output->Append(responseStr.c_str(), responseStr.length());
        return;
    }

    if (msgType == "message")
    {
        std::cout<<"收到消息"<<std::endl;
        g_server->ChatMessage(jsonMsg, output,conn);
    }
    else if (msgType == "join")
    {
        std::cout<<"收到加入房间请求"<<std::endl;
        g_server->JoinRoom(jsonMsg, output,conn);//output暂时没有用。
        // handleGetMessages(jsonMsg, output);
    }
    else if (msgType == "leave")
    {
        std::cout<<"收到离开房间请求"<<std::endl;
        // g_server->LeaveRoom(jsonMsg, output);
    }
    else if (msgType == "create_room")
    {
        // g_server->CreateRoom(jsonMsg, output);
    }
    else if (msgType == "user_list")
    {
        // g_server->UserList(jsonMsg, output);
    }
    else
    {
        LOG_WARN << "未知的消息类型: " << msgType;
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
        // 创建服务器地址
        InetAddress listenAddr("0.0.0.0", 9999);

        // 创建WebSocket服务器 (IO线程数=2, 工作线程数=2)
        WebSocketServer server(listenAddr, 2, 2);
        g_server = &server;

        // // 设置连接保活参数
        // server.SetKeepAlive(true);
        // server.SetKeepAliveIdle(60);     // 60秒无数据则发送探测包
        // server.SetKeepAliveInterval(30); // 探测包发送间隔30秒
        // server.SetKeepAliveCount(3);     // 最多发送3次探测包

        // 设置消息回调函数
        server.SetWebsocketCallback(onMessage);

        std::cout << "WebSocket服务器启动在 ws://0.0.0.0:9999" << std::endl;
        std::cout << "确保logs目录存在" << std::endl;
        std::cout << "按Ctrl+C停止服务器" << std::endl;

        // 启动服务器
        server.Start("0.0.0.0", 9999);

        // 主线程等待，直到收到信号
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "服务器错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
