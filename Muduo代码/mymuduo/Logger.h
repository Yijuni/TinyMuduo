#pragma once

#include <string>

#include "nocopyable.h"

//用法 ： LOG_INFO("%s %d",arg1,arg2)，定义宏需要写多行代码时需要在每行末尾加上"\""
#define LOG_INFO(LogMsgFormat,...)\
    do\
    {\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024]={0};\
        snprintf(buf,1024,LogMsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)

#define LOG_ERROR(LogMsgFormat,...)\
    do\
    {\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024]={0};\
        snprintf(buf,1024,LogMsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)

#define LOG_FATAL(LogMsgFormat,...)\
    do\
    {\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024]={0};\
        snprintf(buf,1024,LogMsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
        exit(1);\
    }while(0)

//这个意思是如果你定义了MUDEBUG这个宏名，就可以正常输出debug的调试信息,否则啥也不输出,发布版本可以关闭debug输出，修改版本可以打开debug输出
#ifdef MUDEBUG
#define LOG_DEBUG(LogMsgFormat,...)\
    do\
    {\
        Logger &logger = Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024]={0};\
        snprintf(buf,1024,LogMsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)
#else
    #define LOG_DEBUG(LogMsgFormat,...)
#endif

//定义日志级别 INFO ERROR FATAL DEBUG
enum LogLevel{
    INFO ,//普通信息
    ERROR,//错误信息
    FATAL,//严重错误崩溃信息
    DEBUG,//调试信息
};

//输出一个日志类
class Logger:nocopyable
{
public:
    //获取日志唯一实例对象
    static Logger& instance();
    //设置日志级别
    void setLogLevel(int level);
    //写日志
    void log(std::string msg);
private:
    Logger(){};
    int logLevel_;
};

