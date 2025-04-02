#include "handle.h"
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