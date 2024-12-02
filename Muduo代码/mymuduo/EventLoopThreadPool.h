#pragma once
#include "nocopyable.h"
#include <functional>
#include <vector>
#include <string>
#include <memory>
class EventLoop;
class EventLoopThread;

class EventLoopThreadPool:nocopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; 
    EventLoopThreadPool(EventLoop* baseloop,const std::string& name);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads){numThreads_ = numThreads;}
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    //多线程,baseloop默认以轮训的方式分配Channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoop();
    bool started() const {return started_;}
    const std::string name() const {return name_;}
private:
    EventLoop *baseloop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;//创建的所有线程
    std::vector<EventLoop*> loops_;//不同线程中所有的loop
};

