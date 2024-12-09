#pragma once

#include "nocopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;
/**
 * Channel（通道），封装sockfd和他注册的事件event，例如EPOLLIN,EPOLLOUT 
 * 还绑定了poller返回具体的事件
 */
class Channel : nocopyable
{

public:
    using EventCallBack = std::function<void()> ;
    using ReadEventCallBack = std::function<void(Timestamp)>;

    Channel(int fd,EventLoop *loop);
    ~Channel();

    //fd得到poller通知以后（事件触发后），根据revents_调用要合适的的回调函数
    void handleEvent(Timestamp receiveTime);

    //设置回调函数对象
    void setReadCallBack(ReadEventCallBack rcb){readCallBack_=std::move(rcb);}
    void setWriteCallBack(EventCallBack wcb){writeCallBack_ = std::move(wcb);}
    void setCloseCallBack(EventCallBack ccb){closeCallBack_ = std::move(ccb);}
    void setErrorCallBack(EventCallBack ecb){errorCallBack_ = std::move(ecb);}

    //防止channel被手动remove后，channel还在执行回调
    void tie(const std::shared_ptr<void>&);

    int fd()const{return fd_;}
    int events()const{return events_;}
    void set_revent(int revt) {revents_ = revt;}//由poller设置，也就是事件触发（事件监听到）后设置
    int index(){return index_;}
    void set_index(int idx){index_ = idx;}

    //通过channel设置事件的状态,也就是fd要关心的事件，需要poller提醒的事件
    
    //对fd读事件关注
    void enableReading(){events_ |= kReadEvent;update(); }
    //对fd读事件不关注
    void disableReading(){events_ &= ~kReadEvent;update(); }
    //对fd写事件关注
    void enableWriting(){events_ |= kWriteEvent;update(); }
    //对fd写事件不关注
    void disableWriting(){events_ &= ~kWriteEvent;update(); }
    //无视所有fd事件
    void disableAll(){events_ = kNoneEvent;update();}

    //返回fd_的事件状态
    bool isNoneEvent()const{return events_==kNoneEvent;}//判断这个fd是否注册了事件
    bool isWriting()const{return events_&kWriteEvent;}//判断fd是否处于写状态
    bool isReading()const{return events_&kReadEvent;}//判断fd是否处于写状态

   void remove();


private:
    void update();
 
    void handleEventWithGuard(Timestamp receiveTime); 

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    
    EventLoop* loop_;//和这个Channel绑定的事件循环，也就是channel所属的eventLoop
    const int fd_;//监听的对象(socket的fd),或者eventfd
    int events_;//监听对象fd_注册的事件,对应bit为0代表对该事件不做处理，1为监听该事件做处理
    int revents_;//poller返回的fd真实发生的事件，也是对这个int的bit位进行操作
    int index_;//标记这个channel的状态，添加到poller过=1，未添加到poller过=-1，添加到poller过但是删除了=2（ChannelMap没删除)

    std::weak_ptr<void> tie_;
    bool tied_;

    //channel通道通过revent_得知fd_真正发生的事件,根据发生的事件调用相应回调函数
    ReadEventCallBack  readCallBack_;
    EventCallBack writeCallBack_;
    EventCallBack closeCallBack_;
    EventCallBack errorCallBack_;

};
