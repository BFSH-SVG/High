#pragma once 
#include "epoll.h"
#include <memory>
#include <functional>
#include <vector>
#include <mutex> 
#include "channel.h"
#include <sys/eventfd.h>
#include <unistd.h>  //close
#include "timeQueue.h"
#include <thread>
namespace muduowebserv{
    class EventLoop{
    public:
        EventLoop()
        : looping_(false),
        quit_(false),
        timeQueue_(std::make_unique<TimeQueue>(this)),
        threadId_(std::this_thread::get_id())
        {
            wakeupFd_=eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
            wakeupChannel_=std::make_unique<Channel>(wakeupFd_);
            wakeupChannel_->setReadCallback([this](){
                handleRead();
            });
            wakeupChannel_->enableReading();
            epoll_.updateChannel(wakeupChannel_.get());
        }
        //启动循环事件
        void loop() {
            looping_ = true;
            quit_ = false;
            //阻塞等待事件
            while(!quit_) {
                //设置数组来接收事件,处理IO事件
                std::vector<Channel*> channels;
                channels = epoll_.poll();  //询问
                for(Channel* channel:channels) {
                    channel->handleEvent();  //处理业务逻辑
                }
                //执行任务队列
                //先写一个临时容器，来交换pedingqueue，这样可以让peding继续接受新完成的任务
                std::vector<std::function<void()>> temp;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    temp.swap(pendingFunctors_);
                }
                for(auto &functor:temp) {
                    functor();
                }
                
            }
            looping_ = false;
        }
        //设置停止
        void quit() {
            quit_ = true;
        }
        bool isLooping() {
            return looping_;
        }  
        //是否是io线程
        bool isInLoopThread() const {
            return threadId_ == std::this_thread::get_id();
        }
        //更新channel
        void updateChannel(Channel* channel) {
            epoll_.updateChannel(channel);
        }
        //返回待处理函数队列
        void runInloop(const std::function<void()>&functor) {
            if(isInLoopThread()) {
                functor();
            }else {
                {
                std::lock_guard<std::mutex> lock(mutex_);
                pendingFunctors_.push_back(functor);
                }
            //关键，开始唤醒
            wakeup();   //pending添加任务，唤醒epoll
            }
        }
        //在指定时间执行定时器
        int64_t runaction(Timer::TimeCallback timecallback,TimeStamp when) {
            return timeQueue_->addTimer(timecallback,when,0);
        }
        //延时多久执行
        int64_t runafter(Timer::TimeCallback timecallback,double delay) {
            TimeStamp when=TimeStamp::now() + delay;
            return timeQueue_->addTimer(timecallback,when,0);
        }
        //每次间隔多久执行
        int64_t runEvery(Timer::TimeCallback timecallback,double interval) {
            TimeStamp when=TimeStamp::now() + interval;
            return timeQueue_->addTimer(timecallback,when,interval);
        }
        //取消定时器
        void cancelTimer(int64_t timerId) {
           timeQueue_->cancelTimer(timerId);
        }
    private:
        Epoll epoll_;
        bool looping_;  //判断是否在循环
        bool quit_; //是否退出
        std::mutex mutex_;
        std::vector<std::function<void()>> pendingFunctors_;    //待处理函数队列
        int wakeupFd_;  //唤醒fd，用于唤醒
        std::unique_ptr<Channel> wakeupChannel_;  //封装eventfd
        std::unique_ptr<TimeQueue> timeQueue_; //定时器队列
        std::thread::id threadId_;   //线程id，记录创建线程的线程id
        //唤醒事件
        void wakeup() {
            uint64_t one=1;
            ssize_t n=::write(wakeupFd_,&one,sizeof(one));
            if(n!=sizeof(one)) {
                perror("write wakeupFd_ error");
            } 
        }
        void handleRead() {
            uint64_t one=0;
            size_t n=::read(wakeupFd_,&one,sizeof(one));
            if(n!=sizeof(one)) {
                perror("read wakeupFd_ error");
            }
        }
    };
}