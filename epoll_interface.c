#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#include "epoll_interface.h"


// add epoll event
void epoll_add_handler(int epoll_fd, struct epoll_event_handler* handler, uint32_t event_mask)
{   
    struct epoll_event event;

    event.data.ptr = handler;
    event.events = event_mask;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->fd, &event) == -1)
    {
        perror("epoll_add_handler() error!\n");
        exit(-1);
    }
}


// waits for incoming events
void epoll_reactor_loop(int epoll_fd)
{
    struct epoll_event current_epoll_event;
    while (1)
    {
        struct epoll_event_handler* handler;
        //block until having an event, 1:one event -1: no timeout
        epoll_wait(epoll_fd, &current_epoll_event, 1, -1);
        handler = (struct epoll_event_handler*) current_epoll_event.data.ptr;
        handler->handle(handler, current_epoll_event.events);
    }
    
}

