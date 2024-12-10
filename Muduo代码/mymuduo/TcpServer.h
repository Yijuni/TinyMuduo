#pragma once
/**
 * 用户使用Muduo库编写服务器程序
 */
#include "nocopyable.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include <unordered_map>
#include <atomic>
class TcpConnection;
//对外服务器编程使用的类
class TcpServer:nocopyable
{
public:
    
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    enum Option{
        kNoReusePort,//是否对端口可重用
        kReusePort,
    };
    
    TcpServer(EventLoop*,const InetAddress& listenAddr,const std::string& name,Option option = kNoReusePort);
    ~TcpServer();
    void setThreadNum(int numThread);//设置底层subloop的个数（线程数）

    void setThreadInitCallback(const ThreadInitCallback&cb){threadInitCallback_ = cb;}
    void setConnectionCallback(const ConnectionCallback &cb){connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback &cb){messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback &cb){writeCompleteCallback_ = cb;}

    void start();//开启服务器监听
     

private:

    void newConnection(int sockfd,const InetAddress &peerAddr);//给Acceptor的回调，也就是有新连接时的回调
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;
    EventLoop* loop_;//baseloop 用户定义的loop
    const std::string ipPort_;//服务器ip端口
    const std::string name_;//服务器名称
    std::unique_ptr<Acceptor> acceptor_;//运行在mainloop，监听新连接事件
    std::unique_ptr<EventLoopThreadPool> threadPool_;//one loop peer thread
     
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;//有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完后的回调

    ThreadInitCallback threadInitCallback_;//线程初始化的回调

    std::atomic_int started_;

    int nextConnId_;//连接id
    ConnectionMap connections_;//保存所有连接
}; 
