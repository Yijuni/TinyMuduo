#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"
#include "Poller.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
//防止一个线程创建多个eventloop
__thread EventLoop *t_loopInThisThread = nullptr;
//定义默认的Poller，IO复用接口的超时时间
const int kPollTimeMs = 10000;
//创建wakeupfd，用来notify唤醒subReactor来处理新来的channel
int createEventFd(){
    int evtfd = eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
    if(evtfd<0){
        LOG_FATAL("eventfd error:%d \n",errno);
    }   
    return evtfd;
}

EventLoop::EventLoop():looping_(false),
    quit_(false),
    callingPendingFunctors_(false),//是否正在处理回调
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    wakeupFd_(createEventFd()),
    wakeupChannel_(new Channel(wakeupFd_,this))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
    if(t_loopInThisThread){//不为空说明该线程已经有一个loop了，那就不能创建了
        LOG_FATAL("Another EventLoop %p exists int this thread %p \n",this,t_loopInThisThread);
    }else{
        t_loopInThisThread = this;
    }
    //设置wakeupfd需要关注的事件，以及对应读事件回调
    wakeupChannel_->setReadCallBack(std::bind(&EventLoop::handleRead,this));
    //每一个eventloop都将循环检测wakeupChannel管理的wakeupfd的EPOLLIN读事件，方便唤醒
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    //资源回收
    wakeupChannel_->disableAll();//读写事件都不关心
    wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread = nullptr;
}
//开始事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping\n",this);
    while (!quit_)
    {
        activeChannels_.clear();
        //监听两类fd： 一种clientfd，一种wakeupfd
        //wakeup：mainloop和subloop之间通信
        //client：正常的与客户端通信
        pollerReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
        //Poller监听哪些channel发生事件了，然后上报给EventLoop，通知Channel处理相应的事件
        for(Channel* channel:activeChannels_){
            channel->handleEvent(pollerReturnTime_);
        }
        //执行当前EventLoop事件循环需要执行的回调操作
        /**
         * 当这个EventLoop是负责连接的eventloop，则需要他以下操作：
         * 通过wakeupfd唤醒subreactor（eventloop）
         * mainLoop事先注册一个回调callback给subloop来执行,mainReactor的回调函数由更上层注册
         * 接受连接，返回channel打包的fd给subreactor负责后序的消息处理
         */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n",this);
}   
//退出事件循环：1.在自己的线程调用quit（可以直接退出）2.在其他线程调用了这个loop的quit（需要进行一些处理）
void EventLoop::quit()
{
    quit_=true;
    if(!isInLoopThread()){
        //如果是在其他线程中被调用了quit,说明是别人调用的，比如调其他线程用了mainloop的quit，那么退出前需要唤醒一下让他从eventloop返回回来
        wakeUp();
    }
}
//在当前loop中执行
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread()){//就在当前线程执行
        cb();
    }else{//不在当前线程执行，那就先需要唤醒loop所在线程执行cb
        queueInLoop(cb);
    }
}
//不在当前线程执行的，把cb放入队列中，唤醒loop所在的线程执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        //unique_lock会自动释放
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    //唤醒该loop线程在执行回调
    /**
     * callPendingFunctors表示当前loop正在执行回调，现在你又添加了新的回调
     * 当执行完上一次回调后，又会阻塞在poll位置，所以需要再wakeup唤醒一下，执行
     * 现在新加的回调
     */
    if(!isInLoopThread() || callingPendingFunctors_){
        wakeUp();//唤醒loop所在线程执行回调
    }
}
//唤醒loop所在线程，向wakeupfd_写一个数据就唤醒了,wakeupChannel会发生读事件，loop线程被唤醒
void EventLoop::wakeUp()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 bytes\n",n);
    }
}
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}
// mainreactor可以通过subreactor所处线程的wakeupfd写数据触发读事件，从而触发这个读回调，通知唤醒subreactor监听事件
void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_,&one,sizeof one);//从wakeupfd读内容到one
  if(n!=sizeof one){
    LOG_FATAL("eventLoo::handleRead read %ld bytes instead of 8 bytes(64 bit)\n",n);
  }  
}
//唤醒后执行每个活跃channel回调后，在执行这个loop监听完事件后的回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_=true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);//交换后，不影响queueInLoop的工作，这个函数直接用局部的vector进行回调
    }
    for(const Functor &functor:functors){
        functor();//执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_=false;
}
