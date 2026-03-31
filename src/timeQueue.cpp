#include "timeQueue.h"
#include "timer.h"
#include "eventLoop.h"
#include<sys/timerfd.h>
#include <error.h>
#include <iostream>
#include <memory>
#include <cstring>
//创建timerfd
namespace muduowebserv {
//构造函数


static int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerfd < 0) {
        perror("create timerfd error");
    }
    return timerfd;
}
TimeQueue::TimeQueue(EventLoop* loop): 
loop_(loop),timerFd_(createTimerfd()),timerChannel_(std::make_unique<Channel>(timerFd_)) {
    //设置读回调
    timerChannel_->setReadCallback([this](){
        handleRead();
    }); 
    //开始监听
    timerChannel_->enableReading();
    //3.注册到epoll
    loop_->updateChannel(timerChannel_.get());
    }
//添加定时器
int64_t TimeQueue::addTimer(Timer::TimeCallback callback,TimeStamp when,double interval) {
    Timer* timer = new Timer(callback,when,interval);
    timerlist_.insert(Entry(when,timer));
    //早到期的，跟新timefd
    if(timerlist_.begin()->second == timer) {
        resetTimefd(timerFd_,when);
    }
    //返回ID
    return timer->id();
}

//找到期定时器
std::vector<TimeQueue::Entry>TimeQueue::getExpired(TimeStamp now) {
    //找到分界点
    Entry sentinel(now,nullptr);
    auto end = timerlist_.lower_bound(sentinel);
    //把所有到期的存入vector
    std::vector<Entry>expired(timerlist_.begin(),end);
    //从set中删除
    timerlist_.erase(timerlist_.begin(),end);
    return expired;
}
//重置周期性定时器，在timer.h里面已经定义过了
void TimeQueue::reset(const std::vector<Entry>&expired,TimeStamp now) {
    //遍历到期的定时器，取出定时器
    for(const Entry& entry:expired) {
        Timer* timer = entry.second;  //临时指针，指向定时器对象
        if(timer->repeat()) {
            timer->reset(now);
            timerlist_.insert(Entry(timer->expire(),timer));
        }else {
            delete timer;  //释放定时器对象，一次性定时器
        }
    }
}
//取消定时器
void TimeQueue::cancelTimer(int64_t timeId) {
    //找到定时器
    for(auto it = timerlist_.begin();it!=timerlist_.end();it++) {
        Timer* timer = it->second;
        if(timer->id() == timeId){
            timerlist_.erase(it);  //从set里面删除
            delete timer; 
            std::cout<<"cancel timer success!"<<std::endl;
            return;
        }
    }
}
//读取timefd，清除触发状态
static void readTimefd(int timefd) {
    uint64_t howmany;
    ssize_t n = ::read(timefd,&howmany,sizeof(howmany));
    if(n != sizeof(howmany)) {
        std::cout<<"read timefd error"<<std::endl;
    }

}
//计算剩余时间
static struct timespec howmanyTimeFromNow(TimeStamp when) {
    int64_t microseconds= when.getMicroSecondsSinceEpoch()-
        TimeStamp::now().getMicroSecondsSinceEpoch();
    if(microseconds <1000) {
        microseconds = 1000;   //不足1ms设置成1ms，给足缓冲时间
    }
    //用结构体tv来接收s和ns
    struct timespec tv;
    tv.tv_sec = static_cast<time_t>(microseconds / TimeStamp::kMicroSecondsPerSecond);
    tv.tv_nsec = static_cast<long> ((microseconds %TimeStamp::kMicroSecondsPerSecond)/1000);
    return tv;
}
//设置闹钟时间，多久返回
static void resetTimefd(int timefd,TimeStamp expiration) {
    struct itimerspec newValue;
    struct itimerspec oldValue;
    //清零
    memset(&newValue,0,sizeof(newValue));
    memset(&oldValue,0,sizeof(oldValue));
    newValue.it_value = howmanyTimeFromNow(expiration);
    int ret = ::timerfd_settime(timefd,0,&newValue,&oldValue);
    if(ret < 0) std::cout<<"timefd_settime error"<<std::endl; 
}
//handle处理
void TimeQueue::handleRead() {
    //获取当前时间
    TimeStamp now = TimeStamp::now();
    //清除timefd状态
    readTimefd(timerFd_);
    //获取到期定时器
    std::vector<Entry> expire= getExpired(now);
    //执行所有定时器的回调
    for(const Entry& entry :expire) {
        entry.second->run();
    }
    //处理周期性定时器
    reset(expire,now); 
    //设置下一次闹钟
    if(!timerlist_.empty()) {
        resetTimefd(timerFd_,timerlist_.begin()->first);
    }
}
//析构函数
TimeQueue:: ~TimeQueue() {
    for(auto &entry : timerlist_) {
        delete entry.second;   //删除所有Timer
    }
}

}