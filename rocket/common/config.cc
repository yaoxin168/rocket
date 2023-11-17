// #include <tinyxml/tinyxml.h>
#include "tinyxml.h"
#include "rocket/common/config.h"
#include "config.h"
#include <string>

//定义宏用于遍历节点:读parent节点下的name节点
//##连接操作符：将两个标记连接在一起
//#字符串化操作符：将该参数转换为一个以双引号括起来的字符串
#define READ_XML_NODE(name, parent)\
TiXmlElement* name##_node = parent->FirstChildElement(#name);\
if (!(name##_node)){\
    printf("%s节点读取失败\n", #name);\
    exit(0);\
}\

//定义宏用于读节点的值
#define READ_STR_FROM_XML_NODE(name, parent)\
TiXmlElement* name##_node = parent->FirstChildElement(#name);\
if (!name##_node || !name##_node->GetText()){\
    printf("空的\n");\
    exit(0);\
}\
std::string name##_str = std::string(name##_node->GetText());\




namespace rocket{

static Config* g_config = NULL;

Config* Config::GetGlobalConfig(){
    return g_config;
}

void Config::SetGlobalConfig(const char* xmlfile){
    if (g_config == nullptr)
    {
        g_config = new Config(xmlfile);
    }
}



Config::Config(const char *xmlfile){
    //1.读取文件
    TiXmlDocument* xml_document = new TiXmlDocument();
    bool rt = xml_document->LoadFile(xmlfile);
    if (!rt){
        printf("配置文件读取失败\n");
        exit(0);
    }
    //2.遍历节点
    READ_XML_NODE(root, xml_document);
    READ_XML_NODE(log, root_node);
    
    //读出loglevel
    READ_STR_FROM_XML_NODE(log_level, log_node)

    m_log_level = log_level_str;
}


}


