#pragma once 
//日志级别
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL    //致命错误
};
//日志级别字符串
inline const char* LogLevelToString(LogLevel level) {
    switch(level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }

}
