#include"buffer.h"
#include<string.h>
void buffer::make_space(std::size_t len) {
    auto n = _buff.size();
    if (n - _write_pos + _read_pos < len) {
        _buff.resize(len + _write_pos + 1);
        reorder_buffer();
    }
    else if (n-_write_pos-1<len) {
        reorder_buffer();
    }
}
buffer::buffer(int buffer_size) :_buff(buffer_size), _read_pos(0), _write_pos(0) {}
void buffer::add_data(const std::string& str) {
    auto c_str = str.data();
    auto len = str.length();
    make_space(len);
    std::copy(c_str, c_str + len, &*_buff.begin() + _write_pos);
    _write_pos += len;    
}
void buffer::add_data(const char* buff, int buff_len) {
    make_space(buff_len);
    std::copy(buff, buff + buff_len, &*_buff.begin() + _write_pos);
    _write_pos += buff_len;
}
ssize_t buffer::read_fd(int fd, int* error_ptr) {
    char buff[65535] = { 0 };
    iovec iov[2];
    auto wtb = writeable_bytes();
    iov[0].iov_base = (void*)write_start_pos();
    iov[0].iov_len = wtb;
    iov[1].iov_base = buff;
    iov[1].iov_len = 65535;
    auto len = readv(fd, iov, 2);
    if (len <= 0) {
        *error_ptr = errno;
        return -1;
    }
    else if(len<wtb){
        _write_pos += len;
    }    
    else {
        _write_pos = _buff.size();
        add_data((char*)iov[1].iov_base, len - wtb);
    }
    return len;
}
ssize_t buffer::write_fd(int fd, int* error_ptr) {
    int len = write(fd, &*_buff.begin() + _read_pos, _write_pos - _read_pos);
    if (len < 0) {
        *error_ptr = errno;
        return -1;
    }
    else {
        _read_pos += len;
        return 0;
    }
}
void buffer::clear() {
    bzero(&_buff[0], _buff.size());
    _read_pos = 0;
    _write_pos = 0;
}
std::string buffer::read_all() {
    std::string return_val(read_start_pos(), readable_bytes());
    clear();
    return return_val;
}