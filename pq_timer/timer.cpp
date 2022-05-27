#include "timer.h"
using std::move;
timer::timer() :_size(0) {
    _heap.reserve(64);
}
timer::~timer() {
    clear();
}
void timer::add_task(int id, int time_out_ms, const call_back& cb_func) {
    if (_ref.count(id)) {
        int i = _ref[id];
        _heap[i].time = _clock::now() + ms(time_out_ms);
        _heap[i].call_back_func = cb_func;
        if (!_sift_down(i)) {
            _sift_up(i);
        }
    }
    else {
        if (_size >= _heap.size())
            _heap.emplace_back(id, cb_func, _clock::now() + ms(time_out_ms));
        else
            _heap[_size] = { id,cb_func,_clock::now() + ms(time_out_ms) };
        _ref[id] = _size;
        _sift_up(_size++);
    }
}
bool timer::_cmp(const timer_node& a, const timer_node& b) {
    return a.time < b.time;
}
#define prt(x) ((x-1)/2)
#define lc(x) ((2*x+1))
#define rc(x) ((2*x+2))
bool timer::_sift_up(int n) {
    if (n > 0) {
        timer_node temp(std::move(_heap[n]));bool return_val = false;
        for (int p = prt(n);p >= 0 && _cmp(_heap[p], _heap[n]);n = p, p = prt(p)) {
            _heap[n] = std::move(_heap[p]);
            _ref[_heap[n].id] = n;
            return_val = true;
        }
        if (return_val) {
            _heap[n] = std::move(temp);
            _ref[temp.id] = n;
            return true;
        }
    }
    return false;
}
bool timer::_sift_down(int n) {
    if (n > 0) {
        timer_node temp(std::move(_heap[n]));
        bool return_val = false;
        for (int c = lc(n);c < _size;n = c, c = lc(c)) {
            int rc = rc(n);
            if (rc < _size && _cmp(_heap[rc], _heap[c])) c = rc;
            if (_cmp(_heap[c], _heap[n])) {
                _heap[n] = std::move(_heap[c]);
                _ref[_heap[n].id] = n;
                return_val = true;
            }
            else break;
        }
        if (return_val) {
            _heap[n] = std::move(temp);
            _ref[temp.id] = n;
            return true;
        }
    }
    return false;
}
// void timer::_del(int id) {
//     int itx = _ref[id];
//     _heap[itx] = std::move(_heap[--_size]);
//     _ref[_heap[itx].id] = itx;
//     _sift_down(itx);
// }
void timer::adjust(int id, int new_time_out_ms) {
    int itx = _ref[id];
    _heap[itx].time = _clock::now() + ms(new_time_out_ms);
    if (!_sift_down(itx)) {
        _sift_up(itx);
    }
}
void timer::_pop() {
    if (_size) {
        _heap[0] = std::move(_heap[--_size]);
        _ref[_heap[0].id] = 0;
        _sift_down(0);
    }
}
void timer::clear() {
    _heap.resize(0);
    _ref.reserve(0);
}
void timer::clear_out_time() {
    while (_size) {
        if (_heap[0].time < _clock::now()) _pop();
        else break;
    }
}
int timer::get_next_time() {
    clear_out_time();
    if (!_heap.empty()) {
        return std::chrono::duration_cast<ms>(_heap[0].time - _clock::now()).count();
    }
    return -1;
}
void timer::pop() {
    if (!_heap.empty()) {
        _heap[0] = std::move(_heap[--_size]);
        _sift_down(0);
    }
}