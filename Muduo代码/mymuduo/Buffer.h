#pragma once
#include "nocopyable.h"
#include <vector>
#include <string>
//网络库底层缓冲区类型定义
class Buffer{
public:
    static const std::size_t kCheapPrepend = 8;//前面不可读空间初始为8bytes
    static const std::size_t kInitialSize = 1024;
 
    explicit Buffer(std::size_t initialSize = kInitialSize);
    std::size_t readableBytes() const;
    std::size_t writableBytes() const;
    std::size_t prependableBytes() const;//返回读指针位置

    const char* peek() const;//返回缓冲区中可读数据的起始地址

    void retrieve(std::size_t len);
    void retrieveAll();
    std::string retrieveAllAsString();
    std::string retrieveAsString(size_t len);

    void ensureWriteableBytes(size_t len);//确保可写缓冲区够用
    
private: 
    void makeSpace(size_t len);
    char* begin();
    const char* begin() const;
    std::vector<char> buffer_;
    std::size_t readerIndex_;
    std::size_t writerIndex_;
};