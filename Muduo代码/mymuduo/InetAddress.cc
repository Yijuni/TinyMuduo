#include "InetAddress.h"
#include <arpa/inet.h>
#include <string.h>
#include <string>
InetAddress::InetAddress()
{
}

InetAddress::~InetAddress()
{
}

InetAddress::InetAddress(uint16_t port,std::string ip)
{
    memset(&addr_,0,sizeof addr_);// bzero(&addr_,sizeof(addr_));//string.h里
    addr_.sin_family = AF_INET;//指定地址族
    addr_.sin_port = htons(port);//主机字节序转网络字节序存储
    //下面语句可以写成inet_aton(ip.c_str(),&addr_.sin_addr) inet_pton(AF_INET,ip.c_str(),&addr_.sin_addr)
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());//IPV4点分十进制IP转到网络IP
}

std::string InetAddress::toIp() const
{
    //从addr_中获取IP地址返回
    char buf[64]={0};
    inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));
    return std::string(buf);
}

std::string InetAddress::toIpPort() const
{
    //返回IP:PORT
    char buf[64]={0};
    inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof buf);
    size_t ipend = strlen(buf);//获取IP字符串结尾的位置的下一个位置
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+ipend,":%u",port);//格式化输出
    return std::string(buf);
}

uint16_t InetAddress::toPort()
{
    return ntohs(addr_.sin_port);
}
// #include <iostream>
// int main(){
//     InetAddress addr(8080);
//     std::cout<<addr.toIpPort()<<std::endl;
//     return 0;
// }
