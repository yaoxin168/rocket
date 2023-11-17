#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include <pthread.h>


void* fun(void*){
    DEBUGLOG("debug log %s", "11");
    INFOLOG("info log %s", "11");
    return nullptr;
}
int main()
{
    rocket::Config::SetGlobalConfig("/home/yx/code/rocket/conf/rocket.xml");
    rocket::Logger::InitGlobLogger();

    pthread_t thread;
    pthread_create(&thread, NULL, &fun, NULL);
    
    DEBUGLOG("debug log %s", "11");
    INFOLOG("info log %s", "11");
    pthread_join(thread, NULL);


    
    return 0;

}