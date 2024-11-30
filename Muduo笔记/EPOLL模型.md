# <center>EPOLL模型一些知识</center>
___
## 一些API
### epoll_create
```c++
#include <sys/epoll.h>
 
int epoll_create(int size);
 
参数：
size:目前内核还没有实际使用，只要大于0就行
 
返回值：
返回epoll文件描述符
```



### epoll_ctl
```c++
#include <sys/epoll.h>
 
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
 
参数：
epfd：epoll文件描述符
op：操作码
EPOLL_CTL_ADD:插入事件
EPOLL_CTL_DEL:删除事件
EPOLL_CTL_MOD:修改事件
fd：事件绑定的套接字文件描述符
events：事件结构体
 
返回值：
成功：返回0
失败：返回-1
```

### epoll_wait
```c++
#include <sys/epoll.h>
 
int epoll_wait(int epfd, struct epoll_event *events,              
int maxevents, int timeout);
 
参数：
epfd：epoll文件描述符
events：epoll事件数组
maxevents：epoll事件数组长度
timeout：超时时间
小于0：一直等待
等于0：立即返回
大于0：等待超时时间返回，单位毫秒
 
返回值：
小于0：出错
等于0：超时
大于0：返回就绪事件个数
```
### epoll_event
```c++
#include <sys/epoll.h>
 
struct epoll_event{
  uint32_t events; //epoll事件，参考事件列表 
  epoll_data_t data;
} ;
typedef union epoll_data {  
    void *ptr;  
    int fd;  //套接字文件描述符
    uint32_t u32;  
    uint64_t u64;
} epoll_data_t;
```
### event事件列表
```c++
#include<sys/epoll.h>
 
enum EPOLL_EVENTS
{
    EPOLLIN = 0x001, //读事件
    EPOLLPRI = 0x002,
    EPOLLOUT = 0x004, //写事件
    EPOLLRDNORM = 0x040,
    EPOLLRDBAND = 0x080,
    EPOLLWRNORM = 0x100,
    EPOLLWRBAND = 0x200,
    EPOLLMSG = 0x400,
    EPOLLERR = 0x008, //出错事件
    EPOLLHUP = 0x010, //表示对应的文件描述符被挂断
    EPOLLRDHUP = 0x2000,
    EPOLLEXCLUSIVE = 1u << 28,
    EPOLLWAKEUP = 1u << 29,
    EPOLLONESHOT = 1u << 30,//只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
    EPOLLET = 1u << 31 //边缘触发
  };
```


