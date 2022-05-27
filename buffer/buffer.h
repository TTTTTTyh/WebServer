#ifndef BUFFER_H 
#define BUFFER_H 
#include<vector>
#include<atomic>
#include<string>
#include <unistd.h>
#include <sys/uio.h> 
#include <iostream>
using std::string;
class buffer {
private:
    std::vector<char> _buff;
    // std::atomic<std::size_t> _read_pos;//0开始
    // std::atomic<std::size_t> _write_pos;
    int _read_pos;
    int _write_pos;
    void make_space(std::size_t len);//拓展vector空间
    inline void reorder_buffer() {
        auto first = &*_buff.begin();
        std::copy(first + _read_pos, first + _write_pos, first);
        auto readable_len = _write_pos - _read_pos;
        _read_pos = 0;
        _write_pos = readable_len;

    }
public:
    void read_check() {
        string str(&_buff[_read_pos], &_buff[_write_pos]);
        std::cout << str << std::endl;
    }
    buffer(int buffer_size = 1024);
    ~buffer() = default;
    std::string read_all();//读出所有未读数据
    void add_data(const std::string& str);//写入数据
    void add_data(const char* buff, int buff_len);
    ssize_t read_fd(int fd, int* error_ptr);//读fd里的数据，然后写到buffer里面
    ssize_t write_fd(int fd, int* error_ptr);//把buffer里面的能读的数据写到fd里面
    inline const char* read_start_pos() {
        return &_buff[_read_pos];
    }
    inline const char* write_start_pos() {
        return &_buff[_write_pos];
    }
    inline ssize_t readable_bytes() {
        return _write_pos - _read_pos;
    }
    inline ssize_t writeable_bytes() {
        return _buff.size() - _read_pos;
    }
    inline void read_to(const char* read_end_ptr) {
        _read_pos += read_end_ptr - (static_cast<const char*>(&*_buff.begin()) + _read_pos);
    }
    inline void write_add(int w) {
        _write_pos += w;
    }
    inline void read_add(int r) {
        _read_pos += r;
    }
    void clear();
};
#endif