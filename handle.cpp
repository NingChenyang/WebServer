#include "handle.h"
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
                resp->SetContentType("application/json");
                resp->AddHeader("Set-Cookie", "auth_token=valid; Path=/; HttpOnly; SameSite=Strict; Max-Age=3600");
                resp->SetBody(response.toStyledString());
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
void handleGetChatRooms(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        // 从查询参数获取用户名
        auto params = ParseQueryParams(req.GetQuery());
        auto it = params.find("username");
        if (it == params.end())
        {
            throw std::runtime_error("需要提供用户名参数");
        }
        std::string username = it->second;
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
        response["data"] = rooms;  // ✅ 包裹标准响应格式
                                   // 最优输出方案
        Json::StreamWriterBuilder writer;
        // writer["indentation"] = "";                              // 紧凑格式（去掉换行和缩进）
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

void handleGetMessages(const HttpRequest &req, HttpResponse *resp)
{
    // std::string room = req.GetQuery("room");
    // // 这里根据room从数据库获取消息
    // Json::Value messages;
    // // 示例数据 - 实际应从数据库查询

    // resp->SetStatusCode(HttpStatusCode::k200Ok);
    // resp->SetContentType("application/json");
    // resp->SetBody(Json::FastWriter().write(messages));
}