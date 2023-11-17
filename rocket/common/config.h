#ifndef ROCKET_COMMON_CONFIG_H
#define ROCKET_COMMON_CONFIG_H

#include <map>
#include <string>

namespace rocket {

class Config {
public:
    Config(const char* xmlfile);//传入xml文件的路径
    static Config* GetGlobalConfig();
    static void SetGlobalConfig(const char* xmlfile);
public:
    std::string m_log_level;

};







}
#endif