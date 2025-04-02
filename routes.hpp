// 在文件顶部定义路由表
#include <unordered_map>
#include <functional>
#include <string>
class HttpRequest;
class HttpResponse;
#include "handle.h"

static const std::unordered_map<std::string, std::function<void(const HttpRequest &, HttpResponse *)>> postRoutes = {
    {"/api/login", handleLoginRequest},
    {"/api/register", HandleRegisterRequest},
    {"/api/logout", HandleLogoutRequest}};

