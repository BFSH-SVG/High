#pragma once
#include "Acceptor.h"
#include "eventLoop.h"
#include "epoll.h"
#include <map>
#include <atomic>
#include "TcpConnection.h"
namespace muduowebserv {
    class TcpConnection; // 前置声明
// 类型别名（和 TcpConnection.h 里一样）
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&,Buffer*)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
    class TcpServer {
    public:
        TcpServer(EventLoop* loop,int port,const std::string&name):loop_(loop),port_(port),name_(name)
        {
            //创建Acceptor
            acceptor_ = std::make_unique<Acceptor>(loop,port);
            //设置新连接回调
            acceptor_->setnewConnectCallback([this](int sockfd){
               newConnection(sockfd);
            });
        }
        //启动服务器
        void start() {
            acceptor_->startlisten();
        }
        //用户设置回调
        void setConnectionCallback(const ConnectionCallback&cb) {
            connectioncallback_ = cb;
        }
        void setMessageCallback(const MessageCallback&cb) {
            messagecallback_ = cb;
        }
    private:
        //成员变量
        EventLoop* loop_;  //监听listenfd,client_fd
        int port_;  //监听端口
        std::string name_;   //服务器名称
        std::map<std::string,TcpConnectionPtr>connections_;
        std::unique_ptr<Acceptor>acceptor_;  //接收器,接受新连接
        std::atomic<int> nextConnId_{0}; // 下一个连接的id
        //用户回调
        ConnectionCallback connectioncallback_;
        MessageCallback messagecallback_;
        CloseCallback closecallback_;
        //成员函数
        void newConnection(int sockfd){
            //1生成连接名字
            char buf[32];
            snprintf(buf,sizeof(buf),"-%d",nextConnId_++);
            std::string connName = name_ + buf; //生成连接名字
            //2创建tcpconnect
            TcpConnectionPtr conn=std::make_shared<TcpConnection>(loop_,sockfd,connName);
            //3设置回调
            conn->setConnectionCallback(connectioncallback_);
            conn->setMessageCallback(messagecallback_);
            conn->setCloseback([this](const TcpConnectionPtr& conn){
                deleteConnection(conn);
            });
            connections_[connName] = conn;
            conn->connectEstablished(); //建立连接
        }
        void deleteConnection(const TcpConnectionPtr& conn) {
            std::cout<<"deleteConnection"<< conn->name()<<std::endl;
            connections_.erase(conn->name());
        }
    };
}