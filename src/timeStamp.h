#pragma once 
#include <cstdint>
#include <sys/time.h>
namespace muduowebserv {
    class TimeStamp{
    public:
    //默认构造，表示无效时间戳
        TimeStamp():microSecondsSinceEpoch_(0) {

        }
        //有参数构造，表示当前时间
        explicit TimeStamp(uint64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch) {

        }
        //获取当前时间
        static TimeStamp now() {
            struct timeval tv;
            gettimeofday(&tv,nullptr);
            return TimeStamp(tv.tv_sec * 1000*1000 + tv.tv_usec);
        }
        //得到时微妙数
        uint64_t getMicroSecondsSinceEpoch() const {
            return microSecondsSinceEpoch_;  //返回的只是副本
        }
        //获取秒数
        time_t getSecondsSinceEpoch() const {
            return static_cast<time_t>(microSecondsSinceEpoch_/1000000);
        }
        //判断时间戳是否有效
        bool isValid() const {
            return microSecondsSinceEpoch_ > 0;
        }
        //常量微秒数，用来转化单位
        static const int kMicroSecondsPerSecond = 1000*1000;
        //返回一个无效时间戳，表示没有时间或者未设置
        static TimeStamp invalid() {
            return TimeStamp();
        }
    private:
        uint64_t microSecondsSinceEpoch_;  //存储1970到当前时间戳

    };
    inline bool operator<(TimeStamp lhs,TimeStamp rhs) {
        return lhs.getMicroSecondsSinceEpoch() < rhs.getMicroSecondsSinceEpoch();
    }
    inline bool operator==(TimeStamp lhs,TimeStamp rhs) {
        return lhs.getMicroSecondsSinceEpoch() == rhs.getMicroSecondsSinceEpoch();
    }
    //表示几微妙之后的时间
    inline TimeStamp operator+(TimeStamp lhs,double seconds) {
        int microSeconds = static_cast<int>(seconds*TimeStamp::kMicroSecondsPerSecond);
        return TimeStamp(lhs.getMicroSecondsSinceEpoch() + microSeconds);
    }
    //时间差
    inline double operator-(TimeStamp high,TimeStamp low) {
        int64_t diff = high.getMicroSecondsSinceEpoch() - low.getMicroSecondsSinceEpoch();
        return static_cast<double>(diff) / TimeStamp::kMicroSecondsPerSecond;
    }
}