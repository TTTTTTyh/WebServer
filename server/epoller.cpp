#include "epoller.h"
#include <assert.h>
epoller::epoller(int max_events) :_epoll_fd(epoll_create(512)),_fd_events(max_events),_max_events(max_events){
    assert(_epoll_fd > 0);
}
epoller::~epoller() {
    close(_epoll_fd);
}
bool epoller::add_fd(int fd, uint32_t events) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}
bool epoller::mod_fd(int fd, uint32_t events) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}
bool epoller::del_fd(int fd) {
    epoll_event ev = {0};
    return 0 == epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &ev);
}
int epoller::wait(int time_out) {
    return epoll_wait(_epoll_fd, &_fd_events[0], _max_events, time_out);
}
int epoller::get_fd(int i) {
    return _fd_events[i].data.fd;
}
uint32_t epoller::get_fd_events(int i) {
    return _fd_events[i].events;
}