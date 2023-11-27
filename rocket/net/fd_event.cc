#include "rocket/net/fd_event.h"
#include "rocket/common/log.h"
#include <string.h>
namespace rocket {

FdEvent::FdEvent(int fd):m_fd(fd){
    memset(&m_listen_events, 0, sizeof(m_listen_events));
}

FdEvent::~FdEvent(){
}

std::function<void()> FdEvent::handler(TriggerEvent event){
    //如果是读事件，就返回读回调函数
    if (event == TriggerEvent::IN_EVENT)
    {
        return m_read_callback;
    }else{
        return m_write_callback;
    }
    return nullptr;
}

void FdEvent::listen(TriggerEvent event_type, std::function<void()> callback){
    //如果是读事件，就记录到epoll_event对象里，回调函数也记录下来
    if (event_type == TriggerEvent::IN_EVENT)
    {
        m_listen_events.events |= EPOLLIN;
        m_read_callback = callback;
    }else{//写事件
        m_listen_events.events |= EPOLLOUT;
        m_write_callback = callback;
    }
    m_listen_events.data.ptr = this;//利用epoll_event结构体的联合体对象data存储指针，就能在loop能取到该指针

}

}