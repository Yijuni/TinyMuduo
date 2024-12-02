#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>
std::atomic_int Thread::numCreated_(0);//静态变量需要在类外初始化
Thread::Thread(ThreadFunc func, const std::string name):
    started_(false),joined_(false),func_(std::move(func)),
    name_(name),tid_(0)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(started_ && !joined_){
        thread_->detach();//thread提供了设置线程分离的方法
    }
}

void Thread::start()
{
    started_ = true;

    sem_t sem;//信号量
    sem_init(&sem,false,0);
    //启动线程
    thread_ = std::make_shared<std::thread>([&](){
        tid_ = CurrentThread::tid();//获取创建线程的tid值
        sem_post(&sem);//P操作，P给信号量资源+1
        func_();//开启一个线程，专门执行这个函数
    });
    //这里必须等待上面的新线程获取完tid值之后才能往下执行，因为线程执行顺序不确定，所以用信号量进行同步
    sem_wait(&sem);//等待资源
}
//给外部调用者提供join
void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty()){
        char buf[32]={0};
        snprintf(buf,sizeof buf,"Thread%d",num);
        name_ = buf;
    }
}
