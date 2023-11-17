#include "rocket/common/log.h"
#include <sys/time.h>//gettimeofday
#include "rocket/common/util.h"
#include <sstream>


namespace rocket{

static Logger* g_logger = nullptr;

Logger* Logger::GetGlobLogger(){
    
    return g_logger;

}


void Logger::InitGlobLogger(){
    //为了避免导致线程安全问题，导致创建多个Logger对象：解决方式是一开始就创建好(单例模式饿汉)
    //从Config中读取日志级别，转为LogLevel枚举类型
    printf("Init log\n");
    LogLevel global_log_level = StringToLevel(Config::GetGlobalConfig()->m_log_level);
    g_logger = new Logger(global_log_level);

}



//日志级别转字符串
std::string LogLevelToString(LogLevel level){
    switch (level)
    {
    case Debug:
        return "DEBUG";
    case Info:
        return "Info";
    case Error:
        return "Error";
    default:
        return "UNKNOWN";
    }
}

LogLevel StringToLevel(const std::string& log_level){
    if (log_level == "DEBUG") return Debug;
    if (log_level == "INFO") return Info;
    if (log_level == "ERROR") return Error;
    return UnKnown;
}



//打印日志
std::string LogEvent::toString(){
    //获取当前时间，存储在timeval结构体中
    struct timeval now_time;
    gettimeofday(&now_time, nullptr);//第二参数是用于存储时区信息，不需要

    //timeval转tm
    struct tm now_time_t;
    localtime_r(&(now_time.tv_sec), &now_time_t);//localtime_r线程安全，localtime不是线程安全的
    //tm转字符串
    char buf[128];
    strftime(buf, 128, "%y-%m-%d %H:%M:%S", &now_time_t);
    //字符串拼接上毫秒:char *转string，从timeval中取出毫秒的信息拼接上
    std::string time_str(buf);
    int ms = now_time.tv_usec / 1000;
    time_str = time_str + "." + std::to_string(ms);

    //获取线程号和进程号
    m_thread_id = getThreadId();
    m_pid = getPid();

    //创建字符串流对象，既能从字符串流中读取，又能输出到字符串流
    std::stringstream ss;
    ss << "[" << LogLevelToString(m_level) << "]\t"
        << "[" << time_str <<"]\t"
        << "[" << m_pid <<":"<<m_thread_id <<"]\t";
    //使用str()函数获取字符串流的内容
    return ss.str();

}


void Logger::pushlog(const std::string& msg){
    ScopeMutex<Mutex> lock(m_mutex);
    m_buffer.push(msg);
}

void Logger::log(){
    ScopeMutex<Mutex> lock(m_mutex);
    std::queue<std::string> tmp;
    m_buffer.swap(tmp);
    //m_buffer.swap(tmp)效率上优于std::swap(m_buffer, tmp)。但std::swap更通用,能适用到没有swap成员函数的场景。
    while (!tmp.empty()){
        std::string msg = tmp.front();
        tmp.pop();
        printf(msg.c_str());
    }
    
}



}