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

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());//绑定TcpConnection，防止该对象被释放
    channel_->enableReading();//注册EPOLL_IN事件

    //新连接建立执行回调
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if(state_==kConnected){
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();//从poller中删除
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
                    // loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                    loop_->runInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                //状态设置过正在关闭，缓冲区数据发送完之后会再次执行shutDownInLoop
                if(kDisconnecting == state_){
                    shutDownInLoop();
                }
            }
        }
        else{
            LOG_ERROR("TcpConnection::handleWrite error code：%d",saveErrno);
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
    closeCallback_(connPtr);//关闭连接的回调,调用的是TcpServer的removeConnection
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

// void TcpConnection::send(const void *message, int len)
// {

// }

void TcpConnection::send(const std::string &message)
{
    if(state_==kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(message.c_str(),message.length());
        }else{
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,this,message.c_str(),message.size()));
        }
    }
}

/**
 * 发送数据,应用写得快，内核发送数据慢，需要把待发送数据写入缓冲区
 */
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrite = 0;
    size_t remaining = len;
    bool faultError = false;
    if(state_ == kDisconnected){//调用过该connection的shutdown就不能发送了
        LOG_ERROR("disconnected,give up writing!\n");
        return;
    }
    if(!channel_->isWriting() && outputBuffer_.readableBytes()==0){//channel第一次开始写数据，而且缓冲区没有待发送数据
        nwrite = write(channel_->fd(),data,len);
        if(nwrite>=0){
            remaining = len-nwrite;
            if(remaining==0 && writeCompleteCallback_){
                //一次性写完了直接调用写完成回调,也不用给channel设置epoll_out事件了
                // loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                loop_->runInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
            }
        }else{
            nwrite = 0;
            if(errno!=EWOULDBLOCK){
                LOG_ERROR("TcpConnection::sendInLoop\n");
                if(errno== EPIPE || errno==ECONNRESET){//SIGPIPE(本地端连接已关闭你还发送就会触发这个错误) RESET(连接被对方重置) 
                    faultError = true;
                }
            }
        }
    }
    //数据没有全部发送完，剩余数据需要保存到outPutBuffer_，并给channel注册EPOLL_OUT
    //发送缓冲区从不可发送（没空间）=》可发送（有空间），会触发EPOLL_OUT信号，调用给channel注册的handleWrite回调继续发送数据
    if(!faultError&&remaining>0){
        size_t oleLen = outputBuffer_.readableBytes();//目前缓冲区剩余的待发送数据
        if(oleLen+remaining>=highWaterMark_&&oleLen<highWaterMark_&&highWaterMarkCallback_){
            loop_->runInLoop(std::bind(highWaterMarkCallback_,shared_from_this(),oleLen+remaining));
        }
        outputBuffer_.append((char*)data+nwrite,remaining);
        if(!channel_->isWriting()){
            channel_->enableWriting();//注册可写事件才能继续发送outputBuffer_缓冲区数据
        }
    } 
}

void TcpConnection::shutdown()
{
    if(state_==kConnected){
        setState(kDisconnecting);//设置状态正在关闭，还没关闭，数据发完才关闭
        loop_->runInLoop(std::bind(&TcpConnection::shutDownInLoop,this));
    }
}

void TcpConnection::shutDownInLoop()
{
    if(!channel_->isWriting()){//没有注册可写事件，说明缓冲区数据全部发送完成
        //关闭写端，触发EPOLLHUP，channel调用closeCallback_也就是TcpConnection的handlClose方法，进而再调用该对象注册的CloseCallback_
        socket_->shutdownWrite();
    }
}
