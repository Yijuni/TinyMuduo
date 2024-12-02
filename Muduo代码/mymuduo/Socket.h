#pragma once
#include "nocopyable.h"
class InetAddress;

class Socket:nocopyable{
public:
    explicit Socket(int sockfd);
    ~Socket();
    int fd() const {return sockfd_;}
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);//接受对端连接请求并保存对端地址

    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setRefuseAddr(bool on);
    void setRefusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};