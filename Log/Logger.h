#pragma once
#include "LogLevel.h"
#include "LogStream.h"
#include <string>
#include <ctime>

namespace muduowebserv {

class Logger {
public:
    //构造传入文件名和行号，日志级别
    Logger(const char*file,int line,LogLevel level);

    ~Logger(); //输出日志

    //获取日志流
    LogStream& stream() {
        return stream_;
    }
    //获取时间字符串
    static std::string getTimeString();
private:
    LogStream stream_;
    LogLevel level_;
    const char* file_;
    int line_;   //行号
};
//宏定义，方便使用
#define LOG_DEBUG Logger(__FILE__,__LINE__,LogLevel::DEBUG).stream()
#define LOG_ERROR Logger(__FILE__,__LINE__,LogLevel::ERROR).stream()
#define LOG_INFO Logger(__FILE__,__LINE__,LogLevel::INFO).stream()
#define LOG_WARN Logger(__FILE__,__LINE__,LogLevel::WARN).stream()
#define LOG_FATAL Logger(__FILE__,__LINE__,LogLevel::FATAL).stream()
}