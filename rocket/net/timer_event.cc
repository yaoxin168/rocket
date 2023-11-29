#include "timer_event.h"
#include "rocket/common/util.h"
#include "rocket/common/log.h"

namespace rocket{

TimerEvent::TimerEvent(int interval, bool is_repeated, std::function<void()> cb)
    : m_interval(interval), m_is_repeated(is_repeated), m_task(cb){
    resetArriveTime();
}

void TimerEvent::resetArriveTime(){
    m_arrive_time = getNowMs() + m_interval;//下一次执行时间 = 当前时间ms + 时间间隔
    DEBUGLOG("成功创建定时任务,会在%lld时执行", m_arrive_time);
}




}