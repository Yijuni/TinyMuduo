#include "Logger.h"
#include "Timestamp.h"

#include<iostream>

Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}
//写日志 [级别信息] 时间信息 : msg
void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
    case INFO:
        std::cout<<"[INFO]"<<std::endl;
        break;
    case ERROR:
        std::cout<<"[ERROR]"<<std::endl;
        break;
    case FATAL:
        std::cout<<"[FATAL]"<<std::endl;
        break;
    case DEBUG:
        std::cout<<"[DEBUG]"<<std::endl;
        break;
    default:
        break;
    }

    //打印时间和msg
    std::cout<<Timestamp::now().toString()<<":"<<msg<<std::endl;

}
