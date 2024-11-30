#pragma once
/*
    2024.11.16 moyoj
    nocopyable可正常构造和析构，但是没法拷贝和赋值
    其他设计的类如果不想让其拷贝赋值就可以继承这个类
*/
class  nocopyable
{
private:
protected:
    nocopyable()=default;
    ~nocopyable()=default;
public:
    nocopyable(const nocopyable&)=delete;
    nocopyable& operator=(const nocopyable&)=delete;
};

