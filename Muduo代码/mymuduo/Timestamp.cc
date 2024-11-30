#include "Timestamp.h"

#include <time.h>

Timestamp::Timestamp():microSecondsSinceEpoch_(0){

}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch){

}
Timestamp Timestamp::now(){
    return Timestamp(time(NULL));
}
std::string Timestamp::toString() const{
    char buf[128]={0};
    //tm这个结构体某些代表的是相对起始值的偏差，比如tm_year=2的话，现在就是1902年，但是有的就是真实时间值
    tm* tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf,128,"%4d/%02d/%02d %02d:%02d:%02d",
            tm_time->tm_year + 1900,tm_time->tm_mon+1,tm_time->tm_mday,
            tm_time->tm_hour,tm_time->tm_min,tm_time->tm_sec);
    return buf;
}
Timestamp::~Timestamp(){

}
// #include <iostream>
// int main(){
//     std::cout<<Timestamp::now().toString()<<std::endl;
//     return 0;
// }