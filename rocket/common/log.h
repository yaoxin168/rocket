#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include <string>
#include <queue>
#include <memory>
#include <semaphore.h>
#include "rocket/common/config.h"
#include "rocket/common/mutex.h"

namespace rocket {


template<typename... Args>
std::string formatString(const char* str, Args&&... args){
    int size = snprintf(nullptr, 0, str, args...);//获取格式化字符串长度

    std::string result;
    if (size > 0)
    {
        result.resize(size);//调整字符串长度，多余就用\0填充，少了就截断
        snprintf(&result[0], size+1, str, args...);
    }
    
    return result;
}


//创建一个Debug级别的日志事件，使用toString将日志信息转为字符串，
//"__va__args__"是可变参数占位符，与格式化字符串str组成完整的字符串
//加上##作用：当可变参数的个数为0时，##可以把前面多余的”，“去掉，否则编译出错
//下面根据全局日志级别判断是否输出

#define DEBUGLOG(str, ...)\
    if(rocket::Logger::GetGlobLogger()->getLogLevel() <= rocket::Debug)\
    {\
        rocket::Logger::GetGlobLogger()->pushlog((rocket::LogEvent(rocket::LogLevel::Debug)).toString() \
        + "[" + std::string(__FILE__) +":"+std::to_string( __LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
        rocket::Logger::GetGlobLogger()->log();\
    }\


#define INFOLOG(str, ...)\
    if(rocket::Logger::GetGlobLogger()->getLogLevel() <= rocket::Info)\
    {\
        rocket::Logger::GetGlobLogger()->pushlog((rocket::LogEvent(rocket::LogLevel::Info)).toString() \
        + "[" + std::string(__FILE__) +":"+std::to_string( __LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
        rocket::Logger::GetGlobLogger()->log();\
    }\
    

#define ERRORLOG(str, ...)\
    if(rocket::Logger::GetGlobLogger()->getLogLevel() <= rocket::Error)\
    {\
        rocket::Logger::GetGlobLogger()->pushlog((rocket::LogEvent(rocket::LogLevel::Error)).toString() + "[" + std::string(__FILE__) +":"+std::to_string( __LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
        rocket::Logger::GetGlobLogger()->log();\
    }\

    
//日志级别
enum LogLevel {
    UnKnown = 0,
    Debug = 1,
    Info = 2,
    Error = 3
};

//日志器：提供打印日志的方法、设置日志输出的路径
class Logger {
public:
    typedef std::shared_ptr<Logger> s_ptr;

    Logger(LogLevel level):m_set_level(level){};
    //将日志msg送入队列
    void pushlog(const std::string& msg);
    //队列pop打印日志
    void log();
    //获取日志器级别
    LogLevel getLogLevel() const {
        return m_set_level;
    }

public:
    static Logger* GetGlobLogger();//获取日志器对象，使用xml文件里的级别初始化
    static void InitGlobLogger();//初始化日志对象，而不是在GetGlobLogger中初始化，避免线程不安全

private:
    LogLevel m_set_level;
    std::queue<std::string> m_buffer;//log和push_log时会多个线程访问它，需要加锁
    Mutex m_mutex;
};


//日志级别转字符串的方法
std::string LogLevelToString(LogLevel level);
//字符串转日志级别的方法
LogLevel StringToLevel(const std::string& log_level);

//日志事件
class LogEvent {
public:

    LogEvent(LogLevel level):m_level(level){};

    std::string getFileName() const{
        return m_file_name;
    }
    LogLevel getLogLevel() const{//获取日志事件级别
        return m_level;
    }

    std::string toString();//打印日志


private:
    std::string m_file_name;//文件名
    int32_t m_file_line;//行号
    int32_t m_pid;//进程号
    int32_t m_thread_id;//线程号

    LogLevel m_level;//日志事件级别

};




}




#endif