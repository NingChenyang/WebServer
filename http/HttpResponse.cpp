#include "HttpResponse.h"

HttpResponse::HttpResponse(bool closeConnection, Version version)
    : closeConnection_(closeConnection),
      version_(version)
{
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::SetCloseConnection(bool on)
{
    closeConnection_ = on;
}

bool HttpResponse::GetCloseConnection() const
{
    return closeConnection_;
}

void HttpResponse::SetStatusCode(HttpStatusCode code)
{
    statusCode_ = code;
}

void HttpResponse::SetStatusMessage(const std::string &message)
{
    statusMessage_ = message;
}

void HttpResponse::AddHeader(const std::string &key, const std::string &value)
{
    headers_[key] = value;
}

void HttpResponse::SetBody(const std::string &body)
{
    body_ = body;
}

void HttpResponse::AppendToBuffer(Buffer *output) const
{

    std::string buf = VersionToString(version_) + " " +
                      std::to_string(static_cast<int>(statusCode_)) + " " +
                      statusMessage_ + "\r\n";
    output->Append(buf);
    if (closeConnection_)
    {
        output->Append("Connection: close\r\n");
    }
    else
    {
        output->Append("Connection: Keep-Alive\r\n");
        buf = "Content-Length: " + std::to_string(body_.size()) + "\r\n";
        output->Append(buf);
    }
    for (auto it = headers_.begin(); it != headers_.end(); it++)
    {
        buf = it->first + ": " + it->second + "\r\n";
        output->Append(buf);
    }
    output->Append("\r\n");
    output->Append(body_);
}

void HttpResponse::SetContentType(const std::string &type)
{
    AddHeader("Content-Type", type);
}
