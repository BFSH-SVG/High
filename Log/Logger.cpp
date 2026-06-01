#include <iostream>
#include "Logger.h"
#include "../src/asyncLogging.h"
#include <sstream>
namespace muduowebserv {

//构造函数
Logger::Logger(const char* file,int line,LogLevel level)
    :file_(file),line_(line),level_(level) {

    }

//析构函数
Logger::~Logger() {
    //输出格式 [时间] [日志级别] [文件名:行号] [日志内容]
    std::ostringstream oss;
    oss << "[" <<getTimeString()<<"]"
              <<"["  <<LogLevelToString(level_) <<']'
              <<"[" <<file_ <<": "<<line_<<"] "
              <<stream_.data()<<"\n"; 
    std::string line = oss.str();    //转化成字符串
    AsyncLogging::instance().append(line.c_str(),line.size());          
}
std::string Logger::getTimeString() {
    time_t now = time(0);   //获取当前时间
    struct tm t;
    localtime_r(&now,&t);  //转化为本地时间
    char buf[64]; 
    snprintf(buf,sizeof(buf),"%4d-%02d-%02d %02d:%02d:%02d",
            t.tm_year+1900,t.tm_mon+1,t.tm_mday,
            t.tm_hour,t.tm_min,t.tm_sec);
    return std::string(buf);  //返回字符串
}

}
