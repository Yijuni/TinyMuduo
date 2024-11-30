#pragma once
#include <unistd.h>
#include <sys/syscall.h>
namespace CurrentThread{
    extern __thread int t_cachedTid;//__thread关键字，一个线程只创建一个此变量

    void cachedTid();
    /**
     * 获取当前线程tid，第一次执行该函数会调用cachedTid获取tid
     * 以后执行就直接返回tid值
     */
    inline int tid(){
        if(__builtin_expect(t_cachedTid==0,0)){//是否初始化线程id
            cachedTid();
        }
        return t_cachedTid;
    }

}