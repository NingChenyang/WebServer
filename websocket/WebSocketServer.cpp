#include "WebSocketServer.h"
// 默认的回调函数
void DefaultWebsocketCallback(const Buffer *buf, Buffer *sendBuf, const ConnectionPtr &conn)
{
    // 进行echo，原数据返回
    sendBuf->Append(buf->Peek(), buf->ReadableBytes());
}
WebSocketServer::WebSocketServer(InetAddress &serv_addr, int io_thread_nums, int Worker_thread_nums)
    : server_(serv_addr, Worker_thread_nums),
      websocket_callback_(DefaultWebsocketCallback),
      worker_pool_(io_thread_nums, "worker")
{
    server_.SetOnConnectedCallback(std::bind(&WebSocketServer::HandleNewConnection, this, std::placeholders::_1));
    server_.SetOnMessageCallback(std::bind(&WebSocketServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    server_.SetOnSentCallback(std::bind(&WebSocketServer::HandleSent, this, std::placeholders::_1));
    server_.SetOnClosedCallback(std::bind(&WebSocketServer::HandleClose, this, std::placeholders::_1));
    
}

WebSocketServer::~WebSocketServer()
{
}

void WebSocketServer::Start(const std::string &ip, uint16_t port)
{
    
    // 暂时的初始化房间列表
    MysqlConnPool *pool = MysqlConnPool::GetInstance();
    auto mysqlconn = pool->GetConn();
    if (!mysqlconn)
    {
        LOG_ERROR << "Failed to get MySQL connection";
    }
    else
    {
        // mysqlconn->Transaction();
        std::string sql = "SELECT id, name FROM rooms";
        auto result = mysqlconn->Query(sql);
        while (mysqlconn->Next())
        {
            int id = std::stoi(mysqlconn->Value(0));
            std::string name = mysqlconn->Value(1);
            auto room = std::make_shared<Room>(id, name);
            rooms_[name] = room;
        }
    }
    server_.Run();
}

void WebSocketServer::Stop()
{
    worker_pool_.Stop();
    server_.Stop();
}

void WebSocketServer::SetWebsocketCallback(WebsocketCallback callback)
{
    websocket_callback_ = callback;
}

void WebSocketServer::JoinRoom(Json::Value mesg, Buffer * output, const ConnectionPtr & conn)
{
    // 加入房间
    std::string room_name = mesg["room"].asString();
    std::string user_name = mesg["username"].asString();
    LOG_INFO << "JoinRoom: " << room_name << " " << user_name;
    {
        std::lock_guard<std::mutex> lock(rooms_mutex_);
        auto room = rooms_[room_name];
        if (room)
        {
            room->AddMember(conn);
        }
    }
    
}

void WebSocketServer::LeaveRoom(Json::Value mesg, Buffer *output, const ConnectionPtr &conn)
{
    // 离开房间
    std::string room_name = mesg["room"].asString();
    std::string user_name = mesg["username"].asString();
    LOG_INFO << "LeaveRoom: " << room_name << " " << user_name;
    {
        std::lock_guard<std::mutex> lock(rooms_mutex_);
        auto room = rooms_[room_name];
        if (room)
        {
            room->RemoveMember(conn);
        }
    }
}

void WebSocketServer::BroadcastMessage(const std::string &room_name, const Json::Value mesg)
{
    // 广播消息
    std::lock_guard<std::mutex> lock(rooms_mutex_);
    auto room = rooms_[room_name];
    if (room)
    {
        room->Broadcast(mesg);
    }
}

void WebSocketServer::ChatMessage(Json::Value mesg, Buffer *output, const ConnectionPtr &conn)
{
    //接收消息
    std::string room_name = mesg["room"].asString();
    std::string user_name = mesg["username"].asString();
    std::string message = mesg["content"].asString();
    std::string timestamp = mesg["timestamp"].asString();
    LOG_INFO << "ChatMessage: " << room_name << " " << user_name << " " << message;
    //存入数据库
    MysqlConnPool *pool = MysqlConnPool::GetInstance();
    auto mysqlconn = pool->GetConn();
    if (!mysqlconn)
    {
        LOG_ERROR << "Failed to get MySQL connection";
    }
    else
    {
        // 先查询用户ID
        std::string userQuery = "SELECT id FROM users WHERE username = '" + user_name + "'";
        mysqlconn->Query(userQuery);
        if (mysqlconn->Next())
        {
            int user_id = std::stoi(mysqlconn->Value(0));
            std::string sql = "INSERT INTO messages (room_id, user_id, content, timestamp) VALUES (" +
                              std::to_string(rooms_[room_name]->GetId()) + ", " +
                              std::to_string(user_id) + ", '" +
                              message + "', '" +
                              timestamp + "')";
            mysqlconn->Update(sql);
            // 广播消息
            BroadcastMessage(room_name, mesg);
        }
        else
        {
            LOG_ERROR << "User not found: " << user_name;
        }
    }

}

void WebSocketServer::HandleNewConnection(const ConnectionPtr &conn)
{
    if (conn->Connected())
    {
        LOG_INFO << "New connection established " << conn->fd() << ":" << conn->peerAddress().ToIpPort();
        try
        {
            WebSocketContext *context = new WebSocketContext();
            conn->SetContext(context); // 使用指针而不是对象
            LOG_INFO << "WebSocketContext created successfully";
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Failed to create WebSocketContext: " << e.what();
        }
    }
}

void WebSocketServer::HandleMessage(const ConnectionPtr &conn, Buffer *buf)
{
    if (worker_pool_.Size() == 0)
    {
        OnMessage(conn, buf);
    }
    else // 线程池处理
    {
        if (conn->Connected())
        {
            WebSocketContext *context = any_cast<WebSocketContext *>(conn->GetMutableContext());
            if (!context)
            {
                LOG_ERROR << "WebSocketContext is null";
                return;
            }
            if (context->GetWebSocketState() == WebSocketContext::WebSocketState::kUnconnected)
            {
                // 处理握手
                HttpContext http_context;
                if (!http_context.ParseRequest(buf))
                {
                    LOG_ERROR << "Failed to parse HTTP request";
                    conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
                    conn->Shutdown();
                    return;
                }
                if (http_context.GotAll())
                {
                    auto http_req = http_context.GetRequest();
                    LOG_INFO << "Upgrade: " << http_req.GetHeader("Upgrade");
                    LOG_INFO << "Connection: " << http_req.GetHeader("Connection");
                    LOG_INFO << "Sec-WebSocket-Version: " << http_req.GetHeader("Sec-WebSocket-Version");
                    LOG_INFO << "Sec-WebSocket-Key: " << http_req.GetHeader("Sec-WebSocket-Key");

                    // 检查Connection头是否包含Upgrade而不是完全相等
                    if (http_req.GetHeader("Upgrade").find("websocket") == std::string::npos ||
                        http_req.GetHeader("Connection").find("Upgrade") == std::string::npos ||
                        http_req.GetHeader("Sec-WebSocket-Version") != "13" ||
                        http_req.GetHeader("Sec-WebSocket-Key").empty())
                    {
                        LOG_ERROR << "Invalid WebSocket upgrade request";
                        conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
                        conn->Shutdown();
                        return;
                    }

                    Buffer hand_shake_buf;
                    context->HandleHandshake(&hand_shake_buf, http_req.GetHeader("Sec-WebSocket-Key"));
                    LOG_INFO << "Handshake response: \n"
                             << std::string(hand_shake_buf.Peek(), hand_shake_buf.ReadableBytes());
                    conn->Send(&hand_shake_buf);
                    context->SetWebSocketHandshakeState();
                    LOG_INFO << "WebSocket handshake completed for " << conn->peerAddress().ToIpPort();
                }
            }
            else
            {
                // 处理数据
                HandleData(conn, context, buf);
            }
        }
        else
        {
            LOG_WARN << "Connection is not connected";
        }
    }
}

void WebSocketServer::OnMessage(const ConnectionPtr &conn, Buffer *buf)
{
    if (conn->Connected())
    {
        WebSocketContext *context = any_cast<WebSocketContext *>(conn->GetMutableContext());
        if (!context)
        {
            LOG_ERROR << "WebSocketContext is null";
            return;
        }
        if (context->GetWebSocketState() == WebSocketContext::WebSocketState::kUnconnected)
        {
            // 处理握手逻辑同上
            HttpContext http_context;
            if (!http_context.ParseRequest(buf))
            {
                LOG_ERROR << "Failed to parse HTTP request";
                conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
                conn->Shutdown();
                return;
            }
            if (http_context.GotAll())
            {
                auto http_req = http_context.GetRequest();
                if (http_req.GetHeader("Upgrade") != "websocket" ||
                    http_req.GetHeader("Connection") != "Upgrade" ||
                    http_req.GetHeader("Sec-WebSocket-Version") != "13" ||
                    http_req.GetHeader("Sec-WebSocket-Key") == "")
                {
                    LOG_ERROR << "Invalid WebSocket upgrade request";
                    conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
                    conn->Shutdown();
                    return;
                }

                Buffer hand_shake_buf;
                context->HandleHandshake(&hand_shake_buf, http_req.GetHeader("Sec-WebSocket-Key"));
                conn->Send(&hand_shake_buf);
                context->SetWebSocketHandshakeState();
                LOG_INFO << "WebSocket handshake completed for " << conn->peerAddress().ToIpPort();
            }
        }
        else
        {
            // 处理数据
            HandleData(conn, context, buf);
        }
    }
    else
    {
        LOG_WARN << "Connection is not connected";
    }
}

void WebSocketServer::HandleData(const ConnectionPtr &conn, WebSocketContext *context, Buffer *buf)
{
    Buffer data_buf;
    context->ParseData(buf, &data_buf);
    WebSocketPacket respondPacket;
    int opcode = context->GetReqOpcode();

    switch (opcode)
    {
    case WebSocketOpcode::kContinuationFrame:
        respondPacket.SetOpcode(WebSocketOpcode::kContinuationFrame);
        break;
    case WebSocketOpcode::kTextFrame:
        respondPacket.SetOpcode(WebSocketOpcode::kTextFrame);
        break;
    case WebSocketOpcode::kBinaryFrame:
        respondPacket.SetOpcode(WebSocketOpcode::kBinaryFrame);
        break;
    case WebSocketOpcode::kCloseFrame:
        respondPacket.SetOpcode(WebSocketOpcode::kCloseFrame);
        break;
    case WebSocketOpcode::kPingFrame: // 心跳响应
        respondPacket.SetOpcode(WebSocketOpcode::kPongFrame);
        break;
    case WebSocketOpcode::kPongFrame:
        // 不回复
        break;
    default:
        LOG_INFO << "Unknown opcode: " << opcode;
        break;
    }
    Buffer send_buf;
    if (opcode != WebSocketOpcode::kCloseFrame && opcode != WebSocketOpcode::kPingFrame && opcode != WebSocketOpcode::kPongFrame)
    {
        // 处理数据
        websocket_callback_(&data_buf, &send_buf,conn);
    }
    Buffer frame_buf;
    respondPacket.EncodeFrame(&frame_buf, &send_buf);
    conn->Send(&frame_buf);
    context->Reset();
}

void WebSocketServer::HandleSent(const ConnectionPtr &conn)
{
    if (conn->Connected())
    {
        LOG_INFO << "Message sent to connection " << conn->fd() << ":" << conn->peerAddress().ToIpPort();
    }
}

void WebSocketServer::HandleClose(const ConnectionPtr &conn)
{
    if (conn->Connected())
    {
        LOG_INFO << "Connection closed " << conn->fd() << ":" << conn->peerAddress().ToIpPort();
        //断开连接自动离开房间
        {
            std::lock_guard<std::mutex> lock(rooms_mutex_);
            for (auto &room : rooms_)
            {
                room.second->RemoveMember(conn);
            }
        }
    }
}
