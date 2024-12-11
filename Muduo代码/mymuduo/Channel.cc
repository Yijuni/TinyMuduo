#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"
#include <sys/epoll.h>
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(int fd, EventLoop *loop)
    :loop_(loop),fd_(fd),events_(0),revents_(0),index_(-1),tied_(false)
{

}

Channel::~Channel()
{
}
/**
 * 这个机制称为“绑定”，通常用于确保在事件处理期间外部对象的生命周期是有效的。
 * tie_ 是一个 weak_ptr，用以弱引用一个对象。lock() 方法尝试提升为 shared_ptr。
 * 如果绑定的对象依然存在，guard 就会指向它；如果对象已被销毁，guard 将为 nullptr
 * 检查 guard 是否有效。如果 guard 有效，意味着与 Channel 绑定的对象仍然存在，可以安全地处理事件。
 */
void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_){
        //提升成强智能指针，防止事件回调函数处理过程中TcpConnection被移除，也就是回调完成前确保资源可用
        std::shared_ptr<void> guard = tie_.lock(); 
        if(guard){//如果提升失败说明TcpConnection没有存活那就不执行
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}
//一个TcpConnection创立的时候会调用
void Channel::tie(const std::shared_ptr<void> &obj )
{
    tie_ = obj;
    tied_=true;
}

/**
 * 当更改fd的事件状态（关注/不关注），需要更新poller里面fd相应事件的epoll_ctl
 * EventLoop包含Channel和poller
 */
void Channel::update()
{
    //通过channel所属的eventloop调用poller相应方法更新fd的注册事件
    loop_->updateChannel(this); 
}
//在channel所属的eventLoop中删除channel
void Channel::remove()
{
    loop_->removeChannel(this);
}
//根据poller返回的具体事件，channel进行对应的回调
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revent:%d\n",revents_);
    /**
     * revents_&'EPOLLHUP': 检查网络连接是否关闭（Hang Up）。
     * !(revents_ & EPOLLIN): 确保没有可读事件（即没有 EPOLLIN 标志）。这意味着如果连接被关闭并且没有新数据可以读取，可能表示连接异常。
     */
    if((revents_&EPOLLHUP)&&!(revents_&EPOLLIN)){
        if(closeCallBack_){
            closeCallBack_();
        }
    }
    //这一部分检查 revents_ 是否包含 EPOLLERR 标志，说明发生了错误。
    if(revents_&EPOLLERR){
        if(errorCallBack_)
        {
            errorCallBack_();
        }
    }
    /**
     * 检查 revents_ 是否包含 EPOLLIN（可读事件）或 EPOLLPRI（优先级数据事件）标志之一。
     * 这表明可以从套接字或文件描述符中读取数据。
     */
    if(revents_&(EPOLLIN | EPOLLPRI)){
        if(readCallBack_){
            readCallBack_(receiveTime);
        }
    }
    //检查 revents_ 是否包含 EPOLLOUT 标志，缓冲区现在不满了。
    if(revents_&EPOLLOUT){
        if(writeCallBack_){
            writeCallBack_();
        }
    }

}
