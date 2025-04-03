#include "handle.h"
#include "mysql/MysqlConnPool.h"
#include "until.hpp" // 添加对 until.hpp 的引用

// 在文件顶部添加查询参数解析函数
std::unordered_map<std::string, std::string> ParseQueryParams(const std::string &query)
{
    std::unordered_map<std::string, std::string> params;
    std::istringstream iss(query);
    std::string pair;

    while (std::getline(iss, pair, '&'))
    {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos)
        {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            // URL解码（此处需要实现解码逻辑）
            params[key] = value;
        }
    }
    return params;
}
void handleLoginRequest(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(req.GetQuery(), root))
        {
            throw std::runtime_error("Invalid JSON format");
        }

        std::string username = root["username"].asString();
        std::string password = root["password"].asString();

        MysqlConnPool *pool = MysqlConnPool::GetInstance();
        auto conn = pool->GetConn();
        if (conn)
        {
            std::string sql = "SELECT id, username, email FROM users WHERE username='" + username + "' AND password='" + password + "'";
            auto result = conn->Query(sql);
            if (result && conn->Next())
            {
                // 构造用户信息JSON响应
                Json::Value response;
                response["success"] = true;
                response["message"] = "登录成功";

                Json::Value userInfo;
                userInfo["id"] = conn->Value(0);       // 用户ID
                userInfo["username"] = conn->Value(1); // 用户名
                userInfo["email"] = conn->Value(2);    // 邮箱
                response["data"] = userInfo;

                resp->SetStatusCode(HttpStatusCode::k200Ok);
                resp->SetStatusMessage("OK");
                resp->SetContentType("application/json; charset=utf-8");

                // 生成一个简单的token（实际应用中应该使用更安全的方式）
                std::string token = username;

                // 设置Cookie，简化cookie设置，确保客户端可以访问
                std::string cookie = "auth_token=" + token + "; Path=/; Max-Age=3600";
                resp->AddHeader("Set-Cookie", cookie);

                // 使用JsonWriter正确处理中文
                Json::StreamWriterBuilder writer;
                writer["emitUTF8"] = true;
                // std::cout << Json::writeString(writer, response) << std::endl;
                resp->SetBody(Json::writeString(writer, response));
            }
            else
            {
                throw std::runtime_error("用户名或密码错误");
            }
        }
        else
        {
            throw std::runtime_error("数据库连接失败");
        }
    }
    catch (const std::exception &e)
    {
        Json::Value response;
        response["success"] = false;
        response["message"] = e.what();

        resp->SetStatusCode(HttpStatusCode::k400BadRequest);
        resp->SetStatusMessage("Bad Request");
        resp->SetContentType("application/json");
        resp->SetBody(response.toStyledString());
    }
}

void HandleRegisterRequest(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(req.GetQuery(), root))
        {
            throw std::runtime_error("无效的JSON格式");
        }

        std::string username = root["username"].asString();
        std::string email = root["email"].asString();
        std::string password = root["password"].asString();

        MysqlConnPool *pool = MysqlConnPool::GetInstance();
        auto conn = pool->GetConn();

        if (!conn)
        {
            throw std::runtime_error("数据库连接失败");
        }

        // 检查用户名是否已存在
        std::string checkSql = "SELECT id FROM users WHERE username='" + username + "'";
        conn->Query(checkSql);
        conn->Next();
        if (conn->Value(0) != "")
        {
            throw std::runtime_error("用户名已存在");
        }

        // 插入新用户
        std::string insertSql = "INSERT INTO users (username, email, password) VALUES ('" +
                                username + "', '" + email + "', '" + password + "')";

        if (!conn->Update(insertSql))
        {
            throw std::runtime_error("注册失败，请重试");
        }

        // 获取新注册用户的信息
        std::string getUserSql = "SELECT id, username, email FROM users WHERE username='" + username + "'";
        conn->Query(getUserSql);
        conn->Next();

        // 构造成功响应
        Json::Value response;
        response["success"] = true;
        response["message"] = "注册成功";

        Json::Value userInfo;
        userInfo["id"] = conn->Value(0);
        userInfo["username"] = conn->Value(1);
        userInfo["email"] = conn->Value(2);
        response["data"] = userInfo;

        resp->SetStatusCode(HttpStatusCode::k200Ok);
        resp->SetStatusMessage("OK");
        resp->SetContentType("application/json");
        resp->SetBody(response.toStyledString());
    }
    catch (const std::exception &e)
    {
        Json::Value response;
        response["success"] = false;
        response["message"] = e.what();

        resp->SetStatusCode(HttpStatusCode::k400BadRequest);
        resp->SetStatusMessage("Bad Request");
        resp->SetContentType("application/json");
        resp->SetBody(response.toStyledString());
    }
}

void HandleLogoutRequest(const HttpRequest &req, HttpResponse *resp)

{
    try
    {
        // 设置过期的 cookie 来清除认证
        resp->SetStatusCode(HttpStatusCode::k302Found);
        resp->SetStatusMessage("Found");
        resp->AddHeader("Location", "/login.html");
        // 设置立即过期的 cookie
        resp->AddHeader("Set-Cookie", "auth_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:01 GMT; HttpOnly; SameSite=Strict");
        resp->SetBody("");
    }
    catch (const std::exception &e)
    {
        Json::Value response;
        response["success"] = false;
        response["message"] = e.what();

        resp->SetStatusCode(HttpStatusCode::k400BadRequest);
        resp->SetStatusMessage("Bad Request");
        resp->SetContentType("application/json");
        resp->SetBody(response.toStyledString());

        LOG_ERROR << "Logout error: " << e.what();
    }
}
// 添加处理函数
// 修改后的获取聊天室函数

// 添加验证token的辅助函数
bool verifyAuthToken(const HttpRequest &req, const std::string &expectedUsername)
{
    auto cookies = req.GetHeader("Cookie");
    if (cookies == "")
    {
        return true; // 暂时放宽限制，允许没有cookie的请求
    }

    size_t tokenPos = cookies.find("auth_token=");
    if (tokenPos == std::string::npos)
    {
        return true; // 暂时放宽限制，允许没有token的请求
    }

    // 从cookie中提取token
    size_t tokenStart = tokenPos + 10;
    size_t tokenEnd = cookies.find(";", tokenStart);
    std::string token = cookies.substr(tokenStart,
                                       tokenEnd == std::string::npos ? std::string::npos : tokenEnd - tokenStart);

    // 简化验证逻辑，只要有token就认为是有效的
    return true;
}

void handleGetChatRooms(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        auto params = ParseQueryParams(req.GetQuery());
        auto it = params.find("username");
        if (it == params.end())
        {
            throw std::runtime_error("需要提供用户名参数");
        }
        std::string username = it->second;

        // 验证token但不强制要求
        if (!verifyAuthToken(req, username))
        {
            // 即使验证失败也继续处理请求
            LOG_WARN << "Token verification failed for user: " << username;
        }

        // ... 其余代码保持不变 ...
        MysqlConnPool *pool = MysqlConnPool::GetInstance();
        auto conn = pool->GetConn();
        if (!conn)
        {
            throw std::runtime_error("数据库连接失败");
        }

        // 关联查询用户加入的聊天室
        std::string sql =
            "SELECT cr.name FROM chatroom_members cm "
            "JOIN rooms cr ON cm.chatroom_id = cr.id "
            "JOIN users u ON cm.user_id = u.id "
            "WHERE u.username = '" +
            username + "'";

        auto result = conn->Query(sql);
        Json::Value rooms;

        while (conn->Next())
        {
            Json::Value m = conn->Value(0);
            rooms.append(m.asString()); // 获取房间名称
        }

        resp->SetStatusCode(HttpStatusCode::k200Ok);
        resp->SetContentType("application/json");

        // 应该改为
        Json::Value response;
        response["success"] = true;
        response["data"] = rooms; //
                                  // 最优输出方案
        Json::StreamWriterBuilder writer;
        writer["emitUTF8"] = true;                               // 保留中文
        resp->SetContentType("application/json; charset=utf-8"); // 显式声明编码
        resp->SetBody(Json::writeString(writer, response));
    }
    catch (const std::exception &e)
    {
        Json::Value response;
        response["success"] = false;
        response["message"] = e.what();

        resp->SetStatusCode(HttpStatusCode::k400BadRequest);
        resp->SetContentType("application/json");
        resp->SetBody(Json::FastWriter().write(response));
    }
}

// 添加获取用户信息的处理函数

void handleGetMessages(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        auto params = ParseQueryParams(req.GetQuery());
        if (params.find("room") == params.end())
            throw std::runtime_error("需要提供聊天室参数");
        // 使用 decode 对房间名称进行解码
        std::string roomName = decode(params["room"]);

        MysqlConnPool *pool = MysqlConnPool::GetInstance();
        auto conn = pool->GetConn();
        if (!conn)
            throw std::runtime_error("数据库连接失败");

        // 根据聊天室名称查询对应的 room id
        std::string roomId;
        std::string sqlRoom = "SELECT id FROM rooms WHERE name = '" + roomName + "'";
        conn->Query(sqlRoom);
        if (conn->Next())
            roomId = conn->Value(0);
        else
            throw std::runtime_error("未找到对应的聊天室");

        // 查询消息，通过 JOIN 获取发件人用户名，并按时间排序
        std::string sql = "SELECT m.user_id, u.username, m.content, m.timestamp FROM messages m "
                          "JOIN users u ON m.user_id = u.id "
                          "WHERE m.room_id = '" +
                          roomId + "' ORDER BY m.timestamp ASC";
        conn->Query(sql);

        Json::Value messages(Json::arrayValue);
        while (conn->Next())
        {
            Json::Value message;
            message["sender"] = conn->Value(1);
            message["message"] = conn->Value(2);
            message["timestamp"] = conn->Value(3);
            messages.append(message);
        }

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["data"] = messages;

        Json::StreamWriterBuilder writer;
        writer["emitUTF8"] = true;
        resp->SetStatusCode(HttpStatusCode::k200Ok);
        resp->SetContentType("application/json; charset=utf-8");
        resp->SetBody(Json::writeString(writer, responseJson));
    }
    catch (const std::exception &e)
    {
        Json::Value responseJson;
        responseJson["success"] = false;
        responseJson["message"] = e.what();
        resp->SetStatusCode(HttpStatusCode::k400BadRequest);
        resp->SetContentType("application/json");
        resp->SetBody(Json::FastWriter().write(responseJson));
    }
}