#include "log.h"
using namespace std;
#define get_time(t_sec) (*localtime(&t_sec))
Log::Log() {
    _line_count = 0;
    _is_async = false;
    _write_thread = nullptr;
    _bq = nullptr;
    _today = 0;
    _file = nullptr;
}
Log::~Log() {
    if (_write_thread && _write_thread->joinable()) {
        while (!_bq->empty()) {
            _bq->flush();
        }
        _bq->close();
        auto notify = [&]()->void {
            int i = 0;
            while (i++ < 1000) {
                _bq->flush();
            }
        };
        thread t(notify);//测试时发现一种情况，当_bq->close()中_con_consumer.notify_all()后,_bq才运行到pop中wait(_con_consumer)部分，导致线程无法结束，因此这儿反复通知，避免这种情况
        t.detach();
        _write_thread->join();
    }
    if (_file) {
        lock_guard<mutex> lock(_mtx);
        flush();
        fclose(_file);
    }
}
const int LOG_NAME_LEN = 256;
void Log::init(int level, const char* path, const char* suffix, int maxQueueCapacity) {
    _is_open = true;
    _level = level;
    if (maxQueueCapacity > 0) {
        _is_async = true;
        if (!_bq) {
            _bq = std::make_unique<block_queue<string>>(maxQueueCapacity);
            _write_thread = std::make_unique<thread>(flush_log_thread);
        }
    }
    else {
        _is_async = false;
    }
    _line_count = 0;
    time_t timer = time(nullptr);
    struct tm* sysTime = localtime(&timer);
    struct tm t = *sysTime;
    _path = path;
    _suffix = suffix;
    char fileName[LOG_NAME_LEN] = { 0 };
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
        path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix);
    _today = t.tm_mday;
    {
        lock_guard<mutex> lock(_mtx);
        _buff.clear();
        if (_file) {
            flush();
            fclose(_file);
        }
    }
    _file = fopen(fileName, "a");
    if (_file == nullptr) {
        mkdir(path, 0777);
        _file = fopen(fileName, "a");
    }
}
void Log::write(int level, const char* format, ...) {
    timeval now = { 0,0 };
    gettimeofday(&now, 0);
    tm t = get_time(now.tv_sec);
    va_list va_list;
    if (_today != t.tm_mday || (_line_count>0 && (_line_count % _MAX_LINE == 0))) {
        unique_lock<mutex> lock(_mtx, defer_lock);
        char newFile[LOG_NAME_LEN];
        char tail[36] = { 0 };
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        if (_today != t.tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", _path, tail, _suffix);
            _today = t.tm_mday;
            _line_count = 0;
        }
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", _path, tail, (_line_count / _MAX_LINE), _suffix);
        }
        lock.lock();
        flush();
        fclose(_file);
        _file = fopen(newFile, "a");
    }
    {
        lock_guard<mutex> lock(_mtx);
        ++_line_count;
        int n = snprintf(const_cast<char*>(_buff.write_start_pos()), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        _buff.write_add(n);
        _append_log_level_title(level);
        va_start(va_list, format);
        int m = vsnprintf(const_cast<char*>(_buff.write_start_pos()), _buff.writeable_bytes(), format, va_list);        
        va_end(va_list);
        _buff.write_add(m);
        _buff.add_data("\n\0", 2);
        if (_is_async && _bq && !_bq->full()) {
            _bq->emplace(_buff.read_all());
        }
        else {
            fputs(_buff.read_start_pos(), _file);
            _buff.read_all();
        }
    }
}
void Log::_append_log_level_title(int level) {
    switch (level) {
    case 0:
        _buff.add_data("[debug]: ", 9);
        break;
    case 1:
        _buff.add_data("[info] : ", 9);
        break;
    case 2:
        _buff.add_data("[warn] : ", 9);
        break;
    case 3:
        _buff.add_data("[error]: ", 9);
        break;
    default:
        _buff.add_data("[info] : ", 9);
        break;
    }
}
void Log::flush() {
    if (_is_async) {
        _bq->flush();
    }
    fflush(_file);
}
void Log::_async_write() {
    string str = "";
    int i = 0;
    while (_bq->pop(str)) {
        lock_guard<mutex> lock(_mtx);
        fputs(str.c_str(), _file);
        fflush(_file);
    }
}
Log* Log::instance() {
    static Log log;
    return &log;
}
void Log::flush_log_thread() {
    Log::instance()->_async_write();
}
int Log::get_level() {
    lock_guard<mutex> locker(_mtx);
    return _level;
}
