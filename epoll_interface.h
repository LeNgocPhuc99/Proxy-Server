struct epoll_event_handler {
    int fd;
    void (*handle)(struct epoll_event_handler*, uint32_t);
    void* closure;
};

// add epoll event
extern void epoll_add_handler(int epoll_fd, struct epoll_event_handler* handler, uint32_t event_mask);

// waits for incoming events
extern void epoll_reactor_loop(int epoll_fd);
