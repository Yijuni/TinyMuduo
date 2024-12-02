#pragma once
#include "nocopyable.h"
#include "EventLoop.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

/**
 * 基类poller，抽象类poller，具体的epollpoller或者pollpoller需要继承poller另外实现
 */
class Channel;

//moduo库中多路事件分发器的核心IO复用模块
class Poller:nocopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;//作为基类最好用虚析构函数

    //给所有IO复用接口保留统一的接口,派生类必须实现
    virtual Timestamp poll(int timeOutMs,ChannelList* activeChannel) = 0;
    virtual void updateChannel(Channel *channel) = 0;//更新channel在epoll_wait中的状态
    virtual void removeChannel(Channel *channel) = 0;//从poller中的map中移除
    //判断channel是否在当前poller当中
    bool hasChannel(Channel *channel) const;
    //EventLoop可以通过该接口获取默认的IO复用具体实现
    static Poller* newDefaultPoller(EventLoop *loop);
protected:
    //map的key就是socket的fd，value就是fd对应的Channel，这里保存所有的channel，不管活跃不活跃
    using ChannelMap = std::unordered_map<int,Channel*>;
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_;
};