/*
muduo网络库提供的主要类
TcpServer:编写服务器程序
TcpClient:编写客户端程序

epoll + 线程池
好处：能把网络I/O的代码和业务代码区分开
业务开发只需要关注用户的连接与断开、用户的可读写事件
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;

/*基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象指针
3.明确TcpServer需要什么构造函数参数
4.服务器类构造函数注册网络事件回调函数
5.设置合适服务端线程数量。muduo库会自己分配I/O线程和worker线程
*/

class ChatServer{
public:
    ChatServer(EventLoop* loop,//事件循环类似于boostasio的io_context
    const InetAddress& listenAddr,//IP+PORT
    const string& nameArg)//服务器名字    
    :_server(loop,listenAddr,nameArg),_loop(loop)
    {
        /*给服务器注册回调函数，事件发生后，loop就会调用回调函数进行处理，我们只关注怎么在回调函数里处理业务*/
        //用户连接时或断开时的回调
        _server.setConnectionCallback(bind(&ChatServer::onConnection,this,placeholders::_1));
        //读写事件回调
        _server.setMessageCallback(bind(&ChatServer::onMessage,this,placeholders::_1,placeholders::_2,placeholders::_3));
        //设置服务器线程数量 1个I/O线程 3个worker线程
        _server.setThreadNum(4);
    }
    //开启事件循环
    void start(){
        _server.start();
    }
private:
    //专门处理用户连接的创建和断开的回调函数 epoll listenfd accept
    void onConnection(const TcpConnectionPtr& con){
        cout<<"连接信息"<<endl;
        if(con->connected())
            cout<<con->peerAddress().toIpPort()<<"->"<<con->localAddress().toIpPort()<<"online"<<endl;
        else{
            cout<<con->peerAddress().toIpPort()<<"->"<<con->localAddress().toIpPort()<<"offline"<<endl;
            con->shutdown();//close(fd)
            // _loop->quit();
        }

    }
    //专门处理用户读写事件的 (连接指针，缓冲区，接收到数据的时间信息)
    void onMessage(const TcpConnectionPtr& con,Buffer* buffer,Timestamp time){
        string buf = buffer->retrieveAllAsString();
        cout<<"receive data: "<<buf<<"time:"<<time.toString()<<endl;
        con->send(buf);
    }
    TcpServer _server;
    EventLoop *_loop;

};

int main(){
    EventLoop loop;//io_context
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"chatserver");
    server.start();
    loop.loop();//epoll_wait以阻塞方式等待用户发来的信息
    return 0;
}