#pragma once

#include <sys/epoll.h>
#include "Poller.h"


/**
 * EPOLL使用
 * epoll_create
 * epoll_ctl add/mod/del
 * epoll_wait
 */
class EPollPoller:public Poller{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;
    //重写Poller抽象方法
    Timestamp poll(int timeOutMs,ChannelList* activeChannel) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;
private:
    //初始事件最大容量
    static const int kInitEventListSize = 16;
    //添加活跃的连接
    void fillActiveChannels(int numEvents,ChannelList *activeChannels) const;
    //更新channel通道
    void update(int operation,Channel* channel);
    //发生事件放入vector
    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;//真正发生的事件会被epoll_wait填入这里

};