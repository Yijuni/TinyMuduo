#pragma once
#include <string>
//时间类
class Timestamp
{

public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;
    ~Timestamp();
private:
    int64_t microSecondsSinceEpoch_;
};

