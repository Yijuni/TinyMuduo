#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>
#include <error.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
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
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&saveErrno);
    if(n>0){
        //收到已经建立连接的客户端的消息，调用用户传入的回调函数处理消息;
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }else if(n==0){
        handleClose();
    }else{
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handRead error");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    int saveErrno = 0;
    if(channel_->isWriting()){//是否注册了可写事件
        ssize_t n = outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if(n>0){
            if(outputBuffer_.readableBytes()==0){
                channel_->disableWriting();//数据已经写完了，就不能监测可写事件了   
                if(writeCompleteCallback_){
                    //调用handleWrite的时机是channel发生可写事件，loop检测到活跃的channel调用其handleEventWithGuard进而调用此函数
                    //也就是loop所在线程调用的，所以queueInLoop不会因为!isInLoopThread()为true而唤醒loop结束poller执行回调
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                //正在关闭，客户端主动关闭或者服务器想关闭连接
                if(kDisconnecting == state_){
                    shutDownInLoop();
                }
            }
        }
        else{
            LOG_ERROR("TcpConnection::handleWrite error");
        }
    }else{
        LOG_ERROR("TcpConnection channel_fd=%d is down,no more writing \n",channel_->fd());
    } 
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n",channel_->fd(),(int)state_);
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);//执行连接关闭的回调
    CloseCallback_(connPtr);//关闭连接的回调
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err =0;
    if(getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen)<0){//获取失败
        err = errno;
    }else{//获取成功
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s-SO_ERROR:%d\n",name_.c_str(),err);
}

void TcpConnection::sendInLoop(const void *message, size_t len)
{
}

void TcpConnection::shutDownInLoop()
{
}
