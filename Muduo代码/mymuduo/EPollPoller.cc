#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include <string.h> 
#include <unistd.h>
#include <errno.h>
const int kNew = -1;//不在channls_中，而且没在poller中注册
const int kAdd = 1;//在channels_中而且在poller中注册了事件
const int kDeleted = 2;//在channels_里添加过，但是在poller中没注册事件

EPollPoller::EPollPoller(EventLoop *loop):
    Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),//epoll_create具体操作
    events_(kInitEventListSize)//vector<epoll_event>
{
    if(epollfd_<0){
        LOG_FATAL("epoll_create error:%d\n",errno);
    }
}

EPollPoller::~EPollPoller()
{
    close(epollfd_);//关闭epoll
}

/**
 * epoll_wait具体操作
 * 通过形参，把发生事件的channel告知给EventLoop
 */
Timestamp EPollPoller::poll(int timeOutMs, ChannelList *activeChannel)
{
    //这里用LOG_DEBUG更合理
    LOG_INFO("func=%s => fd total count:%lu \n",__FUNCTION__,channels_.size());
    //&*events_.begin()获取vector底层数组的首地址，vector底层也是用数组实现的
    //开始轮询事件，发生事件包括其对应的其他信息会直接填写到events_中（epoll_event数组)
    int numEvents = epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeOutMs);
    int saveErrno = errno;//保存最开始的错误
    Timestamp now(Timestamp::now());//记录一下发生事件的时间
    if(numEvents > 0){
        LOG_INFO("%d events happended \n",numEvents);
        fillActiveChannels(numEvents,activeChannel);

        //如果发生的事件个数和vector元素个数一样，那得扩容vector了，因为发生事件可能比返回的还多
        if(numEvents==events_.size()){
            events_.resize(2*events_.size());
        }
    }else if(numEvents==0){
        //超时了
        LOG_DEBUG("%s timeout \n",__FUNCTION__);
    }else{
        if(saveErrno != EINVAL)//错误不等于外部中断
        {
            errno = saveErrno;//把最开始发生的错误复制回去
            LOG_ERROR("EPollPoller::poll err");
        }    
    }
    return now;//把发生事件的时间点返回回去
}

/*
 * channel的update、remove通过EventLoop的 updateChannel、removeChannel调用eventpoller的相应函数进行更新删除
 * 此函数更新channel的状态
 */
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    const int fd = channel->fd();
    LOG_INFO("func=%s => fd=%d:[event:%d index=%d]\n",__FUNCTION__,fd,channel->events(),index);//__FUNCTION__指的是当前函数名

    if(index==kNew || index==kDeleted){//channel从来没添加或者添加了之后删除了
        if(index==kNew){
            channels_[fd]=channel;
        }
        channel->set_index(kAdd);
        update(EPOLL_CTL_ADD,channel);
    }else{//channel已经在poller注册过了
        if(channel->isNoneEvent()){//对任何事件不感兴趣
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
            return;
        }
        update(EPOLL_CTL_MOD,channel);//channel有感兴趣的事件但是感兴趣的事件改变了，只更新下状态
    }
}

/**
 * 这个是在channels_中删掉，poller也取消事件注册
 */
void EPollPoller::removeChannel(Channel *channel)
{

    const int fd = channel->fd();
    int index = channel->index();
    channels_.erase(fd);//从channels_中移除
    LOG_INFO("func=%s => fd=%d\n",__FUNCTION__,fd);

    if(index==kAdd){
        update(EPOLL_CTL_DEL,channel);//从poller中移除事件注册
    }
    channel->set_index(kNew);
}
/**
 * 
 */
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for(int i=0;i<numEvents;i++){
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);//获取发生事件的fd绑定的channel
        channel->set_revent(events_[i].events);//设置该channel真正发生的事件
        activeChannels->push_back(channel);
    }
}

/**
 * 进行epoll_ctl的具体操作 add/mod/del  
 */
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event,0,sizeof event);
    int fd = channel->fd();

    event.events = channel->events();//channel感兴趣的事件传给event，发生某个事件时会告知
    event.data.fd = fd;//事件绑定的fd
    event.data.ptr = channel;//fd绑定的channel,事件发生后可以通过返回的epoll_event获取channel

    if(epoll_ctl(epollfd_,operation,fd,&event)<0){//给epoll注册要监听的事件
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error:%d\n",errno);//errno错误码。出错之后系统提供的函数就会自动置erron
        }else{
            LOG_FATAL("epoll_ctl modify(add mod) error:%d",errno);
        }
    }

}
