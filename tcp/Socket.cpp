#include "Socket.h"

Socket::Socket()
	: fd_(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP))
{
	perror_if(fd_ < 0, "fd create error");
}

Socket::Socket(int fd) : fd_(fd)
{
	perror_if(fd_ < 0, "fd create error");
}

void Socket::SetReuseaddr(bool on)
{
	int opt = on ? 1 : 0;
	setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof(opt))); // 必须
}

void Socket::SetReuseport(bool on)
{
	int opt = on ? 1 : 0;
	setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, static_cast<socklen_t>(sizeof(opt)));
}

void Socket::SetTcpnodelay(bool on)
{
	int opt = on ? 1 : 0;
	setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof(opt))); // 必须
}

void Socket::SetKeepalive(bool on)
{
	int opt = on ? 1 : 0;
	setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof(opt))); // 可以自己做心跳
}

Socket::~Socket()
{
	if (fd_ != -1)
	{
		close(fd_);
		fd_ = -1;
	}
}

void Socket::Bind(InetAddress &ser_addr)
{
	// std::cout << fd_ << std::endl;
	int ret = bind(fd_, (sockaddr *)(&ser_addr.getAddr()), sizeof(sockaddr));
	perror_if(ret < 0, "bind error");
}

void Socket::Listen()
{
	int ret = listen(fd_, 128);
	perror_if(ret < 0, "listen error");
}

int Socket::Accecpt(InetAddress &cli_addr)
{
	sockaddr_in addr_in;
	socklen_t len = sizeof(addr_in);
	int cfd = accept4(fd_, (sockaddr *)&addr_in, &len, SOCK_NONBLOCK);
	if (cfd < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			perror("accept4 error");
		}
		return -1;
	}
	cli_addr.SetAddr(addr_in);
	return cfd;
}

int Socket::fd() const
{
	return fd_;
}

void Socket::SetNonblocking()
{
	fd_ = fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL, 0) | O_NONBLOCK);
	perror_if(fd_ < 0, "set nonblock error");
}
