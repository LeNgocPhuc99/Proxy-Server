#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>

#include "network.h"
#include "epoll_interface.h"
#include "backend_socket.h"
#include "client_socket.h"
#include "loadbalancer.h"


#define BUFFER_SIZE 4096



void close_backend_socket(struct epoll_event_handler* self)
{
    struct backend_socket_event_data* closure = malloc(sizeof(struct backend_socket_event_data));
    closure = (struct backend_socket_event_data*)self->closure;
    closure->webload_data->count_res += 1;
    printf("Data at backend socket: Web addr: %s, num_res: %d\n", closure->webload_data->webaddr, closure->webload_data->count_res);

    close(self->fd);
    free(self->closure);
    free(self);
}

// read data from backend and send to client
void handle_backend_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    struct backend_socket_event_data* closure = (struct backend_socket_event_data*) self->closure;

    char buffer[BUFFER_SIZE];
    int bytes_read;

    if (events & EPOLLIN) 
    {
        // make sure that read every thing
        while ((bytes_read = read(self->fd, buffer, BUFFER_SIZE)) != -1 && bytes_read != 0) 
        {
            if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
            {
                return;
            }

            if (bytes_read == 0 || bytes_read == -1) 
            {
                close_client_socket(closure->client_handler);
                close_backend_socket(self);
                return;
            }
            // send to client
            write_to_client(closure->client_handler, buffer, bytes_read);
        }
        //closure->webload_data->count_res += 1;
    }

    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) 
    {
        close_client_socket(closure->client_handler);
        close_backend_socket(self);
        return;
    }

}


struct epoll_event_handler* create_backend_socket_handler(int backend_socket_fd, struct epoll_event_handler* client_handler, struct webserver* webload_data)
{
    make_socket_non_blocking(backend_socket_fd);

    struct backend_socket_event_data* closure = malloc(sizeof(struct backend_socket_event_data));
    closure->client_handler = client_handler;
    closure->webload_data = webload_data;

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = backend_socket_fd;
    result->handle = handle_backend_socket_event;
    result->closure = closure;

    return result;
}

