#pragma once
#include "nocopyable.h"
#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>
/**
 * 一个Thread对象包含了一个thread的详细信息
 */
class Thread: nocopyable{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc,const std::string &name = std::string());
    ~Thread();
    void start();
    void join();

    bool started() const {return started_;}
    pid_t tid() const {return tid_;} 
    const std::string& name() const {return name_;}
    static int numCreated(){return numCreated_;}
private:
    void setDefaultName();
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_; 
    static int numCreated_;

};