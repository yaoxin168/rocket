#ifndef ROCKET_NET_FDEVENT_H
#define ROCKET_NET_FDEVENT_H

#include <functional>
#include <sys/epoll.h>

namespace rocket{

class FdEvent{
public:
    //事件类型
    enum TriggerEvent{
        IN_EVENT = EPOLLIN,
        OUT_EVENT = EPOLLOUT
    };
    FdEvent();
    FdEvent(int fd);
    ~FdEvent();
    //事件处理函数，根据传入的事件类型event_type返回相应的回调函数
    std::function<void()> handler(TriggerEvent event_type);
    //事件监听函数，用于设置要监听的事件类型和对应的回调函数
    void listen(TriggerEvent event_type, std::function<void()> callback);
    //获取该事件的fd
    int getFd() const{
        return m_fd;
    }
    //获取该fd上需要监听哪些事件
    epoll_event getEpollEvent(){
        return m_listen_events;
    }

protected://设为protected是为了子类WakeUpFdEvent能访问到

    int m_fd {-1};//记录需要监听和处理的特定文件描述符

    epoll_event m_listen_events;//记录监听的事件类型
    std::function<void()> m_read_callback;//读回调函数
    std::function<void()> m_write_callback;//写回调函数

};



}
#endif