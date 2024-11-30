#pragma once
#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include "Timestamp.h"
#include "nocopyable.h"
#include "CurrentThread.h"
//时间循环类 主要包含两个大模块 Channel Poller (epoll抽象)
class Channel;
class Poller;

class EventLoop : nocopyable
{

public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();
    //开启事件循环
    void loop();
    //退出事件循环
    void quit();
    Timestamp pollerReturnTime() const {return pollerReturnTime_;}
    //在当前loop中执行
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在的线程执行cb
    void queueInLoop(Functor cb);

    //唤醒loop所在线程，所谓唤醒实际就是不让线程阻塞在epoll_wait上，写个数据让他继续往下执行
    void wakeUp();

    //EventLoop的方法调用Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);
    //判断eventloop对象是否在自己的线程里
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}
private:
    void handleRead();//响应wakeupfd的，也就是loop作为subreactor时，通过这个函数读取mainreactor发来的唤醒操作
    void doPendingFunctors();//执行eventloop回调，回调都放在了vector容器

    using ChannelList  = std::vector<Channel*>;
    std::atomic_bool looping_;//原子操作，通过CAS实现
    std::atomic_bool quit_;//标识退出loop循环

    const __pid_t threadId_;//记录当前loop所在线程的id

    Timestamp pollerReturnTime_;//poller返回的发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;//真正的最底层的事件循环器

    //作用：mainLoop获取一个新用户的channel，通过轮询选择一个subloop分配给它，通过该成员唤醒subloop处理channel
    int wakeupFd_;//eventfd创建出来的
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;//活跃的链接

    std::atomic_bool callingPendingFunctors_;//标识当前loop是否有需要执行的回调操作
    std::mutex mutex_;//互斥锁用来保护下面vector容器的线程安全操作
    std::vector<Functor> pendingFunctors_;//存储eventloop需要执行的所有回调操作（是loop需要执行的，不是channel）


};

