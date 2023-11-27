#ifndef ROCKET_COMMON_MUTEX_H
#define ROCKET_COMMON_MUTEX_H
#include <pthread.h>
#include <mutex>

namespace rocket {

//
template<class T>
class ScopeMutex{
public:
    //构造函数中加锁
    ScopeMutex(T& mutex):m_mutex(mutex){
        m_mutex.lock();
        m_is_lock = true;
    }
    //析构函数中解锁
    ~ScopeMutex(){
        m_mutex.unlock();
        m_is_lock = false;
    }
    //提供手动加锁、解锁的方法
    void lock(){
        if (!m_is_lock){
            m_mutex.lock();
        }
    }
    void unlock(){
        if (m_is_lock){
            m_mutex.unlock();
        }
    }
private:
    T& m_mutex;
    bool m_is_lock{false};
};

//创建动态互斥锁
class Mutex{
public:
    Mutex(){
        pthread_mutex_init(&m_mutex, NULL);
    }
    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }

    Mutex(const Mutex&) = delete;

    Mutex& operator=(const Mutex&) = delete;

    void lock(){
        pthread_mutex_lock(&m_mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }

private:
    pthread_mutex_t m_mutex;
};


}

#endif