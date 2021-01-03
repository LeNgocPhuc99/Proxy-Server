#include <stdint.h>

struct  epoll_event_handler
{
    int fd;
    // callback func to handle epoll event
    void (*handle)(struct epoll_event_handler*, uint32_t);
    void* closure;
};



extern void epoll_add_handler(int epoll_fd, struct epoll_event_handler* handler, uint32_t event_mask);
extern void epoll_reactor_loop(int epoll_fd);


