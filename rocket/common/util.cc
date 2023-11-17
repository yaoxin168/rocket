#include "rocket/common/util.h"
#include <sys/syscall.h>

namespace rocket {

static int g_pid = 0;
static thread_local int g_thread_id = 0;//将变量声明为线程局部变量

pid_t getPid(){
    if (g_pid == 0)
    {
        g_pid = getpid();
    }
    return g_pid;
}

pid_t getThreadId(){
    if (g_thread_id == 0)
    {
        g_thread_id = syscall(SYS_gettid);//使用系统调用获取当前线程的线程ID
    }
    return g_thread_id;
}


}