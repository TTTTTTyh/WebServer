#ifndef THREAD_POOL_H 
#define THREAD_POOL_H 
#include<mutex>
#include<functional>
#include<queue>
#include<thread>
#include <condition_variable>
#include <iostream>
class thread_pool {
public:
    thread_pool(int max_pool_count = std::thread::hardware_concurrency()) :pool_ptr(std::make_shared<pool>()) {
        for (int i = 0;i < max_pool_count;++i) {
            std::thread([pool_ptr = this->pool_ptr]()->void {
                while (true) {
                    std::unique_lock<std::mutex> lock(pool_ptr->mtx);
                    if (pool_ptr->is_close) break;
                    if (pool_ptr->pool.empty()) {
                        pool_ptr->cond.wait(lock);
                    }
                    else {
                        auto task = std::move(pool_ptr->pool.front());
                        pool_ptr->pool.pop();
                        lock.unlock();
                        task();
                        lock.lock();
                    }
                }
            }).detach();
        }
    }
    ~thread_pool() {
        if (static_cast<bool>(pool_ptr)) {
            {
                std::lock_guard<std::mutex> lock(pool_ptr->mtx);
                pool_ptr->is_close = true;
            }
            int i = 0;
            while (i++ < 1000) {
                pool_ptr->cond.notify_all();
            }
        }
    }
    void add_task(std::function<void()>&& func) {
        std::lock_guard<std::mutex> lock(pool_ptr->mtx);
        pool_ptr->pool.emplace(std::move(func));
        pool_ptr->cond.notify_one();
    }
    void add_task(const std::function<void()>& func) {
        std::lock_guard<std::mutex> lock(pool_ptr->mtx);
        pool_ptr->pool.emplace(func);
        pool_ptr->cond.notify_one();
    }
private:
    struct pool {
        std::mutex mtx;
        std::queue<std::function<void()>> pool;
        std::condition_variable cond;
        bool is_close;
    };
    std::shared_ptr<pool> pool_ptr;    
};
#endif