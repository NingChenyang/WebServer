#include"util.h"
#include<stdio.h>
#include<stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include<string.h>
#include <cassert>

void sockets::SetNonblock(int sockfd)
{
	int flag = fcntl(sockfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(sockfd, F_SETFL, flag);
}

void sockets::ShutdownWrite(int sockfd)
{
	if (::shutdown(sockfd, SHUT_WR) < 0) {
		printf("sockets::shutdownWrite error\n");
	}
}



int sockets::GetSocketError(int sockfd)
{
	int optval;
	socklen_t optlen = static_cast<socklen_t>(sizeof optval);

	if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
	{
		return errno;
	}
	else
	{
		return optval;
	}
}


struct sockaddr_in sockets::GetLocalAddr(int sockfd)
{
	struct sockaddr_in localaddr;
	memset(&localaddr, 0, sizeof(localaddr));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
	if (::getsockname(sockfd, (struct sockaddr*)&localaddr, &addrlen) < 0)
	{
		printf("sockets::getLocalAddr\n");
	}
	return localaddr;
}

struct sockaddr_in sockets::GetPeerAddr(int sockfd)
{
	struct sockaddr_in peeraddr;
	memset(&peeraddr, 0, sizeof(peeraddr));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
	if (::getpeername(sockfd, (struct sockaddr*)&peeraddr, &addrlen) < 0)
	{
		printf("sockets::getPeerAddr\n");
	}
	return peeraddr;
}

std::string ProcessInfo::hostname()
{
	// HOST_NAME_MAX 64
	// _POSIX_HOST_NAME_MAX 255
	char buf[256];
	if (::gethostname(buf, sizeof buf) == 0)
	{
		buf[sizeof(buf) - 1] = '\0';
		return buf;
	}
	else
	{
		return "unknownhost";
	}
}

pid_t ProcessInfo::pid()
{
	return ::getpid();
}

void perror_if(bool condtion, const char* errorMessage)
{

	if (condtion) {
		perror(errorMessage);
		assert(!condtion);
		exit(1);
	}
	
}