#pragma once
#include <mutex>
#include <thread>
#include <vector>
#include <cstring>
#include <memory>
#include <cstdio>
#include <string>
#include <condition_variable>
#include <chrono>
namespace muduowebserv {
//缓冲区类
class LogBuffer{
public:
    static const size_t kBufferSize = 4000*1000;
    LogBuffer():cur_(0){}
    bool append(const char*buffer,size_t len) {
        if(available() >= len) {
            memcpy(data_+cur_,buffer,len);
            cur_ += len;
            return true;
        }
        return false;
    }
    const char* data() const {return data_;}  //获取缓冲区开始地址
    size_t length() const {return cur_;}
    size_t available() const {return kBufferSize - cur_;}  //可用空间
    void reset() {cur_ = 0;}   //重置缓冲区
private:
    char data_[kBufferSize];
    size_t cur_; //当前缓冲区字节数
};

class AsyncLogging {
public:


    static AsyncLogging& instance();
    void append(const char*data,size_t len);
    void start();
    void stop();

private:
    std::mutex mutex_;   //互斥锁
    using LogbufferPtr = std::unique_ptr<LogBuffer>;
    LogbufferPtr currentBuffer_;  //正在写入缓冲区
    LogbufferPtr nextBuffer_;   //备用缓冲区
    std::thread thread_;
    bool running_;   //后台现场是否启动
    std::string filename_;  //日志文件名
    FILE* file_;   //文件指针
    std::condition_variable cond_;   //条件变量
    std::vector<LogbufferPtr> buffers_;  //队列

    //构造函数
    AsyncLogging();
    void threadFunc();

};



}









