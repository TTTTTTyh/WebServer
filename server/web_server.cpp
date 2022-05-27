#include "web_server.h"
#include <iostream>
using std::make_unique;

web_server::web_server(int port, triger_mode tm, int time_out_ms, bool open_linger,
    int sql_port, const char* sql_user, const char* sql_pwd,
    const char* db_name, int conn_pool_num, int thread_num,
    bool open_log, int log_level, int log_size) :_port(port), _time_out_ms(time_out_ms), _open_linger(open_linger),
    _is_close(false), _timer(make_unique<timer>()), _thread_pool(make_unique<thread_pool>(thread_num)), _epoller(make_unique<epoller>()) {
    _src_dir = getcwd(nullptr, 256);
    assert(_src_dir);
    strncat(_src_dir, "/resources/", 16);
    http_con::user_count = 0;
    http_con::src_dir = _src_dir;
    sql_pool::get_instance()->init("localhost", sql_port, sql_user, sql_pwd, db_name, conn_pool_num);
    _listen_event = EPOLLRDHUP;
    _con_event = EPOLLONESHOT | EPOLLHUP;
    switch (tm) {
    case con_ET:
        _con_event |= EPOLLET;break;
    case listen_ET:
        _listen_event |= EPOLLET;break;
    case con_listen_ET:
        _listen_event |= EPOLLET;
        _con_event |= EPOLLET;break;
    }
    http_con::is_ET = (_con_event & EPOLLET);
    if (!_init_socket()) _is_close = true;
    if (open_log) {
        Log::instance()->init(log_level, "./record_log", ".log", log_size);
        if (_is_close) { LOG_ERROR("========== Server init error!=========="); }
        else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", _port, (open_linger ? "true" : "false"));
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                (_listen_event & EPOLLET ? "ET" : "LT"),
                (_con_event & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", log_level);
            LOG_INFO("srcDir: %s", http_con::src_dir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", conn_pool_num, thread_num);
        }
    }
}
web_server::~web_server() {
    close(_listen_fd);
    _is_close = true;
    free(_src_dir);
    sql_pool::get_instance()->close_pool();
}
void web_server::_send_error(int fd, const char* info) {
    int res = send(fd, info, strlen(info), 0);
    if (res < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}
void web_server::_close_con(http_con* client) {
    LOG_INFO("Client[%d] quit!", client->get_fd());
    _epoller->del_fd(client->get_fd());
    client->shut_down();
}
void web_server::_add_client(int fd, sockaddr_in addr) {
    _users[fd].init(fd, addr);
    if (_time_out_ms > 0) {
        _timer->add_task(fd, _time_out_ms, std::bind(&web_server::_close_con, this, &_users[fd]));
    }
    _epoller->add_fd(fd, EPOLLIN | _con_event);
    _set_fd_non_block(fd);
    LOG_INFO("Client[%d] in!", _users[fd].get_fd());
}
void web_server::_deal_listen() {
    socklen_t len = sizeof(sockaddr_in);
    sockaddr_in sock;
    do {
    int fd = accept(_listen_fd, (struct sockaddr*)&sock, &len);
    if (fd <= 0) return;
    else if (http_con::user_count >= MAX_FD) {
        _send_error(fd, "server busy!");
        LOG_WARN("clients are full!");
    }
    _add_client(fd, sock);
    } while (_listen_event & EPOLLET);
}
void web_server::_deal_read(http_con* client) {
    _extent_time(client);
    _thread_pool->add_task(std::function<void()>(std::bind(&web_server::_on_read, this, client)));
}
void web_server::_deal_write(http_con* client) {
    _extent_time(client);
    _thread_pool->add_task(std::function<void()>(std::bind(&web_server::_on_write, this, client)));
}
void web_server::_extent_time(http_con* client) {
    if (_time_out_ms > 0) {
        _timer->adjust(client->get_fd(), _time_out_ms);
    }
}
void web_server::_on_read(http_con* client) {
    int res = -1, read_error;
    res = client->read(&read_error);
    // client->check();
    if (res <= 0 && read_error != EAGAIN) {
        _close_con(client);
        return;
    }
    _on_process(client);
}
void web_server::_on_process(http_con* client) {
    if (client->process()) {
        _epoller->mod_fd(client->get_fd(), EPOLLOUT | _con_event);
    }
    else {
        _epoller->mod_fd(client->get_fd(), EPOLLIN | _con_event);
    }
}
void web_server::_on_write(http_con* client) {
    int res = -1, write_error;
    res = client->write(&write_error);
    if (client->to_write_bytes() == 0) {
        if (client->is_keep_alive()) {
            _on_process(client);
            return;
        }
    }
    else if (res < 0) {
        if (write_error == EAGAIN) {
            _epoller->mod_fd(client->get_fd(), _con_event | EPOLLOUT);
            return;
        }
    }
    _close_con(client);
}
bool web_server::_init_socket() {
    int res;
    sockaddr_in addr;
    if (_port > 65535 || _port < 1024) {
        LOG_ERROR("Port:%d Error!", _port);
    }
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = PF_INET;
    addr.sin_port = htons(_port);
    linger linger_opt = { 0 };
    if (_open_linger) {
        linger_opt.l_onoff = 1;
        linger_opt.l_linger = 1;
    }
    _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_fd < 0) {
        LOG_ERROR("Create socket error!,port:%d", _port);
        return false;
    }
    res = setsockopt(_listen_fd, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger));
    if (res < 0) {
        LOG_ERROR("init linger error");
        close(_listen_fd);
        return false;
    }
    int optval = 1;
    res = setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    res = bind(_listen_fd, (const sockaddr*)&addr, sizeof(sockaddr_in));
    if (res < 0) {
        LOG_ERROR("bind error");
        close(_listen_fd);
        return false;
    }
    res = listen(_listen_fd, 6);
    if (res < 0) {
        LOG_ERROR("Listen port:%d error!", _port);
        close(_listen_fd);
        return false;
    }
    res = _epoller->add_fd(_listen_fd, _listen_event | EPOLLIN);
    if (res == 0) {
        LOG_ERROR("add listen error");
        close(_listen_fd);
        return false;
    }
    _set_fd_non_block(_listen_fd);
    LOG_INFO("server port:%d", _port);
    return true;
}
int web_server::_set_fd_non_block(int fd) {
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
void web_server::start() {
    int time_out_ms = -1;
    if (!_is_close) LOG_INFO("========== Server start ==========");
    while (!_is_close) {
        if (_time_out_ms > 0) {
            time_out_ms = _timer->get_next_time();
        }
        int event_cnt = _epoller->wait(time_out_ms);
        for (int i = 0;i < event_cnt;++i) {
            int fd = _epoller->get_fd(i);
            auto events = _epoller->get_fd_events(i);
            if (fd == _listen_fd) {
                _deal_listen();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                _close_con(&_users[fd]);
            }
            else if (events & EPOLLIN) {
                _deal_read(&_users[fd]);
            }
            else if (events & EPOLLOUT) {
                _deal_write(&_users[fd]);
            }
            else {
                LOG_INFO("Unexpected events: %d", events);
            }
        }
    }
}