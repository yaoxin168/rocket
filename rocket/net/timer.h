#ifndef ROCKET_NET_TIMER_H
#define ROCKET_NET_TIMER_H

#include <map>
#include "rocket/common/mutex.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/timer_event.h"


namespace rocket{

class Timer:public FdEvent{

public:
    Timer();

    ~Timer();

    void addTimerEvent(TimerEvent::s_ptr event);
    void deleteTimerEvent(TimerEvent::s_ptr event);
    void onTimer();//当发生可读事件后,eventloop执行该函数
private:
    void resetArriveTime();//设置定时器描述符的可读事件触发时间为 multimap中最前面的任务的m_arrive_time
private:
    std::multimap<int64_t, TimerEvent::s_ptr> m_pending_events;//存储所有定时器任务
    Mutex m_mutex;//用于增删TimerEvent
};


}


#endif