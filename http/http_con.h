#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sql_RAII.h"
#include "../buffer/buffer.h"
#include "http_request.h"
#include "http_response.h"
class http_con {
public:
    void check() {
        _read_buffer.read_check();
    }
    http_con();
    ~http_con();
    void init(int sock_fd, const sockaddr_in& addr);
    ssize_t read(int* save_error);
    ssize_t write(int* save_error);
    void shut_down();
    int get_fd()const;
    int get_port()const;
    const char* get_ip()const;
    sockaddr_in get_addr()const;
    bool process();
    bool is_keep_alive()const {
        return _request.is_keep_alive();
    }
    int to_write_bytes() { 
        return _iov[0].iov_len + _iov[1].iov_len; 
    }
    static bool is_ET;
    static const char* src_dir;
    static std::atomic<int> user_count;
private:
    int _fd;
    sockaddr_in _addr;
    bool _is_close;
    http_request _request;
    http_response _response;
    buffer _read_buffer;
    buffer _write_buffer;
    int _iov_cnt;
    struct iovec _iov[2];
};

#endif