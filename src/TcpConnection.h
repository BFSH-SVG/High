#pragma once
#include "eventLoop.h"
#include <string>
#include <memory>
#include <unistd.h>  //close
#include "channel.h"  //chanel类需要
#include <iostream>
#include <sys/socket.h> //socket.get
#include <functional>
#include <netinet/in.h> //sockaddr_in
#include <err.h> //perror
#include "buffer.h"
namespace muduowebserv {
    enum ConnectionState {
        kConnected,
        kDisconnected
    };
    class TcpConnection;
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&,Buffer*)>; 
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
    class TcpConnection:public std::enable_shared_from_this<TcpConnection> {
    public:
        TcpConnection(EventLoop* loop,int fd,const std::string& name)
        :loop_(loop),name_(name),sockfd_(fd)
        {
            //设置channel_回调
            channel_ = std::make_unique<Channel>(sockfd_);
            //设置不同的回调
            channel_->setReadCallback([this]() {
                handleRead();
            });
            channel_->setWriteCallback([this](){
                handleWrite();
            });
            channel_->setcloseCallback([this]() {
                handleClose();   
            });
            channel_->setErrorCallback([this](){
                handleError();
            });
        }
        //注册fd
        void connectEstablished() {
            std::cout<<"connection called"<<std::endl;
            channel_->enableReading();
            loop_->updateChannel(channel_.get());
            if(connectionCallback_) {
                connectionCallback_(shared_from_this());
            }
        }
        std::string name() const {
            return name_;
        }
        void setConnectionCallback(const ConnectionCallback& callback) {
            connectionCallback_ = callback;
        }
        void setMessageCallback(const MessageCallback& callback) {
            messageCallback_ = callback;
        }
        void setCloseback(const CloseCallback& callback) {
            closeCallback_ = callback;
        }
        //发送信息
        void send(const std::string& message) {
            if(state_== kDisconnected) {
                return;
            }
            if(loop_->isInLoopThread()) {
                outputBuffer_.append(message.c_str(),message.size());
                flushputBuffer();
            }else {
                    loop_->runInloop([self=shared_from_this(),message]() {
                    //判断是否是连接状态再继续,io线程可能中途关闭连接
                    if(self->state_ == kConnected) {
                        self->outputBuffer_.append(message.c_str(),message.size());
                        self->flushputBuffer();
                    }
                    });
            }
        }

        ~TcpConnection() {
            //关闭fd
            ::close(sockfd_);
        }

    private:
        EventLoop* loop_;  //注册fd
        int sockfd_; 
        std::string name_;
        std::unique_ptr<Channel> channel_;  //channel_负责fd的读写事件
        Buffer inputBuffer_;  //输入缓冲区
        Buffer outputBuffer_; //输出缓冲区
        ConnectionCallback connectionCallback_;  //连接建立，或者断开连接
        MessageCallback messageCallback_; //消息回调
        CloseCallback closeCallback_;  //连接关闭回调
        std::atomic<ConnectionState> state_=kConnected;  //连接状态
        //处理四种回调
        void handleError() {
            int optval;
            socklen_t optlen=sizeof(optval);
            if(getsockopt(sockfd_,SOL_SOCKET,SO_ERROR,&optval,&optlen)<0) {
                perror("getsockopt error");
            }else {
                std::cout<<"handError["<<name_<<"]"<<optval<<std::endl;
            }
        }
        void handleClose() {
            if(state_==kDisconnected) {
                return;
            }
            std::cout<<"handleClose["<<name_<<"]"<<"  closed"<<std::endl;
            state_ = kDisconnected;  //改成关闭状态
            channel_->disableAll();
            loop_->updateChannel(channel_.get());
            if(closeCallback_) {
                closeCallback_(shared_from_this());
            }
        }
        void handleWrite() {
            flushputBuffer();
        }
        void handleRead() {
            ssize_t n=inputBuffer_.readFromFd(sockfd_);
            if(n>0) {
                if(messageCallback_) {
                    messageCallback_(shared_from_this(),&inputBuffer_);
                } 
            }else if(n==0) {
                handleClose();
            }else {
                handleError();
            }
        }
        //即时刷新缓冲区，设置可写事件
        void flushputBuffer() {
            while(outputBuffer_.readableBytes()>0) {
                int saveError = 0;
                int n = outputBuffer_.writeToFd(sockfd_,&saveError);
                if(n>0) continue; 
                else if(n<0) {
                    if(saveError==EWOULDBLOCK||saveError==EAGAIN) {
                        break;
                    }
                    handleError();
                    return;  //处理错误，返回
                }else {
                    //n==0 异常
                    handleError();
                    return;
                }
            }
            if(outputBuffer_.readableBytes()>0) {
                if(!channel_->isWriting()) {
                    channel_->enableWrite();
                    loop_->updateChannel(channel_.get());
                }
            }else {
                if(channel_->isWriting()) {
                    channel_->disableWriting();
                    loop_->updateChannel(channel_.get());
                }
            }
        }
    };

}   