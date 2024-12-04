#include "Buffer.h"
#include <sys/uio.h>
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
    if(writableBytes()+readableBytes() < len + kCheapPrepend)//也就是剩下的可利用空间（包括之前读完的）凑不出来len长度，只能resize
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

void Buffer::append(const char* data,size_t len){//一般是writeindex_后的可写空间不够才用这个，然后自动扩容
    ensureWriteableBytes(len);
    std::copy(data,data+len,beginWrite());
    writerIndex_ += len;
}


ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    //之所以用65536是因为一次读取的字节数最多这么多
    char extrabuf[65536] = {0};//这是栈上的内存空间

    struct iovec vec[2];//该结构体包含一个内存起始地址 和这段内存可存储字节数
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;//先读到buffer_剩下的可读空间里
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf; 
    //如果可写空间本来就大于65535bytes只需要一个内存空间vec[0],否则两个空间都用上
    const int iovcnt = (writable <sizeof extrabuf)? 2 : 1;
    const ssize_t n = readv(fd,vec,iovcnt);//从同一个fd读到多个缓冲区内
    if(n<0){
        *saveErrno = errno;
    }else if(n<=writable){//buffer_可写缓冲区够存储读出来的数据
        writerIndex_ += n;
    }else{
        writerIndex_ = buffer_.size();//已经写满了更新一下writerIndex_
        append(extrabuf,n-writable);//把剩下的没读进去的append一下
    }

    return n;
}



char* Buffer::beginWrite(){
    return begin()+writerIndex_;
}