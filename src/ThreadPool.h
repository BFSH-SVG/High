#pragma once
#include <queue>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <condition_variable>
namespace muduowebserv {
    class ThreadPool {
    public:
        explicit ThreadPool(size_t thread_num): stop_(false) {
            threads_.reserve(thread_num);   //预留空间
            for(size_t i=0;i<thread_num;i++) {
                threads_.emplace_back([this]{
                    while(true) {
                        std::unique_lock<std::mutex> lock(mutex_);  //先加锁
                        condition_.wait(lock,[this]{   //等待条件变量唤醒，睡眠中
                            return !task_queue_.empty() || stop_;
                        });
                        //检查是不是退出
                        if(stop_ && task_queue_.empty()) return;  
                        //取任务
                        auto task=std::move(task_queue_.front());
                        task_queue_.pop();
                        lock.unlock();    //先进行解锁
                        task();
                    }
                });
            }
        } 
        //添加任务
        template<typename F>
        void enqueue(F &&f) {
            std::lock_guard<std::mutex> lock(mutex_);
            task_queue_.push(std::forward<F>(f));
            condition_.notify_one();  //唤醒一个线程
        }
        //析构函数,释放线程池
        ~ThreadPool() {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                stop_ = true;
            }
            //唤醒所有线程
            condition_.notify_all();  
            //等待线程执行完毕
            for(auto &thread: threads_) {
                if(thread.joinable()) thread.join(); //等待线程执行完毕
            }
        }
        private:
        std::queue<std::function<void()>> task_queue_; //任务队列
        std::vector<std::thread> threads_;           //工作线程
        std::mutex mutex_;  
        std::condition_variable condition_;  //条件变量
        bool stop_ = false;    //是否停止
    };
}
