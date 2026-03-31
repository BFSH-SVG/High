#pragma once 
#include <cstring>   //memcpy
#include <iostream>
namespace muduowebserv{

    class LogStream{
    public:
        static const int kBufferSize = 4096; //缓冲区大小
        //构造函数，进行初始化
        LogStream(): cur_pos_(0) {

        }
        //追加数据到缓冲区
        void append(const char*data,size_t len) {
            if(cur_pos_ + len <kBufferSize) {
                memcpy(buffer_ +cur_pos_,data,len);
                //跟新最新位置
                cur_pos_ += len;
            } else {
                std::cerr<<"buffer is full"<<std::endl;
            }
            //添加'\0'
            buffer_[cur_pos_] = '\0';
        }
        //获取已经写入长度长度
        size_t getLength() const {
            return cur_pos_;
        }
        //获取缓冲区数据
        const char* data() const {
            return buffer_;
        }
        //清空缓冲区数据
        void clearBuffer() {
            // cur_pos_ = 0;
            // memset(buffer_,0,kBufferSize);
            memset(buffer_,0,cur_pos_);
            cur_pos_ = 0;
        }
        //输出重载,支持字符串
        LogStream& operator<<(const char* data) {
            append(data,strlen(data));
            return *this;
        }
        //重载整数
        LogStream& operator<<(int data) {
            char buf[32];
            //将整数转化为字符串
            snprintf(buf,sizeof(buf),"%d",data);
            append(buf,strlen(buf));
            return *this;
        }
    private:
        char buffer_[kBufferSize];  
        int cur_pos_;  //当前位置
    };

}