#include <sys/timerfd.h>
#include "rocket/net/timer.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"
#include <errno.h>
#include <string.h>

namespace rocket{

Timer::Timer():FdEvent()//先调用父类构造函数
{
    //1.创建定时器描述符
    //CLOCK_MONOTONIC表示一个单调递增的时钟，不受系统时间的更改或调整的影响
    m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    DEBUGLOG("timer fd=%d", m_fd);

    //给该定时器 设置可读事件的监听函数
    listen(FdEvent::IN_EVENT, std::bind(&Timer::onTimer, this));
}

Timer::~Timer(){
    

    
}
//触发可读事件后执行的回调函数
void Timer::onTimer(){
    //1.读出 定时器fd读缓冲区 的8字节数据，表示定时器超时的次数
    char buf[8];
    while (1)
    {
        //把缓冲区数据读完，防止因为epoll默认是水平触发模式，一直通知该fd已就绪
        if ((read(m_fd, buf, 8) == -1) && errno == EAGAIN)//EAGAIN就是表示没有数据可读
        {
            break;
        }
    }
    //2.执行定时任务
    int64_t now = getNowMs();
    std::vector<TimerEvent::s_ptr> tmps;
    std::vector<std::pair<int64_t, std::function<void()>>> tasks;//需要执行的任务
    ScopeMutex<Mutex> lock(m_mutex);
    //遍历任务，到期了的任务全部执行
    auto it = m_pending_events.begin();
    for(;it != m_pending_events.end(); ++it){
        //任务已到期
        if (it->first <= now)
        {
            if (!(it->second->isCancled()))
            {
                tmps.push_back(it->second);
                tasks.push_back(std::make_pair(it->second->getArriveTime(), it->second->getCallBack()));
            }
        }else{//没有任务到期
            break;
        }
    }
    //3.删掉上面执行了的任务
    //erase(iterator first, iterator last)：删除位于区间 [first, last) 内的所有元素，并返回指向被删除元素之后位置的迭代器
    m_pending_events.erase(m_pending_events.begin(), it);
    lock.unlock();

    //4.把需要重复执行的任务再次添加
    for(auto i = tmps.begin();i != tmps.end();++i){
        if ((*i)->isRepeated())
        {
            //依据现在的时间调整任务的m_arrive_time
            (*i)->resetArriveTime();
            addTimerEvent(*i);
        }
    }

    resetArriveTime();

    //真正执行任务，上面只是将需要执行的任务放在tasks容器中
    for(auto i:tasks){
        if (i.second)
        {
            i.second();
        }
        
    }

}

void Timer::resetArriveTime(){
    ScopeMutex<Mutex> lock(m_mutex);
    auto tmp = m_pending_events;
    lock.unlock();
    //如果当前任务队列是空的，就不用设置了
    if (tmp.empty())
    {
        return;
    }
    
    int64_t now = getNowMs();//获取当前时间
    auto it = tmp.begin();//取第一个定时任务
    int64_t interval = 0;
    if (it->second->getArriveTime() > now)
    {
        interval = it->second->getArriveTime() - now;
    }else{
        //已经有任务过时了，100ms后触发可读事件，去执行过时的任务
        interval = 100;//ms
    }

    itimerspec value;
    memset(&value, 0, sizeof(value));
    value.it_value.tv_nsec = (interval % 1000) * 1000000;//interval % 1000是为了取得毫秒部分。毫秒转纳秒
    value.it_value.tv_sec = interval / 1000;
    int rt = timerfd_settime(m_fd, 0, &value, nullptr);
    if (rt != 0)
    {
        ERRORLOG("timerfd_settimer error");
    }
    DEBUGLOG("定时器时间间隔重置为 %lld", now+interval);
}
/*
增加任务：先看增加的任务是不是早于map里所有任务
如果是：定时器的间隔时间设为100ms，即很快就去执行这个已过期任务
如果不是：直接将该任务添加到map中，map维持着以最小时间间隔触发可读事件(然后去执行onTimer())
*/
void Timer::addTimerEvent(TimerEvent::s_ptr event){
    bool is_reset_timerfd = false;

    ScopeMutex<Mutex> lock(m_mutex);
    if (m_pending_events.empty())
    {
        is_reset_timerfd = true;
    } else{
        //获取map中的第一个任务，由于map有自动排序的特点，当前这个任务的时间就是所有任务的最早执行时间
        auto it = m_pending_events.begin();
        //如果该时间 比 新添加的这个定时任务时间大，就说明要重设到期时间为100ms后，
        //即快速的去处理这个过期任务，而不是按照以前map中最早任务的时间来，这样的话可能要等很久
        if ((*it).second->getArriveTime() > event->getArriveTime())
        {
            is_reset_timerfd = true;
        }
    }
    //正式将该事件插入
    m_pending_events.emplace(event->getArriveTime(), event);
    lock.unlock();
    //重设到期时间
    if (is_reset_timerfd)
    {
        resetArriveTime();
    }
    
}

void Timer::deleteTimerEvent(TimerEvent::s_ptr event){
    //先设置取消该任务，避免。。。。。。。
    event->setCancled(true);

    ScopeMutex<Mutex> lock(m_mutex);
    auto begin = m_pending_events.lower_bound(event->getArriveTime());//查找第一个不小于给定值的元素的位置
    auto end = m_pending_events.upper_bound(event->getArriveTime());//查找第一个大于给定值的元素的位置
    //在这段时间的任务中遍历，找到该event任务进行删除
    auto it = begin;
    for (it = begin;it != end; ++it)
    {
        if (it->second == event)
        {
            break;
        }
    }
    if (it != end)
    {
        m_pending_events.erase(it);
    }
    lock.unlock();
    DEBUGLOG("success delete TimerEvent");
}

}



