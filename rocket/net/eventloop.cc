#include "rocket/net/eventloop.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <string.h>
#include <sys/eventfd.h>
#include "rocket/common/log.h"
#include "rocket/common/util.h"


#define ADD_TO_EPOLL() \
    auto it  = m_listen_fds.find(event->getFd());\
    int op = EPOLL_CTL_ADD;\
    if (it != m_listen_fds.end()){\
        op = EPOLL_CTL_MOD;\
    }\
    epoll_event tmp = event->getEpollEvent();\
    INFOLOG("事件类型 = %d", (int)tmp.events);\
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);\
    if (rt == -1)\
    {\
        ERRORLOG("failed epoll_ctl when add fd, errno=%d, error=%s", errno, strerror(errno));\
    }\
    m_listen_fds.insert(event->getFd()); \
    DEBUGLOG("ADD_TO_EPOLL成功, fd[%d]", event->getFd()) \


#define DELETE_TO_EPOLL() \
    auto it  = m_listen_fds.find(event->getFd());\
    if (it == m_listen_fds.end()){\
        return;\
    }\
    int op = EPOLL_CTL_DEL;\
    epoll_event tmp = event->getEpollEvent();\
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);\
    if (rt == -1)\
    {\
    }\



namespace rocket {

static thread_local EventLoop* t_current_eventloop = nullptr;
static int g_epoll_max_timeout = 10000;
static int g_epoll_max_events = 10;

EventLoop::EventLoop(){
    if (t_current_eventloop != nullptr)
    {
        ERRORLOG("创建eventloop失败");
        exit(0);
    }
    m_thread_id = getThreadId();
    //1.创建epoll实例，这里的10没有意义
    m_epoll_fd = epoll_create(10);
    //判断是否创建成功
    if (m_epoll_fd == -1){
        ERRORLOG("创建用于操作epoll实例的文件描述符失败");
        exit(0);
    }
    //2.准备使用eventfd进行事件通知，用于唤醒阻塞在epoll_wait处的eventloop线程
    initWakeUpFdEvent();
    initTimer();
    INFOLOG("线程%d创建eventloop成功", m_thread_id);

    t_current_eventloop = this;

}

EventLoop::~EventLoop(){
    close(m_epoll_fd);
    if (m_wakeup_fd_event)
    {
        delete m_wakeup_fd_event;
        m_wakeup_fd_event = NULL;
    }
    if (m_timer)
    {
        delete m_timer;
        m_timer = nullptr;
    }
    
    
}

void EventLoop::initTimer(){
    m_timer = new Timer();
    //这里不需要设置其可读事件的回调函数了，在Timer的构造函数里已经写好了
    addEpollEvent(m_timer);//监听m_timer的可读事件
}

void EventLoop::addTimerEvent(TimerEvent::s_ptr event){//event是TimerEvent类型的共享指针
    m_timer->addTimerEvent(event);
}



void EventLoop::initWakeUpFdEvent(){
    //成功：返回新的文件描述符. 失败：返回-1
    m_wakeup_fd = eventfd(0, EFD_NONBLOCK);//EFD_NONBLOCK表示读写时不阻塞，若遇到文件不可读写，返回-1
    if (m_wakeup_fd == -1){
        ERRORLOG("创建eventfd失败");
        exit(0);
    }

    m_wakeup_fd_event = new WakeUpFdEvent(m_wakeup_fd);
    
    m_wakeup_fd_event->listen(FdEvent::IN_EVENT, [this](){//定义了一个读回调函数
        DEBUGLOG("开始读唤醒");
        char buf[8];
        //读取发生错误或无数据可读了就退出，读取没错且有数据可读就继续
        while (read(m_wakeup_fd, buf, 8) != -1 && errno != EAGAIN){   
        }
        DEBUGLOG("读完了");
    });//监听其可读事件
    
    addEpollEvent(m_wakeup_fd_event);//监听m_wakeup_fd_event，专门用于唤醒阻塞在epoll_wait处的eventloop线程

}




void EventLoop::loop(){
    m_stop_flag = false;
    while (!m_stop_flag)
    {
        ScopeMutex<Mutex> lock(m_mutex);
        //1.从任务队列中取任务
        std::queue<std::function<void()>> tmp_tasks;
        //2.清空
        m_pending_tasks.swap(tmp_tasks);
        lock.unlock();
        //3.执行所有任务：所谓的任务是一些回调函数，函数有几种：一种是往epoll树上增删监视的事件、一种是具体Fdevent执行针对该事件触发时的回调函数
        while (!tmp_tasks.empty()){
            std::function<void()> cb = tmp_tasks.front();
            tmp_tasks.pop();
            //执行任务
            if (cb) cb();
        }
        
        int timeout = g_epoll_max_timeout;//阻塞时间
        //传出参数，保存发生了变化的fd
        epoll_event result_events[g_epoll_max_events];
        // DEBUGLOG("epoll_wait")
        //eventloop线程阻塞在此，继续执行有三种情况：要么有事件发生(wakeup是其中监听的m_wakeup_fd有事件发生)、要么阻塞时长到了、要么被其它线程唤醒
        int rt = epoll_wait(m_epoll_fd, result_events, g_epoll_max_events, timeout);
        // DEBUGLOG("epoll_wait 结束")
        if (rt < 0){
            ERRORLOG("epoll_wait error");
        }else{//rt是发生变化的文件描述符个数
            for (int i = 0; i < rt; i++)
            {
                //取出发生了变化的文件描述符，看看是触发的什么事件
                epoll_event trigger_event = result_events[i];
                FdEvent* fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);//void*的转换是使用static_cast
                //epoll_event结构体含events、data，events是监测的事件、data是一个联合体(一般保存fd，但这里保存的是FdEvent指针)
                if (fd_event == NULL){//当前事件无法处理
                    ERRORLOG("fd_event = NULL, continue");
                    continue;
                }
                //当前事件可以处理，怎么处理：把FdEvent里记录的 该事件处理函数 添加到队列里，等待下一次eventloop解决
                if (trigger_event.events & EPOLLIN){//如果是触发了读事件
                    DEBUGLOG("触发可读事件")

                    addTask(fd_event->handler(FdEvent::IN_EVENT));//取它的 读回调函数，将其放入队列中
                    DEBUGLOG("成功添加可读事件的回调函数")
                }
                if (trigger_event.events & EPOLLOUT){
                    DEBUGLOG("触发可写事件")
                    addTask(fd_event->handler(FdEvent::OUT_EVENT));
                }
                
            }
            
        }
        
    }
    

}
void EventLoop::stop(){
    m_stop_flag = true;
    wakeup();
}

void EventLoop::wakeup(){
    INFOLOG("WAKE UP");
    m_wakeup_fd_event->wakeup();
}


void EventLoop::addEpollEvent(FdEvent* event){
    //如果当前是执行eventloop的线程
    if (isInLoopThread())
    {
        // //看看之前是不是有添加过该事件的fd
        // auto it  = m_listen_fds.find(event->getFd());
        // //如果有添加过该事件的fd，那么就是执行修改，而不是执行添加
        // if (it != m_listen_fds.end()){
        //     op = EPOLL_CTL_MOD;
        // }
        // //对m_epoll_fd实例中的 event->getFd() 执行 op操作
        // epoll_event tmp = event->getEpollEvent();//获取到该fd需要监听的事件
        // int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);//tmp被拷贝到底层的树中
        // if (rt == -1)
        // {
        //     // ERRORLOG("addEpollEvent失败");
        // }
        
        //设置epoll实例中的该fd所要监听的事件
        ADD_TO_EPOLL();
        
    }else{
    //当前不是eventloop线程，那就只是将该任务添加到队列里
        auto cb = [this,event](){
            ADD_TO_EPOLL();
        };
        addTask(cb, true);//希望尽快
        
    }
    
}

void EventLoop::addTask(std::function<void()> cb, bool is_wake_up){
    ScopeMutex<Mutex> lock(m_mutex);
    m_pending_tasks.push(cb);
    lock.unlock();
    //如果想要唤醒eventloop线程，让其马上把队列里的任务执行完，就调用wakeup
    //也就是马上把需要监听的事件加入到epoll的事件表中。而不是eventloop线程阻塞在epoll_wait处，等待所监听的fd发生事件进入下一次循环
    if (is_wake_up)
    {
        wakeup();
    }
    
}


void EventLoop::deleteEpollEvent(FdEvent* event){
    //如果当前是执行eventloop的线程
    if (isInLoopThread())
    {
        // auto it  = m_listen_fds.find(event->getFd());
        // //如果本身就没有监听event->getFd()的event事件，就直接退出
        // if (it == m_listen_fds.end()){
        //     return;
        // }
        // int op = EPOLL_CTL_DEL;
        // //对m_epoll_fd实例中的 event->getFd() 执行 删除操作
        // epoll_event tmp = event->getEpollEvent();//获取到该fd需要监听的事件
        // int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);//tmp被拷贝到底层的树中
        // if (rt == -1)
        // {
        //     // ERRORLOG("addEpollEvent失败");
        // }
        DELETE_TO_EPOLL();
    }else{
        //当前不是eventloop线程，那就只是将该任务添加到队列里
        auto cb = [this,event](){
            DELETE_TO_EPOLL();
        };
        addTask(cb, true);
    }
    
}


bool EventLoop::isInLoopThread(){
    return getThreadId() == m_thread_id;
}

}

