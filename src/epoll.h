#pragma once 
#include <cstdio>
#include <sys/epoll.h>
#include <iostream>
#include <vector>
#include <unistd.h>  //提供linux系统调用
#include "channel.h"
namespace muduowebserv {
    class Epoll {
    public:
        Epoll() {
            //1创建epoll实例
            epfd_ = epoll_create(1);
            if(epfd_ <0) {
                perror("epoll_create_failed!");
            }
            //初始化数组
            events_.resize(kMaxEvents);
        }
        void updateChannel(Channel* channel) {
            //监听事件
            struct epoll_event ev;
            ev.events = channel->events();
            //保存channel指针,不负责释放，只是暂时借用
            ev.data.ptr = channel;
            int op = EPOLL_CTL_MOD;  //epoll_ctl操作类型，默认修改状态
            if(channel->getIndex() == kNew) {
                op = EPOLL_CTL_ADD;
                channel->setIndex(kAdded);  //设置标志位
            }else if(channel->getIndex() == kDeleted) {
                op = EPOLL_CTL_ADD;
                channel->setIndex(kAdded);
            } else{
                //已经在epoll中,为1
                //判断是否为空事件
                if(channel->isNoneEvent()) {
                    op = EPOLL_CTL_DEL;
                    channel->setIndex(kDeleted);
                }
            }
            //将关心的fd和事件添加事件数组里面
            //先执行系统调用，然后再判断是否成功
            if(epoll_ctl(epfd_,op,channel->fd(),&ev)<0) {
                perror("epoll_ctl_failed!");
            }
        }
        //epoll_wait 阻塞等待，将有事件的fd返回
        std::vector<Channel*> poll() {  //接收所有有事件的channel
            int n = epoll_wait(epfd_,events_.data(),kMaxEvents,-1);
            std::vector<Channel*> activeChannels; //活跃数组，返回的数组
            for(int i=0;i<n;i++) {
                Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
                channel->setRevents(events_[i].events); 
                activeChannels.emplace_back(channel);
            }
            return activeChannels;
        }
        ~Epoll() {
            close(epfd_);
        }
    private:
        int epfd_;  //epoll实例
        std::vector<struct epoll_event> events_;   //epoll事件组
        static const int kMaxEvents = 100;  //数组最大容量
        //三种channel状态
        static const int kNew = 0;    //新添加
        static const int kAdded = 1;  //已添加，修改状态
        static const int kDeleted = 2;  //删除
    };
}
