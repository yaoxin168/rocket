#ifndef ROCKET_NET_TIMEREVENT
#define ROCKET_NET_TIMEREVENT
#include <functional>
#include <memory>

namespace rocket{


class TimerEvent{

public:
    typedef std::shared_ptr<TimerEvent> s_ptr;

    TimerEvent(int interval, bool is_repeated, std::function<void()> cb);

    int64_t getArriveTime() const {
        return m_arrive_time;
    }

    void setCancled(bool value){
        m_is_cancled = value;
    }

    bool isCancled(){
        return m_is_cancled;
    }
    bool isRepeated(){
        return m_is_repeated;
    }
    std::function<void()> getCallBack(){
        return m_task;
    }
    void resetArriveTime();//对于重复执行的任务，在执行完后，重新计算其m_arrive_time
private:
    int64_t m_arrive_time;//这个任务想要在什么时候执行=当前时间+m_interval ms，由于时间戳时间可能比较长，这里使用int64
    int64_t m_interval;//时间间隔
    bool m_is_repeated {false};//是否重复执行
    bool m_is_cancled {false};//是否取消该定时任务
    std::function<void()> m_task;
};






}
#endif