#include <iostream>
#include "EchoServer.h"
#include "InetAddress.h"

int main(int argv, char *argc[])
{
	InetAddress serv_addr("172.21.211.240", 8888);
	EchoServer echoserver(serv_addr, 4, 3);
	echoserver.start();
	return 0;
}