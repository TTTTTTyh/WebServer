#ifndef EPOLLER_H 
#define EPOLLER_H 
#include <sys/epoll.h> 
#include <vector>
#include <unistd.h>
class epoller {
public:
    explicit epoller(int max_events = 1024);
    ~epoller();
    bool add_fd(int fd, uint32_t events);
    bool mod_fd(int fd, uint32_t events);
    bool del_fd(int fd);
    int wait(int time_out = -1);
    int get_fd(int i);
    uint32_t get_fd_events(int i);
private:
    std::vector<epoll_event> _fd_events;
    int _max_events;
    int _epoll_fd;
};
#endif