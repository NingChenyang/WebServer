#pragma once
#include"HttpUtil.h"
#include"HttpRequest.h"
#include"../tcp/Buffer.h"
#include"../tcp/TimeStamp.h"
class HttpContext
{
public:

    HttpContext();
    ~HttpContext();
    bool ParseRequest(Buffer *buf, TimeStamp eceiveTime);
    bool ParseRequest(Buffer *buf);
    bool GotAll() const;
    void Reset();
    const HttpRequest &GetRequest() const;
    HttpRequest &GetRequest();
private:
bool processRequestLine(const char *begin,const char *end);

    HttpRequestPaseState state_;
    HttpRequest request_;
};
