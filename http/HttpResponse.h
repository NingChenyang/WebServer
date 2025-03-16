#pragma once
#include"../tcp/Buffer.h"
#include"HttpUtil.h"
#include<cstring>
#include<iostream>
class Buffer;

class HttpResponse
{
public:
    HttpResponse(Version Version,bool closeConnection);
    ~HttpResponse();
    void SetCloseConnection(bool on);
    // bool GetCloseConnection() const;
    void SetStatusCode(HttpStatusCode code);
    // HttpStatusCode GetStatusCode() const;
    void SetStatusMessage(const std::string &message);
    // const std::string &GetStatusMessage() const;
    void AddHeader(const std::string &key, const std::string &value);
    void SteBody(const std::string &body);
    void AppendToBuffer(Buffer *output) const;
private:

    Version version_;
    HttpStatusCode statusCode_;
    std::string statusMessage_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    bool closeConnection_;
};
