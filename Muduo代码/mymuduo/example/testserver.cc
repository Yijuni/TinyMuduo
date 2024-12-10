#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>
#include <string>
#include <functional>
class  EchoServer{
public:
    EchoServer(EventLoop* loop,const InetAddress &addr,const std::string &name):
        loop_(loop),server_(loop,addr,name)
    {
        //设置回调函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection,this,std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

        //设置合适的loop线程数量
        server_.setThreadNum(3);
    }
    void start(){
        server_.start();
    }
private:
    //连接建立或者断开的回调
    void onConnection(const TcpConnectionPtr &conn){
        if(conn->connected()){
            LOG_INFO("Connection Up : %s",conn->peerAddr().toIpPort().c_str());
        }else{
            LOG_INFO("Connection Down : %s",conn->peerAddr().toIpPort().c_str());
        }
    }
    
    //可读事件回调，读到对端数据后怎么处理
    void onMessage(const TcpConnectionPtr &conn,Buffer *buf,Timestamp time){
        std::string msg  = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();//写端 EPOLLHUP =》closeCallback_
    }

    
    EventLoop *loop_;
    TcpServer server_;
};


int main(){

    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop,addr,"EchoSer-01");//Acceptor non-blocking listen-fd create bind
    server.start();//listenfd loopthread listenfd->acceptChannel->mainLoop
    loop.loop();

    return 0;
}