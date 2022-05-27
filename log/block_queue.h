#ifndef BLOCK_QUEUE_H 
#define BLOCK_QUEUE_H 
#include<queue>
#include<condition_variable>
#include<mutex>
template<class T>
class block_queue
{
private:
    std::queue<T> _queue;
    std::mutex _mtx;
    std::condition_variable _cond_consumer;
    std::condition_variable _cond_producer;
    std::size_t _cap;
public:
    bool _is_close;
    explicit block_queue(int max_size) : _cap(max_size) {}
    ~block_queue() = default;
    void emplace(T&& t) {
        std::unique_lock<std::mutex> lock(_mtx);
        if (_queue.size() == _cap) {
            _cond_producer.wait(lock);
        }
        _queue.emplace(std::forward<T>(t));
        _cond_consumer.notify_one();
    }
    void emplace(const T& t) {
        std::unique_lock<std::mutex> lock(_mtx);
        if (_queue.size() == _cap) {
            _cond_producer.wait(lock);
        }
        _queue.emplace(t);
        _cond_consumer.notify_one();
    }
    bool pop(T& t) {
        std::unique_lock<std::mutex> lock(_mtx);
        while (_queue.empty()) {
            _cond_consumer.wait(lock);
            if (_is_close) {
                return false;
            }
        }
        t = std::move(_queue.front());
        _queue.pop();
        _cond_producer.notify_one();
        return true;
    }
    bool empty() {
        std::lock_guard<std::mutex> lock(_mtx);
        return _queue.empty();
    }
    inline void flush() {
        _cond_consumer.notify_one();
    };
    inline bool full() {
        std::lock_guard<std::mutex> lock(_mtx);
        return _queue.size() == _cap;
    }
    inline std::size_t size() {
        std::lock_guard<std::mutex> lock(_mtx);
        return _queue.size();
    }
    void close() {
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _is_close = true;
            while (!_queue.empty()) {
                _queue.pop();
            }
        }
        _cond_producer.notify_all();
        _cond_consumer.notify_all();
    }
};
#endif