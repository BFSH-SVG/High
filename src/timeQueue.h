#pragma once
#include "timeStamp.h"
#include "timer.h"
#include <set>
#include <vector>
#include <memory>
namespace muduowebserv {
    class EventLoop;
    class Channel;
    class TimeQueue {
    public:
        TimeQueue(EventLoop* loop);
        //添加定时器，返回定时器ID
        int64_t addTimer(Timer::TimeCallback callback,TimeStamp when,double interval);
        //删除定时器
        void cancelTimer(int64_t timeId); 

        //析构，删除所有定时器
        ~TimeQueue();
    private:
        using Entry = std::pair<TimeStamp,Timer*>;  //一对，时间戳和定时器指针
        using TimerList = std::set<Entry>;  //定时器，按照时间排序
        std::unique_ptr<Channel> timerChannel_;  //定时器通道
        EventLoop* loop_;
        TimerList timerlist_;   //定时器列表
        int timerFd_;    //定时器文件描述符，与epoll集成

        //私有方法，获取所有已到期的定时器
        std::vector<Entry> getExpired(TimeStamp now);
        //重置周期性定时器
        void reset(const std::vector<Entry>& expired,TimeStamp now);    
        //定时器到期处理函数
        void handleRead();
        //跟新timefd的超时时间
        void resetTimerfd(TimeStamp expire);

    };
}