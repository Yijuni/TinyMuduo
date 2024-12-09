#pragma once
#include "nocopyable.h" 
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"
#include <memory>
#include <string>
#include <atomic>
class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor =>有一个新的用户连接，accept获得confd，包装为TcpConnection 
 * 用户设置回调TcpServer设置回调=》TcpConnection设置回调 =》Channel设置回调 =》Poller监测事件 =》Channel回调操作
 */

class TcpConnection:nocopyable,public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(EventLoop* loop,const std::string name,int sockfd,const InetAddress& localAddr,const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const{return loop_;}
    const std::string& name() const{return name_;}
    const InetAddress& localAddr() const{return localAddr_;}
    const InetAddress& peerAddr() const{return peerAddr_;}

    bool connected() const{return state_ == kConnected;}//是否已连接

    void send(const void *message,int len);//发送数据
    void send(const std::string& message);
    void shutdown();//关闭连接
    
    void setConnectionCallback(const ConnectionCallback& cb){connectionCallback_ = cb;}
    void setCloseCallback(const CloseCallback& cb){CloseCallback_ = cb;}
    void setWriteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_ = cb;}
    void setHighWaterCallback(const HighWaterMarkCallback&cb,size_t highWaterMark){highWaterMarkCallback_ = cb;highWaterMark_=highWaterMark;}
    void setMessageCallback(const MessageCallback& cb){messageCallback_ = cb;}

    void connectEstablished();//连接建立
    void connectDestroyed();//连接销毁

    
private:
    enum StateE {kDisconnected,kConnecting,kConnected,kDisconnecting};

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* data,size_t len);
    void shutDownInLoop();

    void setState(StateE state){state_ = state;}
    EventLoop* loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;
    
    //这里和Acceptor类似
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;//有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完后的回调
    CloseCallback CloseCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;

    size_t highWaterMark_;//水位线
    
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};