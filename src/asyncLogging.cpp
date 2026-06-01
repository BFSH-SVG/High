#include "asyncLogging.h"


namespace muduowebserv {
//构造
AsyncLogging::AsyncLogging():running_(false),file_(nullptr) {
    currentBuffer_.reset(new LogBuffer());
    nextBuffer_.reset(new LogBuffer());
}

AsyncLogging& AsyncLogging::instance() {
    static AsyncLogging instance;    //单例，只创建一次
    return instance;
}
void AsyncLogging::append(const char*data,size_t len) {
    std::lock_guard<std::mutex> lock(mutex_);
    if(len>LogBuffer::kBufferSize) {
        fprintf(stderr,"AsyncLogging: log line large\n");
        return;
    }
    if(currentBuffer_->available() >= len) {
        currentBuffer_->append(data,len);
        return;
    }else {
        //缓冲区满了，已入队列
        buffers_.push_back(std::move(currentBuffer_));
        if(nextBuffer_) {
            currentBuffer_ = std::move(nextBuffer_);
        }else {
            currentBuffer_.reset(new LogBuffer());   //备用没了，新建
        }
        //写入新缓冲
        currentBuffer_->append(data,len);
        //通知后台
        cond_.notify_one();
    }
}
//启动异步日志
void AsyncLogging::start() {
    if(running_) return;
    running_ = true;
    file_ = fopen("server.log","a");
    thread_ = std::thread(&AsyncLogging::threadFunc,this);
}

void AsyncLogging::stop() {
    running_ = false;
    cond_.notify_all();
    if(thread_.joinable()) {
        thread_.join();
    }
    if(file_) {
        fflush(file_);
        fclose(file_);
    }
}

//后台线程
void AsyncLogging::threadFunc() {
    std::vector<LogbufferPtr> buffersToWrite;   //后台队列
    LogbufferPtr newBuffer1(new LogBuffer());
    LogbufferPtr newBuffer2(new LogBuffer());
    while(running_) {
        {
            std::unique_lock<std::mutex>lock(mutex_);
            cond_.wait_for(lock,std::chrono::seconds(3),[this]{
                return !buffers_.empty()||!running_;
            });
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            if(nextBuffer_) {
                buffers_.push_back(std::move(nextBuffer_));
                nextBuffer_ = std::move(newBuffer2);
            }
            buffersToWrite.swap(buffers_);
        }
        //开始写文件
        for(const auto&buf:buffersToWrite) {
            fwrite(buf->data(),1,buf->length(),file_);
        }
        fflush(file_);
        //只保留两个缓冲区
        if(buffersToWrite.size()>2) {
            buffersToWrite.resize(2);
        }
        if(!newBuffer1) {
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }
        if(!newBuffer2 && !buffersToWrite.empty()) {
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }
        buffersToWrite.clear();
    }
    fflush(file_);
}
}