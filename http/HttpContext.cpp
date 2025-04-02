#include "HttpContext.h"

HttpContext::HttpContext()
    : state_(HttpRequestPaseState::kExpectRequestLine)
{
}

HttpContext::~HttpContext()
{
}

bool HttpContext::ParseRequest(Buffer *buf, TimeStamp eceiveTime)
{
    bool ok = true;
    bool hasMore = true;
    while (hasMore)
    {
        if (state_ == HttpRequestPaseState::kExpectRequestLine)
        {
            const char *crlf = buf->FindCRLF();
            if (crlf)
            {
                ok = processRequestLine(buf->Peek(), crlf);
                if (ok)
                {
                    request_.SetReceiveTime(eceiveTime);
                    buf->RetrieveUntil(crlf + 2);
                    state_ = HttpRequestPaseState::kExpectHeaders;
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == HttpRequestPaseState::kExpectHeaders)
        {
            const char *crlf = buf->FindCRLF();
            if (crlf)
            {
                //: 后面的空格
                const char *colon = std::find(buf->Peek(), crlf, ':');
                if (colon != crlf)
                {
                    // const char* space=std::find(colon,crlf,' ');
                    // if (space!=crlf)
                    // {
                    //     colon=space;
                    // }
                    
                    
                    request_.AddHeader(buf->Peek(), colon, crlf);
                }
                else
                {
                    // state_=HttpRequestPaseState::kGotAll;
                    // hasMore=false;
                    state_ = HttpRequestPaseState::kExpectBody;
                }
                buf->RetrieveUntil(crlf + 2);
            }
            else
            {
                hasMore = false;
            }
            
        }
        else if (state_ == HttpRequestPaseState::kExpectBody)
        {
            int len = buf->ReadableBytes();
            if (len)
            {
                request_.SetQuery(buf->Peek(), buf->BeginWrite());
                buf->Retrieve(len);
            }
            state_=HttpRequestPaseState::kGotAll;
            hasMore=false;
        }

    }
    return ok;
}
    bool HttpContext::ParseRequest(Buffer * buf)
    {

        bool ok = true;
        bool hasMore = true;
        while (hasMore)
        {
            if (state_ == HttpRequestPaseState::kExpectRequestLine)
            {
                const char *crlf = buf->FindCRLF();
                if (crlf)
                {
                    ok = processRequestLine(buf->Peek(), crlf);
                    if (ok)
                    {
                        buf->RetrieveUntil(crlf + 2);
                        state_ = HttpRequestPaseState::kExpectHeaders;
                    }
                    else
                    {
                        hasMore = false;
                    }
                }
                else
                {
                    hasMore = false;
                }
            }
            else if (state_ == HttpRequestPaseState::kExpectHeaders)
            {
                const char *crlf = buf->FindCRLF();
                if (crlf)
                {
                    //: 后面的空格
                    const char *colon = std::find(buf->Peek(), crlf, ':');
                    if (colon != crlf)
                    {
                        const char *space = std::find(colon, crlf, ' ');
                        if (space != crlf)
                        {
                            colon = space;
                        }

                        request_.AddHeader(buf->Peek(), colon, crlf);
                    }
                    else
                    {
                        // state_=HttpRequestPaseState::kGotAll;
                        // hasMore=false;
                        state_ = HttpRequestPaseState::kExpectBody;
                    }
                    buf->RetrieveUntil(crlf + 2);
                }
                else
                {
                    hasMore = false;
                }
            }
            else if (state_ == HttpRequestPaseState::kExpectBody)
            {
                int len = buf->ReadableBytes();
                if (len)
                {
                    
                    request_.SetQuery(buf->Peek(), buf->BeginWrite());
                    buf->Retrieve(len);
                }
                state_ = HttpRequestPaseState::kGotAll;
                hasMore = false;
            }
        }
        // buf->RetrieveAll();
        return ok;
    }

    bool HttpContext::GotAll() const
    {
        return state_ == HttpRequestPaseState::kGotAll;
    }

    void HttpContext::Reset()
    {
        state_ = HttpRequestPaseState::kExpectRequestLine;
        HttpRequest request;
        request_.Swap(request);
    }

    const HttpRequest &HttpContext::GetRequest() const
    {
        return request_;
    }

    HttpRequest &HttpContext::GetRequest()
    {
        return request_;
    }

    bool HttpContext::processRequestLine(const char *begin, const char *end)
    {
        bool succeed = true;
        const char* space=std::find(begin,end,' ');
        if (space!=end&&request_.SetMethod(begin,space))
        {
            begin=space+1;
            space=std::find(begin,end,' ');
            if (space!=end)
            {
                const char *question = std::find(begin, space, '?');
                if (question != space)
                {
                    request_.SetPath(begin, question);
                    request_.SetQuery(question+1, space);
                }
                else
                {
                    request_.SetPath(begin, space);
                }
                begin = space + 1;
                std::string version(begin, end);
                // printf("version=%s\n", version.data());
                if (version == "HTTP/1.0")
                    request_.SetVersion(Version::kHttp10);
                else if (version == "HTTP/1.1")
                    request_.SetVersion(Version::kHttp11);
                else
                    succeed = false;
            }
        }
        return succeed;
    }
