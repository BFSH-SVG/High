#include "Log/LogStream.h"
#include "Log/LogLevel.h"
#include "eventLoop.h"
#include "channel.h"
#include <iostream>
#include "epoll.h"
#include <unistd.h> 
#include "TcpServer.h"
#include "httpServe.h"
#include "ThreadPool.h"
#include <chrono>   //用于时间测量，时间库
#include <atomic>  //原子操作库
#include "db/SimpleDB.h"
using namespace muduowebserv;
// int main() {
//     // 测试 LogStream
//     muduowebserv::LogStream log;
//     log << "Hello " << "World! " << 123;
//     std::cout << "Log content: " << log.data() << std::endl;
//     std::cout << "Length: " << log.getLength() << std::endl;

//     // 测试 LogLevel
//     std::cout << "Level: " << LogLevelToString(LogLevel::INFO) << std::endl;
//     std::cout << "Level: " << LogLevelToString(LogLevel::ERROR) << std::endl;

//     return 0;
// }
int main() {
    // muduowebserv::EventLoop loop;
    // muduowebserv::Channel channel(0);
    // //设置回调
    // channel.setReadCallback([&channel]() {
    //     char buf[20];
    //     int n = read(0,buf,sizeof(buf)-1);
    //     //将buf的换行符去掉
    //     if(n>0) {
    //         buf[n] = '\0';
    //         if(n>0&&(buf[n-1])=='\n') {
    //             buf[n-1] = '\0';
    //         }
    //     }
    //     std::cout<<"read is already!"<<std::endl;
    //     std::cout<<"received:"<<buf<<std::endl;

    // });
    // channel.enableReading();
    // loop.updateChannel(&channel);
    // std::cout<<"read is listening,please input:"<<std::endl;
    // loop.loop();
    // muduowebserv::EventLoop loop;
    // muduowebserv::TcpServer server(&loop,8080,"textserver");
    // //设置连接回调
    // server.setConnectionCallback([](const TcpConnectionPtr& conn){
    //     std::cout<<"new connection link:"<<conn->name()<<std::endl;
    // });
    // server.setMessageCallback([](const TcpConnectionPtr& conn,Buffer* buf){
    //     std::cout<<"receive message!"<<buf->retrieveAllAsString(buf->readableBytes())<<std::endl;
    // });
    // //启动服务器
    // server.start();

    //运行事件循环s
    //loop.loop();
    //打开数据库
    SimpleDB::open("myserver.db");
    SimpleDB::execute("CREATE TABLE IF NOT EXISTS my_test(id INTEGER,name TEXT);");  //数据库自动建表
    EventLoop loop;
    HttpServer server(&loop,8080,"textserver");
    server.start();
    std::cout<<"Http Server is running on port 8080"<<std::endl;
    loop.loop();
    //只关闭一次 SimpleDB::close();
    //线程池测试
    muduowebserv::ThreadPool p(8); 
    // p.enqueue([]{
    //     std::cout<<"task 1 is running"<<std::endl;
    // });
    // p.enqueue([]{
    //     std::cout<<"task 2 is running"<<std::endl;
    // });
    // std::cout<<"tasks are submit"<<std::endl;
    // const int TASK_COUNT = 10000;
    // std::cout<<"task start"<<std::endl;

    // auto start_time = std::chrono::high_resolution_clock::now();
    // std::atomic<int> count(0);
    // for(int i=0;i<TASK_COUNT;i++) {
    //     p.enqueue([&]{
    //         std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //         count++;
    //     });
    // }
    // //等待所有任务执行完毕
    // while(count<TASK_COUNT) {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // }
    // auto end_time = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time);
    // std::cout<<"time cost:"<<duration.count()<<"ms"<<std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // std::cout<<"task end"<<std::endl;
    return 0;
}
