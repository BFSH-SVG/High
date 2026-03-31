#pragma once 
#include <sys/socket.h>
#include <vector>
#include "eventLoop.h"
#include "netinet/in.h"
#include <unistd.h>  //close
#include <cstdio>  //perror
#include <functional>  //std::function
#include <memory> //std::unique_ptr
namespace muduowebserv{
    class Acceptor{
    public:
        Acceptor(EventLoop* loop,int port) :loop_(loop),
        listenFd_(-1),listenChannel_(nullptr),listening_(false)
        {
            //1.创建socket
            listenFd_ = socket(AF_INET,SOCK_STREAM,0);
            if( listenFd_<0) {
                perror("socket create failed");
            }
            //2.设置地址复用
            int opt = 1;
            setsockopt(listenFd_,SOL_SOCKET,SO_REUSEADDR,(const void*)&opt,sizeof(opt));
            //地址
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            addr.sin_port = htons(port);
            //3.绑定
            if(bind(listenFd_,(const struct sockaddr*)&addr,sizeof(addr))<0) {
                perror("bind failed!");
            }
        }
        //用户开始监听，创建channel并注册
        void startlisten() {
            listening_ = true;
            //创建channel，封装fd
            if(listen(listenFd_,SOMAXCONN)<0) {
                perror("listen failed!");
            }
            listenChannel_=std::make_unique<Channel>(listenFd_);
            //设置读回调
            listenChannel_->setReadCallback([this]() {
                struct sockaddr_in client_addr;
                socklen_t client_addrLen=sizeof(client_addr);
                int conFd = accept4(listenFd_,(struct sockaddr*)&client_addr,&client_addrLen,SOCK_NONBLOCK | SOCK_CLOEXEC);
                if(conFd>0) {
                    //新连接回调
                    if(newConnectCallback_) {
                        newConnectCallback_(conFd);  //回调给上层处理
                    }else {
                       close(conFd);
                    }
                }else {
                    perror("accept failed!");
                }
            });
            //启动监听
            listenChannel_->enableReading();
            //注册channel到loop
            loop_->updateChannel(listenChannel_.get()); //从智能指针获取裸指针
        }
        //设置新连接回调,启动连接的时候做什么
        void setnewConnectCallback(std::function<void(int)>callback) {
            newConnectCallback_ = callback;
        }
        ~Acceptor() {
            //关闭socket 
            if(listenFd_ >=0) {
                close(listenFd_);
            } 
            //channel由智能指针管理，无需手动
        }
    private:
       EventLoop* loop_ ;  //指向eventloop，用于注册channel
       int listenFd_;  //监听的fd
       std::unique_ptr<Channel> listenChannel_;   //封装成channel,监听新事件
       bool listening_;  //是否正在监听
       std::function<void(int)> newConnectCallback_;   //新连接回调函数

    };
}