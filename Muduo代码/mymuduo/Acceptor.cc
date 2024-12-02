#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <errno.h>
#include "Logger.h"
#include <sys/socket.h>
#include <sys/types.h>
static int createNonblocking(){//创建一个监听连接的socket
    int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
    if(sockfd<0){
        LOG_FATAL("%s : %s : %d listen socket create err:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);//文件 函数 行数 错误号
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool refuseport):
    loop_(loop),acceptSocket_(createNonblocking()),
    acceptChannel_(acceptSocket_.fd(),loop),listenning_(false)
{
    acceptSocket_.setRefusePort(true);
    acceptSocket_.setRefusePort(true);

    acceptSocket_.bindAddress(listenAddr);//bind

    //TcpServer::start() Acceptor.listen 有新用户的连接，
    //要执行一个回调，把返回的连接fd打包成channel，唤醒一个loop，把channel放进loop

    //这里还不能让他监听可读事件，需要listen之后才让他检测可读事件进而加入epoll_wait,防止浪费资源
    acceptChannel_.setReadCallBack(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();//从epoll_wait中移除,但是还在poller中
    acceptChannel_.remove();//从poller的map中移除
}

void Acceptor::listen()//listen
{
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();//监测可读事件

}
//listenfd有事件发生了，也就是缓冲区有数据可读了，也就是有新用户连接了，会调用这个
void Acceptor::handleRead()//accept
{
    InetAddress peerAddr;//对端IP+PORT
    int confd = acceptSocket_.accept(&peerAddr);
    if(confd>=0){
        if(newConnectionCallback_){
            newConnectionCallback_(confd,peerAddr);//轮询找到subloop唤醒，分发当前的新客户端的channel
        }else{
            ::close(confd);
        }
    }else{
        LOG_ERROR("%s : %s : %d accept confd err:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);//文件 函数 行数 错误号
        if(errno == EMFILE)//没有可用的fd给新连接了
        {
            LOG_ERROR("%s : %s : %d sockfd reached limit!\n",__FILE__,__FUNCTION__,__LINE__);
        }
    }
}
