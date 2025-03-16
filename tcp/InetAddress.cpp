#include "InetAddress.h"

InetAddress::InetAddress()
{
}

InetAddress::InetAddress(sockaddr_in& addr)
    :addr_(addr)
{
}

InetAddress::InetAddress(const char* ip, unsigned short port)
{
    std::memset(&addr_, 0, sizeof(addr_));
    if (ip==nullptr)
    {
        addr_.sin_addr.s_addr = inet_addr(INADDR_ANY);
    }
    else
    {
        addr_.sin_addr.s_addr = inet_addr(ip);
    }
    addr_.sin_port = htons(port);
    addr_.sin_family = AF_INET;
}

const sockaddr_in& InetAddress::getAddr()
{
    return addr_;
    // TODO: 在此处插入 return 语句
}

void InetAddress::SetAddr(sockaddr_in& addr)
{
    std::memset(&addr_, 0, sizeof(addr_));
    addr_ = addr;
}

std::string InetAddress::ToIp() const
{
    return std::string(inet_ntoa(addr_.sin_addr));
}

uint16_t InetAddress::ToPort() const
{
    return ntohs(addr_.sin_port);
}

std::string InetAddress::ToIpPort() const
{
    std::string str = inet_ntoa(addr_.sin_addr) + std::string(":") + std::to_string(ntohs(addr_.sin_port));
    return str;
}
