#pragma once

#include "Socket.h"
#include "nocopyable.h"
#include "Channel.h"
#include <functional>

class EventLoop;
class InetAddress;

class Acceptor:nocopyable{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
    ~Acceptor();
    void setNewConnctionCallback(const NewConnectionCallback &cb){newConnectionCallback_ = cb;}
    bool listenning(){return listenning_;}
    void listen();
private:
    void handleRead();
    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;//这个回调函数由TcpServer给出,新用户连接进来需要执行什么任务
    bool listenning_;


};