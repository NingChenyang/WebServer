#pragma once
#include <arpa/inet.h>
#include<string>

namespace sockets
{
	void SetNonblock(int sockfd);

	void ShutdownWrite(int sockfd);

	int GetSocketError(int sockfd);

	struct sockaddr_in GetLocalAddr(int sockfd);
	struct sockaddr_in GetPeerAddr(int sockfd);
}  // namespace sockets

namespace ProcessInfo
{
	std::string hostname();

	pid_t pid();
}
void perror_if(bool condtion, const char* errorMessage);
