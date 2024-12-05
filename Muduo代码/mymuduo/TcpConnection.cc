#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>
static EventLoop* CheckLoopNotNull(EventLoop* loop){
    if(loop==nullptr){
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null \n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr):
    loop_(CheckLoopNotNull(loop)),name_(name),state_(kConnecting),reading_(true),
    socket_(new Socket(sockfd)),channel_(new Channel(sockfd,loop_)),localAddr_(localAddr),
    peerAddr_(peerAddr),highWaterMark_(64*1024*1024)//64MB
{
    //给channel设置回调函数,poller通知channel后，channel调用相应回调函数
    channel_->setReadCallBack(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
    channel_->setWriteCallBack(std::bind(&TcpConnection::handleWrite,this));
    channel_->setCloseCallBack(std::bind(&TcpConnection::handleClose,this));
    channel_->setErrorCallBack(std::bind(&TcpConnection::handleError,this));

    LOG_INFO("TcpConnecton::ctor[%s] at fd=%d\n",name_.c_str(),sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n",name_.c_str(),socket_->fd(),(int)state_);
}

void TcpConnection::send(const void *message, int len)
{

}

void TcpConnection::shutDown()
{
}

void TcpConnection::connectEstablished()
{
}

void TcpConnection::connectDestroyed()
{
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
}

void TcpConnection::handleWrite()
{
}

void TcpConnection::handleClose()
{
}

void TcpConnection::handleError()
{
}

void TcpConnection::sendInLoop(const void *message, size_t len)
{
}

void TcpConnection::shutDownInLoop()
{
}
