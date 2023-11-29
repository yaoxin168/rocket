#ifndef ROCKET_NET_EVENTLOOP_H
#define ROCKET_NET_EVENTLOOP_H

#include <pthread.h>
#include <queue>
#include <set>
#include <functional>
#include "rocket/common/mutex.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/wakeup_fd_event.h"
#include "rocket/net/timer.h"

namespace rocket {
//事件循环：不断地监听事件队列，当有事件发生时就执行相应的回调函数
class EventLoop{
public:
    EventLoop();
    ~EventLoop();

    void loop();

    void wakeup();

    void stop();

    void addEpollEvent(FdEvent* event);

    void deleteEpollEvent(FdEvent* event);

    bool isInLoopThread();//判断当前线程是不是执行事件循环的线程

    void addTask(std::function<void()> cb, bool is_wake_up = false);//当前线程不是执行eventloop的线程，
    //那么就先把任务添加到队列里，等这个线程自己从epoll_wait返回后，才执行这个任务，而不是由其它线程来执行

    //调用m_timer的添加任务方法
    void addTimerEvent(TimerEvent::s_ptr event);

private:
    void dealWakeup();

    //监听m_listen_fds，专门用于唤醒eventloop线程，因为eventloop线程可能因为所监听的fd都没有事件发生而阻塞一段时间，
    //就可以调用wakeup触发m_listen_fds的事件
    void initWakeUpFdEvent();

    void initTimer();//初始化m_timer

private:
    pid_t m_thread_id {0};//记录执行eventloop的线程的id
    bool m_stop_flag {false};//是否停止loop
    int m_wakeup_fd;//用于唤醒epoll_wait的eventfd，即WakeUpFdEvent事件的专属fd
    std::set<int> m_listen_fds;//当前监听的所有套接字：保存eventfd
    int m_epoll_fd {0};//epoll实例的文件描述符
    WakeUpFdEvent* m_wakeup_fd_event{NULL};
    std::queue<std::function<void()>> m_pending_tasks;//所有待执行的任务队列：从中取任务时需要加锁
    Timer* m_timer{nullptr};//定时器：用于定时执行任务，继承自FdEvent
    Mutex m_mutex;

};

}

#endif