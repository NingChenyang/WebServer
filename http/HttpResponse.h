#pragma once
#include "../tcp/Buffer.h"
#include "HttpUtil.h"
#include <cstring>
#include <iostream>
class Buffer;

class HttpResponse
{
public:
    HttpResponse(bool closeConnection, Version Version = Version::kHttp11);
    ~HttpResponse();
    void SetCloseConnection(bool on);
    bool GetCloseConnection() const;
    void SetStatusCode(HttpStatusCode code);
    // HttpStatusCode GetStatusCode() const;
    void SetStatusMessage(const std::string &message);
    // const std::string &GetStatusMessage() const;
    void AddHeader(const std::string &key, const std::string &value);
    void SetBody(const std::string &body);
    void AppendToBuffer(Buffer *output) const;
    void SetContentType(const std::string &type);

private:
    bool closeConnection_;
    Version version_;
    HttpStatusCode statusCode_;
    std::string statusMessage_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    
};
