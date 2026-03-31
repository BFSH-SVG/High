#pragma once
#include "TcpServer.h"
#include "httpRequest.h"
#include "httpResponse.h"
#include "ThreadPool.h"
#include "eventLoop.h"
namespace muduowebserv {

    class HttpServer {
    public:
        HttpServer(EventLoop* loop,int port,const std::string&name):server_(loop,port,name),threadpool_(8),loop_(loop)
        {
             //设置消息回调
            server_.setMessageCallback([this](const TcpConnectionPtr&conn,Buffer*buf) {
                onMessage(conn,buf);
            });
        }
        void start() {
            server_.start();
        }
        //开始放入缓冲区进行解析
        void onMessage(const TcpConnectionPtr&conn,Buffer*buf){
            //交给线程池来处理解析
            //先复制buff内容，然后清空
            std::string data=std::string(buf->peek(),buf->readableBytes());
            buf->retrieve(buf->readableBytes());

            threadpool_.enqueue([this,conn,data](){
                HttpRequest request;
                Buffer tempbuf; //创建临时缓冲区
                tempbuf.append(data.c_str(),data.size());
            bool success= request.parse(&tempbuf);
            std::cout << "Parse success: " << success << std::endl;
            if(success) {

                //模拟真实数据库查询
                //std::this_thread::sleep_for(std::chrono::milliseconds(20));


                //开始响应
                HttpResponse response;
                response.setStatusCode("200");
                response.statusMessage("OK");
                response.addHeader("Content-Type","text/plain");
                response.setBody("hello linux!");
                //构造成字符串
                std::string responseString=response.toString();
                std::cout << "Response: " << responseString << std::endl;
               //解析完调用loop方法，将响应发送到pedingqueue中
                loop_->runInloop([conn,responseString](){
                    conn->send(responseString);
                });    
            }  
        });       
    }
    private:
    TcpServer server_;    
    ThreadPool threadpool_;   //线程池
    EventLoop* loop_;  //事件循环对象
    };
}