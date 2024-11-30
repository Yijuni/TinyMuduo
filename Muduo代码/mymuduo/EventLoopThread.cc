#include "EventLoopThread.h"
#include "EventLoop.h"
#include <memory>
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,const std::string &name = std::string()):
    loop_(nullptr),exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc,this),name),
    initcallback_(cb)
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_!=nullptr){
        loop_->quit();
        thread_.join();//等待子线程结束
    }
}
//运行一个新线程，并获取该线程中的eventloop
EventLoop *EventLoopThread::startLoop()
{
    thread_.start();//启动底层线程
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_==nullptr)//防止虚假唤醒，写成while
        {
            cond_.wait(lock);//先unlock之前获得的mutex，然后阻塞当前的执行线程。被唤醒后，该thread会重新获取mutex，获取到mutex后执行后面的动作。
        }
        loop = loop_;
        
    }

    return loop;
}
//这个方法是在新线程中要执行的
void EventLoopThread::threadFunc()
{
    //one loop per thread
    EventLoop loop;//在启动的线程中，创建一个EventLoop
    if(initcallback_){//初始化函数如果存在就运行初始化函数
        // 新线程中设置或初始化EventLoop对象之前执行一些自定义的初始化代码。
        initcallback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr; 
}
