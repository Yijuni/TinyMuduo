#include "Poller.h"
#include "EPollPoller.h"
#include <stdlib.h>
/**
 * 在这里实现Poller的这个成员函数，是为了防止基类包含派生类
 * 这个成员函数是创建某种Poller，然后返回给EventLoop
 * 创建的某种Poller是这个类的派生类，不可能在Poller.cc文件中创建并返回
 * 
 * 
 */
Poller* Poller::newDefaultPoller(EventLoop* loop){
    if(::getenv("MUDUO_USE_POLL")){
        return nullptr;//生成poll实例
    }else{
        return new EPollPoller(loop);//生成epoll实例
    }
}