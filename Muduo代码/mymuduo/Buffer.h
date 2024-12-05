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

    std::string retrieveAllAsString();//Buffer->std::string
    std::string retrieveAsString(size_t len);

    void append(const char* data,size_t len);
    ssize_t readFd(int fd,int *saveErrno);//从fd读取对端发来的数据
    ssize_t writeFd(int fd,int *saveErrno);//往fd写数据，发送数据给对端

    std::size_t readableBytes() const;
    std::size_t writableBytes() const;
    std::size_t prependableBytes() const;//返回读指针位置
private: 


    const char* peek() const;//返回缓冲区中可读数据的起始地址
    //更新readerindex_
    void retrieve(std::size_t len);
    void retrieveAll();
    void ensureWriteableBytes(size_t len);//确保可写缓冲区够用
    char* beginWrite();
    void makeSpace(size_t len);
    char* begin();
    const char* begin() const;
    std::vector<char> buffer_;
    std::size_t readerIndex_;
    std::size_t writerIndex_;
};