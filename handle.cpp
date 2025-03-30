#include "handle.h"
void handleLoginRequest(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        // 解析请求体
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(req.GetQuery(), root))
        {
            throw std::runtime_error("Invalid JSON format");
        }

        std::string username = root["username"].asString();
        std::string password = root["password"].asString();

        // 这里添加实际的登录验证逻辑
        bool loginSuccess = false;
        std::string message = "登录失败";

        // 示例验证（请替换为实际的验证逻辑）
        MysqlConnPool *pool = MysqlConnPool::GetInstance();
        auto conn = pool->GetConn();
        if (conn)
        {
            std::string sql = "SELECT * FROM user WHERE username='" + username + "' AND password='" + password + "'";
            auto result = conn->Query(sql);
            if (result)
            {
                loginSuccess = true;
                message = "登录成功";
            }
            else
            {
                loginSuccess = false;
                message = "用户名或密码错误";
            }
        }
        else
        {
            message = "数据库连接失败";
        }

        // resp->SetStatusCode(HttpStatusCode::k200Ok);
        // resp->SetStatusMessage("OK");
        // resp->SetContentType("application/json");
        // resp->SetBody(response.toStyledString());
        if (loginSuccess)
        {
            // 登录成功返回302重定向
            resp->SetStatusCode(HttpStatusCode::k302Found);
            resp->SetStatusMessage("Found");
            resp->AddHeader("Location", "/home.html");
            resp->SetBody(""); // 重定向响应可以不需要响应体
        }
        else
        {
            // 登录失败返回JSON响应
            Json::Value response;
            response["success"] = false;
            response["message"] = message;

            resp->SetStatusCode(HttpStatusCode::k200Ok);
            resp->SetStatusMessage("OK");
            resp->SetContentType("application/json");
            resp->SetBody(response.toStyledString());
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
        // 解析 JSON 请求体
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(req.GetQuery(), root))
        {
            throw std::runtime_error("无效的JSON格式");
        }

        std::string username = root["username"].asString();
        std::string email = root["email"].asString();
        std::string password = root["password"].asString();

        // 数据库连接
        MysqlConnPool *pool = MysqlConnPool::GetInstance();
        auto conn = pool->GetConn();

        if (!conn)
        {
            throw std::runtime_error("数据库连接失败");
        }

        // 检查用户名是否已存在
        std::string checkSql = "SELECT id FROM user WHERE username='" + username + "'";
        conn->Query(checkSql);
        conn->Next();
        if (conn->Value(0) != "")
        {
            throw std::runtime_error("用户名已存在");
        }
            // 插入新用户
            std::string insertSql = "INSERT INTO user (username, password) VALUES ('" + username + "', '" + password + "')";

        if (!conn->Update(insertSql))
        {
            throw std::runtime_error("注册失败，请重试");
        }

        // 注册成功响应
        resp->SetStatusCode(HttpStatusCode::k302Found);
        resp->SetStatusMessage("Found");
        resp->AddHeader("Location", "/index.html");
        resp->SetBody(""); // 清空响应体
    }
    catch (const std::exception &e)
    {
        // 错误处理
        Json::Value response;
        response["success"] = false;
        response["message"] = e.what();
        std::cout<<"error: "<<e.what()<<std::endl;
        resp->SetStatusCode(HttpStatusCode::k400BadRequest);
        resp->SetStatusMessage("Bad Request");
        resp->SetContentType("application/json");
        resp->SetBody(response.toStyledString());
    }
}