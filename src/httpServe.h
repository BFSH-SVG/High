#pragma once
#include "TcpServer.h"
#include "httpRequest.h"
#include "httpResponse.h"
#include "ThreadPool.h"
#include "eventLoop.h"
#include <fstream>   //文件读取
#include <sstream>   //字符串拼接
#include "Log/Logger.h"
#include "db/SimpleDB.h" //数据库
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
                //获取路径，实现路由
                std::string path=request.getPath();
                if(path == "/") {
                    //默认首页
                    std::string content=readFile("./www/index.html");
                    if(!content.empty()) {
                        response.setStatusCode(k200OK);
                        response.addHeader("Content-Type","text/html");
                        response.setBody(content);
                    }else {
                        response.setStatusCode(k404notFound);
                        response.setBody("404 Not Found");
                    }
                }else if(path == "/about") {
                    response.setStatusCode(k200OK);
                    response.addHeader("Content-Type","text/plain");
                    response.setBody("this is about page");
                }else if(path == "/help") {
                    response.setStatusCode(k200OK);
                    response.addHeader("Content-Type","text/plain");
                    response.setBody("help page");
                } else if(path == "/api/users") {
                    //查询数据库
                    auto results = SimpleDB::query("select * from my_test;");
                    //构造json
                    std::string json = "[";    //构造字符json
                    for(size_t i = 0;i<results.size();i++) {
                        json += "{\"id\":" + results[i][0]+",\"name\":\""+results[i][1]+ "\"}";//拼接json
                        if(i!=results.size()-1) {
                            json += ",";
                        }
                        json += "]";
                    }
                    //
                    //设置响应
                    response.setStatusCode(k200OK);
                    response.addHeader("Content-Type","application/json");
                    response.setBody(json);
                }else {
                    //防止路径遍历攻击
                    if(path.find("..")!=std::string::npos) {
                        response.setStatusCode(k404notFound);
                        response.setBody("not found");
                    }else {
                        //读取内容
                        std::string filename = "./www"+path;
                        std::string content = readFile(filename);
                        if(!content.empty()) {
                            response.setStatusCode(k200OK);
                            response.addHeader("Content-Type",getContentType(path));
                            response.setBody(content);
                        } else {
                            response.setStatusCode(k404notFound);
                            response.setBody("404 Not Found");
                        }
                    }
                }
                //构造成字符串，并返回给用户
                std::string responseString=response.toString();
                LOG_INFO<<request.getMethod()<<" "<<path<<" "<<responseString.size();
               //解析完调用loop方法，将响应发送到pedingqueue中
                loop_->runInloop([conn,responseString](){
                    conn->send(responseString);
                });    
            }  else {
                //解析失败
                HttpResponse response;
                response.setStatusCode(k400badRequest);
                response.addHeader("Content-Type","text/plain");
                response.setBody("bad request");
                std::string responseString=response.toString();
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
    //读取文件内容
    const std::string readFile(const std::string&filename) {
        std::ifstream file(filename,std::ios::binary);
        if(!file) {
            return "";
        }
        //读取内容
        std::ostringstream ss;
        ss<<file.rdbuf();     //读取到读缓冲区
        return ss.str();
    }
    //根据文件获取类型
    std::string getContentType(const std::string&path) {
       size_t pos= path.find_last_of(".");
       if(pos == std::string::npos) {
           return "text/plain";
       }
       //查找后缀
       std::string ext = path.substr(pos); 
       if(ext == ".html") {
            return "text/html";
       }else if(ext == ".css") {
            return "text/css";
       }else if(ext ==".js") {
            return "application/javascript";
       } else if(ext == ".jpg" || ext == ".jpeg") {
            return "image/jpeg";
       } else if(ext == ".json") {
            return "application/json";
       } else if(ext == ".png") {
            return "image/png";
       }  else {
            return "text/plain";
       }
    }


    };
}