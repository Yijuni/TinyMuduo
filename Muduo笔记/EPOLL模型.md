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
#### 触发时机
1. **EPOLL_OUT**

    1. **客户端连接场景**：客户端成功连接到服务端后，得到的文件描述符（fd）被添加到 `epoll` 事件池中，由于连接是可写的，会触发 `EPOLLOUT` 事件。

    2. **客户端发包场景**：当缓冲区从满变为不满时，会触发 `EPOLLOUT` 事件。这通常发生在数据包发送逻辑中，当数据包被发送到内核缓冲区，然后由内核再将缓冲区的内容发送出去。如果 `send` 操作部分成功，表示缓冲区满了，那么剩下的数据需要等待 `EPOLLOUT` 事件发生后再继续发送。

    3. **重新注册 `EPOLLOUT` 事件**：如果连接可用，且缓冲区不满的情况下，调用 `epoll_ctl` 将文件描述符重新注册到 `epoll` 事件池（使用 `EPOLL_CTL_MOD`），这时也会触发 `EPOLLOUT` 事件。

    4. **边缘触发模式下的状态变化**：在边缘触发（ET）模式下，`EPOLLOUT` 事件只有在发送缓冲区状态从不可写变为可写时，才会触发一次。这意味着，除非发送缓冲区的状态发生了变化，否则 `EPOLLOUT` 不会重复触发。

    5. **非阻塞socket和EAGAIN错误配合使用**：在非阻塞socket的情况下，`EPOLLOUT` 事件与 `EAGAIN` 错误码配合使用。如果某次 `write` 操作写满了发送缓冲区，并返回 `EAGAIN` 错误码，或者对端读取了一些数据，使得缓冲区重新可写，此时会触发 `EPOLLOUT` 事件。

    总结来说，`EPOLLOUT` 事件通常在发送缓冲区状态发生变化时触发，特别是在边缘触发模式下，这种变化是从不可写状态到可写状态的转变。在水平触发模式下，只要缓冲区可写，`EPOLLOUT` 就会持续触发。理解这些触发条件对于有效地使用 `epoll` 进行事件驱动编程至关重要。
2. **EPOLLHUP**

    1. **本端描述符产生一个挂断事件**：当本地端（server端）的socket被挂断时，会触发 `EPOLLHUP` 事件。这通常意味着本端的socket连接已经无法进行读写操作。

    2. **本端socket关闭或关闭读端**：`EPOLLHUP` 也可以在本端socket关闭或者关闭读端时被触发。

    3. **与 `shutdown` 函数调用相关**：如果本端调用了 `shutdown(SHUT_RDWR)`，这会导致 `EPOLLHUP` 事件被触发。需要注意的是，不能在调用 `close` 之后触发 `EPOLLHUP`，因为 `close` 之后文件描述符已经失效。

    4. **默认注册的事件**：`EPOLLHUP` 是默认注册的事件之一，因此在注册epoll事件时不需要特别注册它。

    5. **与 `EPOLLERR` 事件一起触发**：在某些情况下，本端socket出现问题时，`EPOLLHUP` 和 `EPOLLERR` 事件可能会同时触发。

    6. **对端正常关闭不触发**：对端正常关闭连接（如程序里调用 `close()`，或在shell下使用 `kill` 或 `Ctrl+C`）会触发 `EPOLLIN` 和 `EPOLLRDHUP`，但不会触发 `EPOLLERR` 和 `EPOLLHUP`。

    `EPOLLHUP` 事件是epoll机制中用于处理连接挂起情况的重要事件，正确处理该事件对于维护稳定的网络连接非常关键。


### 常见错误码
1. **EPIPE**

    `EPIPE` 是一个错误码，它表示在连接导向的socket（如TCP）上，本地端已被关闭。当尝试向已经关闭的管道或socket写入数据时，会触发这个错误。具体来说，以下几种情况下会触发 `EPIPE` 错误：

    1. **在CLOSE_WAIT状态的连接上发送数据**：当客户端已经关闭了连接，服务器端如果尝试发送数据，会触发 `EPIPE` 错误。

    2. **本端socket上已经调用过shutdown(SEND_SHUTDOWN)**：如果在本端的socket上已经调用了 `shutdown` 函数，设置了 `SEND_SHUTDOWN` 选项，那么再尝试发送数据时会触发 `EPIPE` 错误。

    3. **对方socket中断**：如果在发送数据之前，对方的socket连接已经中断或关闭，发送端的 `write` 操作会返回 `-1`，并且 `errno` 会被设置为 `EPIPE`（错误码 32），表示“Broken pipe”（管道破裂）。

    4. **忽略SIGPIPE信号**：如果程序忽略了 `SIGPIPE` 信号，那么在发生 `EPIPE` 错误时，程序不会因此而退出，而是可以通过检查 `errno` 来发现 `EPIPE` 错误。

    5. **SIGPIPE信号的默认行为**：如果没有特别处理 `SIGPIPE` 信号，当发生 `EPIPE` 错误时，进程会接收到 `SIGPIPE` 信号，其默认行为是终止进程。

    因此，`EPIPE` 错误通常与管道或socket的写入端在没有读取端的情况下被关闭有关。处理 `EPIPE` 错误时，可以捕获 `SIGPIPE` 信号并进行相应的错误处理，或者在写入操作时检查返回值和 `errno`，以确定是否发生了 `EPIPE` 错误，并据此进行相应的程序逻辑处理。

2. **ECONNRESET**

    `ECONNRESET` 错误是在网络编程中常见的错误，它表示连接被对方重置，也就是说在读取数据时连接被意外关闭。这种错误通常发生在以下几种情况：

    1. **接收端关闭连接**：当接收端（如服务器）调用 `recv` 或 `read` 操作时，如果对端已经关闭连接，这些操作可能会返回 `ECONNRESET` 错误。

    2. **对端重启连接**：在连接还未完全建立时，如果对端重启了连接，也可能会触发 `ECONNRESET` 错误。

    3. **发送端断开连接**：如果发送端已经断开连接，但是调用 `send` 操作时，可能会触发这个错误。

    4. **网络问题**：可能是网络连接不稳定或中断导致连接被重置。

    5. **服务器问题**：服务器端可能在处理请求期间发生了错误，导致主动关闭了连接。

    6. **超时**：如果连接在长时间没有数据传输时被关闭，可能是由于超时设置导致的。
3. **EWOULDBLOCK**

    `EWOULDBLOCK`（在某些系统上也称为 `EAGAIN`）是一个与非阻塞 I/O 操作相关联的错误码，它表明尝试进行的 I/O 操作不能立即完成，如果操作是在阻塞模式下进行的，那么它将会阻塞。以下是关于 `EWOULDBLOCK` 的一些关键点：

    1. **非阻塞模式下的行为**：在非阻塞模式下，当尝试进行读（`read`）或写（`write`）操作时，如果数据不可立即读取或写缓冲区已满，操作会立即返回 `-1`，并且 `errno` 被设置为 `EWOULDBLOCK` 或 `EAGAIN`。这意味着操作会阻塞，但由于设置了非阻塞模式，所以操作不会真的阻塞，而是返回错误码提示资源暂时不可用。

    2. **处理方法**：对于非阻塞socket，当遇到 `EWOULDBLOCK` 或 `EAGAIN` 错误时，通常的做法是忽略这个错误，并在下一次循环中重试操作。这是因为在非阻塞模式下，`read` 或 `write` 操作可能需要多次尝试才能成功。

    3. **与阻塞模式的区别**：在阻塞模式下，`read` 或 `write` 操作会挂起进程直到数据可用或操作完成。而在非阻塞模式下，如果操作不能立即完成，会返回 `EWOULDBLOCK` 或 `EAGAIN`，提示程序稍后再试。

    4. **信号中断（`EINTR`）**：`EWOULDBLOCK` 和 `EINTR` 是两种不同的错误情况。`EINTR` 表示系统调用被信号中断，而 `EWOULDBLOCK` 表示操作会阻塞，但由于非阻塞模式，所以立即返回。

    5. **边缘触发（ET）模式下的注意事项**：在使用 `epoll` 的边缘触发（ET）模式时，需要特别小心处理 `EWOULDBLOCK` 或 `EAGAIN`，因为 ET 模式下事件只会被触发一次，直到状态发生变化。这意味着如果一次 `write` 操作没有完全清空发送缓冲区，你需要继续注册可写事件，否则剩余的数据可能永远不会被发送。

    6. **超时设置**：对于阻塞socket，可以通过设置超时（`SO_RCVTIMEO` 和 `SO_SNDTIMEO`）来避免永久阻塞的情况，当超时发生时，`recv` 或 `send` 会返回 `-1`，并且 `errno` 可能被设置为 `EWOULDBLOCK` 或 `EAGAIN`。

    总结来说，`EWOULDBLOCK` 是非阻塞 I/O 操作中常见的一个错误码，它提示程序操作不能立即完成，需要稍后再试。正确处理 `EWOULDBLOCK` 对于编写高效和稳定的网络程序至关重要。




