#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"
#include <functional>
#include <strings.h>
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
    connectionCallback_(),messageCallback_(),nextConnId_(1),started_(0)
{
    //acceptor->acceptor返回confd,然后调用以下函数
    acceptor_->setNewConnctionCallback(std::bind(&TcpServer::newConnection,this,
        std::placeholders::_1,std::placeholders::_2));//设置新连接的回调
    

}

TcpServer::~TcpServer()
{
    for(auto& item:connections_){
        TcpConnectionPtr conn(item.second);
        item.second.reset();//原来的智能指针释放掉裸指针，指向该裸指针的智能指针计数减一
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }

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
//新的客户端连接会执行这个回调，acceptor管理的channel检测到可读事件调用handleRead，accept新连接，进而会执行这个回调
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *ioLoop = threadPool_->getNextLoop();//选择一个subloop来管理新的连接fd
    char buf[64]={0};
    snprintf(buf,sizeof buf,"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());
    //通过sockfd获取其绑定的本机的IP地址和端口信息
    sockaddr_in local;
    bzero(&local,sizeof local);
    socklen_t addrLen = sizeof local;
    if(getsockname(sockfd,(sockaddr *)&local,&addrLen)<0){
        LOG_ERROR("sockets::getLocalAddr\n");
    }

    InetAddress localAddr(local);

    //根据连接成功的sockfd，创建TcpConnection连接对象，状态：kConnecting
    TcpConnectionPtr connPtr(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));
    connections_[connName] = connPtr;

    //下面三个的回调都是用这个库的人设置的 
    connPtr->setConnectionCallback(connectionCallback_);//TcpConnection涉及连接关闭或者连接出错都会调用这个
    connPtr->setMessageCallback(messageCallback_);
    connPtr->setWriteCompleteCallback(writeCompleteCallback_);

    //设置如何关闭连接的回调
    connPtr->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
    //调用establish，channel开始监听可读事件，状态：kConnected
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,connPtr));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",name_.c_str(),conn->name().c_str());

   connections_.erase(conn->name());
   EventLoop *ioLoop = conn->getLoop();
   ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}
