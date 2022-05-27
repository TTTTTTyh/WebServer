#ifndef LOG_H
#define LOG_H
#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>         
#include "block_queue.h"
#include "../buffer/buffer.h"
using  std::unique_ptr;
using std::mutex;
class Log {
public:
    void init(int level, const char* path = "./record_log", const char* suffix = ".log", int maxQueueCapacity = 1024);
    static Log* instance();
    static void flush_log_thread();
    void write(int level, const char* format, ...);
    void flush();
    inline bool is_open() { return _is_open; }
    int get_level();
private:
    Log();
    ~Log();
    void _append_log_level_title(int level);
    void _async_write();
    bool _is_async;
    FILE* _file;
    unique_ptr<block_queue<std::string>> _bq;
    unique_ptr<std::thread> _write_thread;
    mutex _mtx;
    static const int _MAX_LINE = 5000;
    bool _is_open;
    int _line_count;
    int _today;
    int _level;
    buffer _buff;
    const char* _path;
    const char* _suffix;
};
#define LOG_BASE(level,format,...) {\
            Log* log_ = Log::instance();\
            if (log_->is_open() && log_->get_level() <= level) {\
            log_->write(level, format, ##__VA_ARGS__);\
            log_->flush();\
    }}
#define LOG_DEBUG(format,...) LOG_BASE(0, format, ##__VA_ARGS__)
#define LOG_INFO(format,...) LOG_BASE(1, format, ##__VA_ARGS__)
#define LOG_WARN(format,...) LOG_BASE(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format,...) LOG_BASE(3, format, ##__VA_ARGS__)
#endif
