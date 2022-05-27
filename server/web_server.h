#ifndef WEB_SERVER_H 
#define WEB_SERVER_H 
#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../pq_timer/timer.h"
#include "../pool/sql_pool.h"
#include "../pool/thread_pool.h"
#include "../pool/sql_RAII.h"
#include "../http/http_con.h"
class web_server {
public:
    enum triger_mode {
        con_ET,
        listen_ET,
        con_listen_ET
    };
    web_server(int port, triger_mode tm, int time_out_ms, bool opt_linger,
        int sql_port, const char* sql_user, const char* sql_pwd,
        const char* db_name, int conn_pool_num, int thread_num,
        bool open_log, int log_level, int log_size);
    ~web_server();
    void start();
private:
    bool _init_socket();
    void _add_client(int fd, sockaddr_in addr);
    void _deal_listen();
    void _deal_write(http_con* client);
    void _deal_read(http_con* client);
    void _send_error(int fd, const char* info);
    void _extent_time(http_con* client);
    void _close_con(http_con* client);
    void _on_read(http_con* client);
    void _on_write(http_con* client);
    void _on_process(http_con* client);
    static const int MAX_FD = 65536;
    static int _set_fd_non_block(int fd);
    int _port;
    bool _open_linger;
    int _time_out_ms;
    bool _is_close;
    int _listen_fd;
    char* _src_dir;
    uint32_t _listen_event;
    uint32_t _con_event;
    std::unique_ptr<timer> _timer;
    std::unique_ptr<thread_pool> _thread_pool;
    std::unique_ptr<epoller> _epoller;
    std::unordered_map<int, http_con> _users;
};
#endif