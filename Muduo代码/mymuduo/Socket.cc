#include "Socket.h"
#include <unistd.h>
#include "Logger.h"
#include <InetAddress.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
Socket::Socket(int sockfd):sockfd_(sockfd)
{
}

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if(::bind(sockfd_,(sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in))<0){
        LOG_FATAL("bind socketfd:%d failed\n",sockfd_);
    }
}

void Socket::listen()
{
    //::是全局作用域，防止和自己定义的产生冲突
    if(::listen(sockfd_,1024)!=0){//最大监听1024个链接
        LOG_FATAL("listen socketfd:%d failed\n",sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr){
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr,len);
    int confd = ::accept(sockfd_,(sockaddr*)&addr,&len);
    if(confd>=0){
        peeraddr->setSockAddr(addr);//设置网络地址的sockaddr_in
    }
    return confd;
}

void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_,SHUT_WR)<0){
        LOG_ERROR("sockets::shutdown error\n");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on?1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof optval);
}

void Socket::setRefuseAddr(bool on)
{
    int optval = on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof optval);
}

void Socket::setRefusePort(bool on)
{
    int optval = on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof optval);
}

void Socket::setKeepAlive(bool on)
{
    int optval = on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof optval);
}
