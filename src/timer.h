#pragma once
#include "timeStamp.h"
#include <functional>
#include <atomic>
namespace muduowebserv {

    class Timer {
    public:
        using TimeCallback = std::function<void()>;
        Timer(TimeCallback callback,TimeStamp when,double interval):
         expireTime_(when),interval_(interval),repeat_(interval>0),callback_(std::move(callback)),timeId_(numCreate_++) {
         }
         //执行回调
         void run() const {
            callback_();
         }
         int64_t id() const {
            return timeId_;
         }
         //获取到期时间
         TimeStamp expire() const {
            return expireTime_;
         }
         //是否重复
         bool repeat() const {
            return repeat_;
         }
         void reset(TimeStamp now) {
            if(repeat_) {
                expireTime_ = now + interval_; //设置新的到期时间
            }
         }

    private:
        TimeStamp expireTime_;  //到期时间
        double interval_;  //重复间隔
        bool repeat_;   //是否重复
        TimeCallback callback_;   //回调函数
        int64_t timeId_;    //唯一id
        static std::atomic<int64_t> numCreate_;    //计数器，用来生成唯一id,只是声明
    };
    inline std::atomic<int64_t> Timer::numCreate_(0);  //定义，分配内存












}