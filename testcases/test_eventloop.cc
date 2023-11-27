#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include <pthread.h>
#include "rocket/net/fd_event.h"
#include "rocket/net/eventloop.h"
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    rocket::Config::SetGlobalConfig("/home/yx/code/rocket/conf/rocket.xml");
    rocket::Logger::InitGlobLogger();

    rocket::EventLoop* eventloop = new rocket::EventLoop();
    //1.创建套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        ERRORLOG("listenfd = -1");
        exit(0);
    }
    //2.设置sockaddr_in结构体
    struct sockaddr_in sai;
    memset(&sai, 0, sizeof(sai));

    sai.sin_family = AF_INET;
    sai.sin_port = htons(12345);//端口主机字节序->网络字节序
    inet_aton("127.0.0.1", &sai.sin_addr);
    //3.绑定
    //先设置端口复用
    int reuse=1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    bind(listenfd, reinterpret_cast<sockaddr*>(&sai), sizeof(sai));

    //4.监听12345端口
    listen(listenfd, 100);
    //定义一个listenfd对应的FdEvent：文件描述符的读事件发生了表示有新的连接来了，就sccept
    rocket::FdEvent event(listenfd);
    event.listen(rocket::FdEvent::IN_EVENT, [listenfd](){
        //6.有新连接了，就accept获取通信cfd
        struct sockaddr_in clientaddr;
        memset(&clientaddr, 0, sizeof(clientaddr));
        socklen_t len = sizeof(clientaddr);
        int cfd = accept(listenfd, reinterpret_cast<sockaddr*>(&clientaddr), &len);
        DEBUGLOG("成功与%s建立连接", inet_ntoa(clientaddr.sin_addr));//网络字节序转点分十进制
    });
    //5.监听 连接fd 的可读事件，去执行读回调函数
    eventloop->addEpollEvent(&event);
    eventloop->loop();
    return 0;

}