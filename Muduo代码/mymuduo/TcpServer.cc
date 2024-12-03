#include "TcpServer.h"
#include "Logger.h"
#include <functional>
EventLoop* CheckLoopNotNull(EventLoop* loop){
    if(loop==nullptr){
        LOG_FATAL("%s:%s:%d mainloop is null \n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}
        

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,const std::string& name, Option option):
    loop_(CheckLoopNotNull(loop)),name_(name),ipPort_(listenAddr.toIpPort()),
    acceptor_(new Acceptor(loop,listenAddr,option==kReusePort)),
    threadPool_(new EventLoopThreadPool(loop,name)),
    connectionCallback_(),messageCallback_(),nextConnId_(1)
{
    //acceptor->acceptor返回confd,然后调用以下函数
    acceptor_->setNewConnctionCallback(std::bind(&TcpServer::newConnection,this,
        std::placeholders::_1,std::placeholders::_2));//设置新连接的回调
    

}

TcpServer::~TcpServer()
{
}

void TcpServer::setThreadNum(int numThread)
{
    threadPool_->setThreadNum(numThread);
}

void TcpServer::start()
{
    if(started_++==0)//防止一个TcpServer对象，被start多次
    {
        threadPool_->start(threadInitCallback_);//启动底层loop线程池
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));//baseloop执行监听连接任务
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{

}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
}
