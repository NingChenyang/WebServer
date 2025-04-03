#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include "../tcp/Server.h"
#include "../mysql/MysqlConnPool.h"
#include"WebSocketPacket.h"
#include"WebSocketContext.h"
#include"../http/HttpContext.h"
#include"Room.h"
class WebSocketContext;
class HttpRequest;
class HttpResponse;
class WebSocketServer
{
public:
    // 在回调函数中，目前还没想到WebsocketPacket类的用处
    // using WebsocketCallback = std::function<void(const Buffer*, Buffer*, WebsocketPacket& respondPacket)>;
    using WebsocketCallback = std::function<void(const Buffer *, Buffer *, const ConnectionPtr &)>;
    WebSocketServer(InetAddress &serv_addr, int io_thread_nums, int Worker_thread_nums);
    ~WebSocketServer();
    void Start(const std::string &ip, uint16_t port);
    void Stop();
    void SetWebsocketCallback(WebsocketCallback callback);


    //聊天室相关
    void JoinRoom(Json::Value mesg,Buffer *output ,const ConnectionPtr &conn);
    void LeaveRoom(Json::Value mesg, Buffer *output, const ConnectionPtr &conn);
    void BroadcastMessage(const std::string &room_name, const Json::Value mesg);
    void ChatMessage(Json::Value mesg, Buffer *output, const ConnectionPtr &conn);
    

private:
void HandleNewConnection(const ConnectionPtr &conn);
    void HandleMessage(const ConnectionPtr &conn, Buffer *buf);
    void OnMessage(const ConnectionPtr &conn, Buffer *buf);
    void HandleData(const ConnectionPtr &conn, WebSocketContext *context, Buffer *buf);
    void HandleSent(const ConnectionPtr &conn);
    void HandleClose(const ConnectionPtr &conn);
    Server server_;
    WebsocketCallback websocket_callback_;
    ThreadPool worker_pool_;

    // 房间列表
    std::mutex rooms_mutex_;
    std::unordered_map<std::string, std::shared_ptr<Room>> rooms_;
};
