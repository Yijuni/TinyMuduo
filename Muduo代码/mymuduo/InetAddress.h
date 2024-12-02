#pragma once
#include <netinet/in.h>
#include <string>
//封装socket地址类型 
class InetAddress
{
public:
    InetAddress();
    ~InetAddress();
    explicit InetAddress(uint16_t port,std::string ip);
    explicit InetAddress(const sockaddr_in &addr):addr_(addr){}
    
    std::string toIp()const;
    std::string toIpPort()const;
    uint16_t toPort();
    const sockaddr_in* getSockAddr() const{return &addr_;}
    void setSockAddr(const sockaddr_in& addr){addr_=addr;}
private:
    sockaddr_in addr_;
};

