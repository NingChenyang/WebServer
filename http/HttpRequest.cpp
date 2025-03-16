#include "HttpRequest.h"

HttpRequest::HttpRequest() : method_(Method::kInvalid),
                             version_(Version::kUnknown)
{
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::Swap(HttpRequest &rhs)
{
    std::swap(method_, rhs.method_);
    std::swap(version_, rhs.version_);
    path_.swap(rhs.path_);
    headers_.swap(rhs.headers_);
    query_.swap(rhs.query_);
}

bool HttpRequest::SetMethod(const char *start, const char *end)
{
    std::string method(start, end);
    if (method == "GET")
    {
        method_ = Method::kGet;
    }
    else if (method == "POST")
    {
        method_ = Method::kPost;
    }
    else if (method == "HEAD")
    {
        method_ = Method::kHead;
    }
    else if (method == "PUT")
    {
        method_ = Method::kPut;
    }
    else if (method == "DELETE")
    {
        method_ = Method::kDelete;
    }
    else
    {
        method_ = Method::kInvalid;
    }
    return method_ != Method::kInvalid;
}

Method HttpRequest::GetMethod() const
{
    return method_;
}

const char *HttpRequest::MethodToString() const
{
    if (method_ == Method::kGet)
    {
        return "GET";
    }
    else if (method_ == Method::kPost)
    {
        return "POST";
    }
    else if (method_ == Method::kHead)
    {
        return "HEAD";
    }
    else if (method_ == Method::kPut)
    {
        return "PUT";
    }
    else if (method_ == Method::kDelete)
    {
        return "DELETE";
    }
    else
    {
        return "INVALID";
    }
}

void HttpRequest::SetVersion(Version version)
{
    version_ = version;
}

Version HttpRequest::GetVersion() const
{
    return version_;
}

void HttpRequest::SetPath(const char *start, const char *end)
{
    path_.assign(start, end);
}

const std::string &HttpRequest::GetPath() const
{
    return path_;
}

void HttpRequest::AddHeader(const char *start, const char *colon, const char *end)
{
    std::string field(start, colon);
    ++colon;
    while (colon < end && isspace(*colon))
    {
        ++colon;
    }
    std::string value(colon, end);
    while (!value.empty() && isspace(value[value.size() - 1]))
    {
        value.resize(value.size() - 1);
    }
    headers_[field] = value;
}

std::string HttpRequest::GetHeader(const std::string &field) const
{
    auto it = headers_.find(field);
    if (it != headers_.end())
    {
        return it->second;
    }
    else
    {
        return "";
    }
}

const std::unordered_map<std::string, std::string> HttpRequest::Headers() const
{
    return headers_;
}

void HttpRequest::SetQuery(const char *start, const char *end)
{
    query_.assign(start, end);
}

const std::string &HttpRequest::GetQuery() const
{
    return query_;
}

void HttpRequest::SetReceiveTime(TimeStamp t)
{
    receiveTime_ = t;
}

TimeStamp HttpRequest::GetReceiveTime() const
{
    return receiveTime_;
}
