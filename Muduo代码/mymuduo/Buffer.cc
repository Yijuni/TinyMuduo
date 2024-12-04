#include "Buffer.h"

Buffer::Buffer(std::size_t initialSize):buffer_(kCheapPrepend + initialSize),
    readerIndex_(kCheapPrepend),writerIndex_(kCheapPrepend)
{

}
std::size_t Buffer::readableBytes() const{
    return writerIndex_ - readerIndex_;
}
std::size_t Buffer::writableBytes() const{
    return buffer_.size()-writerIndex_;
}
std::size_t Buffer::prependableBytes() const{
    return readerIndex_;
}
char* Buffer::begin(){
    return &(*buffer_.begin());//返回底层数组裸指针
}
//作为常量给一些常量表达式使用
const char* Buffer::begin() const{
    return &(*buffer_.begin());//返回底层数组裸指针
}
const char* Buffer::peek() const{
    return begin() + readerIndex_;
}
//Buffer->std::string
void Buffer::retrieve(std::size_t len){
    if(len <readableBytes()){
        readerIndex_ += len;//更新下一次的读取位置
        return;
    }
    retrieveAll();

}
void Buffer::retrieveAll(){
    writerIndex_ = kCheapPrepend;//读完了所有数据，再从头开始写
    readerIndex_ = kCheapPrepend;
}
//onMessage函数上报的Buffer数据，转成string类型数据返回
std::string Buffer::retrieveAllAsString(){
    return retrieveAsString(readableBytes());//可读取数据的长度
}
std::string Buffer::retrieveAsString(size_t len){
    std::string result(peek(),len);
    retrieve(len);//对缓冲区进行复位操作，也就是修改readerIndex_
    return result;
}
void Buffer::makeSpace(size_t len){
    if(writableBytes()+readableBytes() < len + kCheapPrepend)//也就是剩下的可利用空间凑不出来len长度，只能resize
    {
        buffer_.resize(writerIndex_+len);//实际上只增加了writeIndex_+len - buffer_.size()[原来的size]
        return;
    }
    //剩下的可写空间可以够len长度，那就移动数据腾出足够空间
     size_t readable = readableBytes();
     std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
     readerIndex_ = kCheapPrepend;
     writerIndex_ = readerIndex_+readable;
}
void Buffer::ensureWriteableBytes(size_t len)
{
    if(writableBytes()<len){
        makeSpace(len);//扩容函数
    }

}
