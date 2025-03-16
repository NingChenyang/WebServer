#pragma once
#include "../tcp/TimeStamp.h"
#include <string>
#include <vector>
#include <unordered_map>
#include"HttpUtil.h"

class HttpRequest
{
public:
    
    HttpRequest();
    ~HttpRequest();
    void Swap(HttpRequest &rhs);
    bool SetMethod(const char *start, const char *end);
    Method GetMethod() const;
    const char *MethodToString() const;
    void SetVersion(Version version);
    Version GetVersion() const;
    void SetPath(const char *start, const char *end);
    const std::string &GetPath() const;
    void AddHeader(const char *start, const char *colon, const char *end);
    std::string GetHeader(const std::string &field) const;
    const std::unordered_map<std::string, std::string> Headers() const;
    void SetQuery(const char *start, const char *end);
    const std::string &GetQuery() const;
    void SetReceiveTime(TimeStamp t);
    TimeStamp GetReceiveTime() const;

private:
    // 请求方法
    Method method_;
    // 请求版本
    Version version_;
    // 请求路径
    std::string path_;
    // 请求头部
    std::unordered_map<std::string, std::string> headers_;
    // 请求体body
    std::string query_;

    TimeStamp receiveTime_;
};
