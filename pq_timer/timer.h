#ifndef TIMER_H 
#define TIMER_H 
#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>
typedef std::chrono::high_resolution_clock _clock;
typedef _clock::time_point time_stamp;
typedef std::chrono::milliseconds ms;
typedef std::function<void()> call_back;
class timer
{
private:
    class timer_node {
    public:        
        int id;
        call_back call_back_func;
        time_stamp time;
        timer_node() = default;
        timer_node(int id, const call_back& c, const time_stamp& t) :id(id), call_back_func(c), time(t) {}
        timer_node(timer_node&& t) :id(t.id), call_back_func(std::move(t.call_back_func)), time(std::move(t.time)) {
            t.id = -1;
        }
        timer_node(const timer_node& t) :id(t.id), call_back_func(t.call_back_func), time(t.time) {}
        timer_node& operator =(timer_node&& t) {
            id = t.id;call_back_func = std::move(t.call_back_func);
            time = std::move(t.time);
            t.id = -1;
        }
    };
    std::vector<timer_node> _heap;
    bool _sift_up(int n);
    bool _sift_down(int n);
    std::unordered_map<int, int> _ref;
    bool _cmp(const timer_node& a, const timer_node& b);
    int _size;
    // void _del(int id);
    void _pop();
public:
    timer();
    void add_task(int id, int time_out_ms,const call_back& cb_func);
    int get_next_time();
    void adjust(int id, int new_time_out_ms);
    void clear();
    void clear_out_time();
    void pop();
    ~timer();
};
#endif