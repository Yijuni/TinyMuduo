#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseloop, const std::string &name):
    baseloop_(baseloop),name_(name),started_(false),
    numThreads_(0),next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;
    for(int i=0;i<numThreads_;i++){
        char buf[name_.size()+32];
        snprintf(buf,sizeof buf,"%s%d",name_.c_str(),i);
        EventLoopThread *t = new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());//底层线程创建一个eventloop并返回
    }
    //整个服务端只有一个线程一个loop
    if(numThreads_==0 && cb){
        cb(baseloop_);//回调不为空，执行一下

    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseloop_;
    if(!loops_.empty()){//轮询获取下一个工作线程
        loop = loops_[next_];
        next_ = (next_++)%loops_.size();    
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoop()
{
    if(loops_.empty()){
        return std::vector<EventLoop*>(1,baseloop_);
    }
    return loops_;
}
