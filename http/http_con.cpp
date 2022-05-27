#include "http_con.h"
#include <iostream>
const char* http_con::src_dir;
std::atomic<int> http_con::user_count;
bool http_con::is_ET;
http_con::http_con() :_fd(-1), _is_close(true) {
    _addr = { 0 };
}
http_con::~http_con() {
    shut_down();
}
void http_con::init(int fd, const sockaddr_in& addr) {
    user_count++;
    _addr = addr;
    _fd = fd;
    _write_buffer.clear();
    _read_buffer.clear();
    _is_close = false;
    LOG_INFO("Client[%d](%s:%d) in, user_count: %d", fd, get_ip(), get_port(), (int)user_count);
}
void http_con::shut_down() {
    _response.unmap_file();
    if (!_is_close) {
        _is_close = true;
        user_count--;
        close(_fd);
        LOG_INFO("Client[%d](%s:%d) out, user_count: %d", _fd, get_ip(), get_port(), (int)user_count);
    }
}
int http_con::get_fd() const {
    return _fd;
}
sockaddr_in http_con::get_addr()const {
    return _addr;
}
const char* http_con::get_ip()const {
    return inet_ntoa(_addr.sin_addr);
}
int http_con::get_port()const {
    return _addr.sin_port;
}
ssize_t http_con::read(int* save_error) {
    ssize_t len = -1;
    int l0 = 0;
    do {
        len = _read_buffer.read_fd(_fd, save_error);
        if (len < 0) {
            break;
        }
        l0 += len;
    } while (is_ET);
    return len;
}
ssize_t http_con::write(int* save_error) {
    ssize_t len = -1;
    do {
        len = writev(_fd, _iov, _iov_cnt);
        if (len <= 0) {
            *save_error = errno;
            break;
        }
        if (_iov[0].iov_len + _iov[1].iov_len == 0) { break; } /* 传输结束 */
        else if (static_cast<size_t>(len) > _iov[0].iov_len) {
            _iov[1].iov_base = (uint8_t*)_iov[1].iov_base + (len - _iov[0].iov_len);
            auto len0 = _iov[1].iov_len;
            _iov[1].iov_len -= (len - _iov[0].iov_len);
            auto len1 = _iov[1].iov_len;
            if (len1 > len0) {
                int a = 1;
            }
            if (_iov[0].iov_len) {
                _write_buffer.clear();
                _iov[0].iov_len = 0;
            }
        }
        else {
            _iov[0].iov_base = (uint8_t*)_iov[0].iov_base + len;
            _iov[0].iov_len -= len;
            _write_buffer.read_add(len);
        }
    } while (is_ET || to_write_bytes() > 10240);
    return len;
}
bool http_con::process() {
    _request.init();
    if (_read_buffer.readable_bytes() <= 0) {
        return false;
    }
    else if (_request.parse(_read_buffer)) {
        LOG_DEBUG("%s", _request.path().c_str());
        _response.init(src_dir, _request.path(), _request.is_keep_alive(), 200);
    }
    else {
        _response.init(src_dir, _request.path(), false, 400);
    }
    _response.make_response(_write_buffer);
    /* 响应头 */
    _iov[0].iov_base = const_cast<char*>(_write_buffer.read_start_pos());
    _iov[0].iov_len = _write_buffer.readable_bytes();
    _iov_cnt = 1;

    /* 文件 */
    if (_response.file_len() > 0 && _response.file()) {
        _iov[1].iov_base = _response.file();
        _iov[1].iov_len = _response.file_len();
        _iov_cnt = 2;
    }
    LOG_DEBUG("body filesize:%d", _response.file_len());
    return true;
}