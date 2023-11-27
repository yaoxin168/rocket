#include <unistd.h>
#include "rocket/net/wakeup_fd_event.h"
#include "rocket/common/log.h"

namespace rocket{


WakeUpFdEvent::WakeUpFdEvent(int fd): FdEvent(fd){//调用父类构造函数

}
WakeUpFdEvent::~WakeUpFdEvent(){
}


void WakeUpFdEvent::wakeup(){
    char buf[8] = {'a'};//1字节的数组
    
    int rt = write(m_fd, buf, 8);//向m_fd写入8字节数据
    if (rt != 8)
    {
        ERRORLOG("write to wakeupfd less than8 bytes, fd[%d]", m_fd);
    }
    

}



}