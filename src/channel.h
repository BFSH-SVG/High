#pragma once 
#include <iostream>
#include <functional>
#include <sys/epoll.h>

namespace muduowebserv {
    class Channel {
    public:
        Channel(int fd):fd_(fd),events_(0),revents_(0) {

        }

        int fd() const{
            return fd_;
        }
        int events() const {
            return events_;
        }
        //设置回调函数，因为四个读写回调是私有的，外部只能通过这个调用
        void setReadCallback(std::function<void()>readcallback) {
            readCallback_ = readcallback;
        } 
        void setWriteCallback(std::function<void()>writecallback) {
            writeCallback_ = writecallback;
        }
        void setcloseCallback(std::function<void()>closecallback) {
            closeCallback_ = closecallback;
        }
        void setErrorCallback(std::function<void()>errorcallback) {
            errorCallback_ = errorcallback;
        }
        //处理函数
        void handleEvent() {  //事件分发
            //先执行错误和关闭
            if(revents_ & EPOLLERR) {
                if(errorCallback_) {
                    errorCallback_();
                }
            }
            if(revents_ & EPOLLHUP) {  //关闭
                if(closeCallback_) {
                    closeCallback_();
                    return;  //退出
                }
            }
            if(revents_ & EPOLLIN) {
                if(readCallback_) {
                    readCallback_();
                }
            }
            if(revents_ & EPOLLOUT) {
                if(writeCallback_) {
                    writeCallback_();
                }
              
            }
        }
        //设置实际发生的事情(epoll调用) 
        void setRevents(int revents) {
            revents_ = revents;
        }
        //开始监听
        void enableReading() {
            events_ |= EPOLLIN;    //0000 | 0001
            //TODO
        }
        void enableWrite() {       // 0010 0100
            events_ |= EPOLLOUT; 
        }

        void  disableAll() {  //禁用所有事件监听
            events_ = 0;   //关闭连接
        }
        void disableReading() {
            events_ &= ~EPOLLIN;
        }
        void disableWriting() {    //禁用写事件监听
            events_ &= ~EPOLLOUT;
        }
        bool isReading() const{
           return  events_ & EPOLLIN;
        }
        bool isWriting() const{
            return events_ & EPOLLOUT;   
        }

        int getIndex() const {
            return index_; //获取状态
        }
        void setIndex(int index) {
            index_ = index;  //设置状态
        }
        //判断是否没有事件
        bool isNoneEvent() const {
            return events_ == 0;
        }
    private:
        int fd_;   //文件描述符
        int events_;   //监听事件
        int revents_;   //就绪事件
        int index_ = 0;  //在epoll中的状态
        std::function<void()>readCallback_;   //读回调
        std::function<void()>writeCallback_;  //写回调
        std::function<void()>closeCallback_;  //关闭回调
        std::function<void()>errorCallback_;  //错误回调
    };






}