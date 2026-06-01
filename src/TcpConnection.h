#pragma once
#include "eventLoop.h"
#include <string>
#include <memory>
#include <unistd.h>  //close
#include "channel.h"  //chanel类需�?
#include <iostream>
#include <sys/socket.h> //socket.get
#include <functional>
#include <netinet/in.h> //sockaddr_in
#include <err.h> //perror
#include "buffer.h"
#include <sys/sendfile.h>
#include "Log/Logger.h"
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
            //设置不同的回�?
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
        //发送信�?
        void send(const std::string& message) {
            if(state_== kDisconnected) {
                return;
            }
            if(loop_->isInLoopThread()) {
                outputBuffer_.append(message.c_str(),message.size());
                flushputBuffer();
            }else {
                    loop_->runInloop([self=shared_from_this(),message]() {
                    //判断�?否是连接状态再继续,io线程�?能中途关�?连接
                    if(self->state_ == kConnected) {
                        self->outputBuffer_.append(message.c_str(),message.size());
                        self->flushputBuffer();
                    }
                    });
            }
        }
        void trySendfile() {
            if(sendingFd_ ==-1) return;
            while(sendingOffset_ <sendingTotal_) {
                ssize_t n=::sendfile(
                    sockfd_,sendingFd_,
                    &sendingOffset_,
                    sendingTotal_-sendingOffset_);
                if(n>0) {
                    continue;
                } 
                if(n<0&&errno==EAGAIN) {
                    //缓冲区满�?
                    if(!channel_->isWriting()) {
                        channel_->enableWrite();
                        loop_->updateChannel(channel_.get());
                    }
                    return;    //立即返回，不阻�??
                }
                //其他错�??
                LOG_ERROR<<"sendfile error: "<<strerror(errno);
                break;

            }
            //发完或者出错，清理资源
            cleanupSending();
        }
        void cleanupSending() {
            if(sendingFd_!=-1) {
                ::close(sendingFd_);
                sendingFd_ = -1;
            }
            sendingOffset_ = 0;
            sendingTotal_ = 0;
            //如果注册了wirte事件，取消掉
            if(channel_->isWriting()) {
                channel_->disableWriting();
                loop_->updateChannel(channel_.get());
            }
        }
        //sendfile发�?
        void sendFile(int fileFd,size_t fileSize) {
            //1检�?state_�?否存�?
            if(state_ != kConnected) {
                ::close(fileFd);   //连接�?开，关�?文件
                return;
            }
            //因为文件夹无法一次发送完成所以用1ms等待
            // ssize_t offset = 0;
            // while(offset <fileSize) {
            //     size_t n=::sendfile(sockfd_,fileFd,&offset,fileSize-offset);
            //     if(n>0) {
            //         //发送成功一部分，继�?�?�?
            //         continue;
            //     }else if(n==0) {
            //         break;
            //     }else if(n<=0 && errno==EAGAIN) {
            //         //缓冲区满了，休息1ms
            //         usleep(1000);
            //         continue;
            //     }
            //     //其他错�??
            //     break;
            // }
            //用事件调用优�?
            //保存状态，后续调用
            sendingFd_ = fileFd;
            sendingOffset_ = 0;
            sendingTotal_ = fileSize;
            trySendfile();
            // ::sendfile(sockfd_,fileFd,nullptr,fileSize);  //开始发�?
            // ::close(fileFd);  //发完关闭
        }
        //优雅关闭
        void shutdown() {
            if(state_ == kConnected) {
                //�ر�ǰ��ȡ����ʱ��
                cancelKeepAliveTimer();
                state_ = kDisconnected;  //标�?�为已断开
                ::shutdown(sockfd_,SHUT_WR);   //关闭写�??
            }
        }
        //取消定时�?
        void cancelKeepAliveTimer() {
            if(keepAliveTimerId_ != 0 ) {
                loop_->cancelTimer(keepAliveTimerId_);
                keepAliveTimerId_ = 0;  //停�?�运�?
            }
        }
        //设置超时定时�?
        void setKeepAliveTimeOut(double timeoutSeconds) {
            cancelKeepAliveTimer();
            //����Ͽ��Ͳ��ô����¶�ʱ����
            if(state_ == kDisconnected) {
                std::cout << "setKeepAliveTimeOut: already disconnected, skip timer" << std::endl;
                return;
            }
            keepAliveTimerId_ = loop_->runafter([self=shared_from_this()]{
                std::cout << "keep-alive timeout triggered for " << self->name() << std::endl;
                if(self->state_ == kConnected) {
                    self->shutdown();
                }
            },timeoutSeconds);
            std::cout << "keep-alive timer registered for " << name_ << ", timeout=" << timeoutSeconds << "s" << std::endl;
        }
        ~TcpConnection() {
            //关闭fd
            ::close(sockfd_);
        }

    private:
        EventLoop* loop_;  //注册fd
        int sockfd_; 
        std::string name_;
        std::unique_ptr<Channel> channel_;  //channel_负责fd的�?�写事件
        Buffer inputBuffer_;  //输入缓冲�?
        Buffer outputBuffer_; //输出缓冲�?
        ConnectionCallback connectionCallback_;  //连接建立，或者断开连接
        MessageCallback messageCallback_; //消息回调
        CloseCallback closeCallback_;  //连接关闭回调
        int sendingFd_ = -1;   //正在发送的文件fd,-1表示没有
        off_t sendingOffset_ = 0;
        off_t sendingTotal_ = 0;    //文件总大�?
        std::atomic<ConnectionState> state_=kConnected;  //连接状�?
        int64_t keepAliveTimerId_ = 0;   //�?0表示有定时器在跑
        //处理四�?�回�?
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
            cancelKeepAliveTimer();
            if(state_==kDisconnected) {
                channel_->disableAll();
                loop_->updateChannel(channel_.get());
                if(closeCallback_) {
                    closeCallback_(shared_from_this());
                }
                return;
            }
            auto self = shared_from_this();
            if(sendingFd_ !=-1) {
                ::close(sendingFd_);
                sendingFd_ = -1;
            }
            sendingOffset_ = 0;
            sendingTotal_ = 0;
            std::cout<<"handleClose["<<name_<<"]"<<"  closed"<<std::endl;
            state_ = kDisconnected;
            channel_->disableAll();
            loop_->updateChannel(channel_.get());
            if(closeCallback_) {
                closeCallback_(shared_from_this());
            }
        }
        void handleWrite() {
            //优先处理sendfile，�?�果正在发文�?
            if(sendingFd_ !=-1) {
                trySendfile();   //继续发�?
                return;
            }
            flushputBuffer();
        }
        void handleRead() {
            ssize_t n=inputBuffer_.readFromFd(sockfd_);
            if(n>0) {
                //有新数据就取消定时器，重新�?�时
                cancelKeepAliveTimer();
                if(messageCallback_) {
                    messageCallback_(shared_from_this(),&inputBuffer_);
                } 
            }else if(n==0) {
                handleClose();
            }else {
                handleError();
            }
        }
        //即时刷新缓冲区，设置�?写事�?
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
                    return;  //处理错�??，返�?
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