#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#define MAX_EVENTS 128
#include <cstring>
#include <unistd.h>
// std::string TimeToString()
// {
//     char time_[32] = {0};
//     tm *tm = localtime(time(0));
//     snprintf(time_, 32, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
//     return std::string(time_);
// }
void setnonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << "fcntl F_GETFL";
        return;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        std::cerr << "fcntl F_SETFL";
        return;
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        std::cout << "请输入ip和端口" << std::endl;
        return -1;
    }
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd < 0)
    {
        std::cerr << "socket";
        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_family = AF_INET;

    if (connect(cfd, (sockaddr *)&server_addr, sizeof server_addr) < 0)
    {
        std::cerr << "connect";
    }
    std::cout << "connect successed" << std::endl;
    char buf[1024] = {0};
    char tmpbuf[1024] = {0};
   std:: cout<<"开始时间" << time(0)<<std::endl;
    for (int i = 0; i < 100000; i++)
    {
        bzero(buf, 1024);
        // bzero(tmpbuf, 1024);
        sprintf(buf, "这是第%d个消息", i);
        int len = strlen(buf);
        // memcpy(tmpbuf, &len, 4);
        // memcpy(tmpbuf, buf, len);
       // std::cout << "send:" << buf << std::endl;
       if( send(cfd, buf, len, 0)==-1)
       std::cout<<"send error"<<std::endl;
        bzero(buf, 1024);
        // recv(cfd, &len, 4, 0);
        if(recv(cfd, buf, 1024, 0)==-1)
        std::cout<<"recv error"<<std::endl;
        // perror("recv");
        // std::cout << "recv:" << buf << std::endl;
    }
    
   std::cout<<"结束时间" << time(0) << std::endl;

    // for (int i = 0; i < 10000; i++)
    // {
    //     bzero(buf, 1024);
    //     int len=0;
    //     recv(cfd,&len,4,0);
    //     recv(cfd, buf, len, 0);
    //     std::cout << "recv:" << buf << std::endl;
    // }
    return 0;
}
