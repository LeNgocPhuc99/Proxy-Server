#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>

#include "network.h"
#include "epoll_interface.h"
#include "server_socket.h"
#include "client_socket.h"

#define MAX_LISTEN_BACKLOG 4096

// info of incoming connection 
struct server_socket_event_data 
{
    int epoll_fd;
    char* backend_addr;
    char* backend_port;
};


void handle_client_connection(int epoll_fd, int client_socket_fd, char* backend_host, char* backend_port) 
{

    struct epoll_event_handler* client_socket_event_handler;
    client_socket_event_handler = create_client_socket_handler(client_socket_fd, epoll_fd, backend_host, backend_port);
   
    // EPOLLET =  edge-triggered mode
    // EPOLLOUT: client connection ready to write
    epoll_add_handler(epoll_fd, client_socket_event_handler, EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLOUT);

}


// accept clinet connect && handle
void handle_server_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    struct server_socket_event_data* closure = (struct server_socket_event_data*) self->closure;

    int client_socket_fd;
    while (1) 
    {
        client_socket_fd = accept(self->fd, NULL, NULL);
        if (client_socket_fd == -1) 
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) 
            {
                break;
            } 
            else 
            {
                perror("Could not accept");
                exit(1);
            }
        }

        handle_client_connection(closure->epoll_fd, client_socket_fd, closure->backend_addr, closure->backend_port);
    }
}


int create_and_bind(char* server_port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    // IPv4 && IPv6
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* addrs;
    int getaddrinfo_error;
    getaddrinfo_error = getaddrinfo(NULL, server_port, &hints, &addrs);
    if (getaddrinfo_error != 0) 
    {
        fprintf(stderr, "Couldn't find local host details: %s\n", gai_strerror(getaddrinfo_error));
        exit(1);
    }

    int server_socket_fd;
    struct addrinfo* addr_iter;
    for (addr_iter = addrs; addr_iter != NULL; addr_iter = addr_iter->ai_next) 
    {

        server_socket_fd = socket(addr_iter->ai_family, addr_iter->ai_socktype, addr_iter->ai_protocol);
        if (server_socket_fd == -1) 
        {
            continue;
        }

        int so_reuseaddr = 1;
        if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr)) != 0) 
        {
            continue;
        }

        if (bind(server_socket_fd, addr_iter->ai_addr, addr_iter->ai_addrlen) == 0)
        {
            break;
        }

        close(server_socket_fd);
    }

    if (addr_iter == NULL) 
    {
        fprintf(stderr, "Couldn't bind\n");
        exit(1);
    }

    freeaddrinfo(addrs);

    return server_socket_fd;
}


struct epoll_event_handler* create_server_socket_handler(int epoll_fd, char* server_port, char* backend_addr, char* backend_port)
{

    int server_socket_fd;
    server_socket_fd = create_and_bind(server_port);
    make_socket_non_blocking(server_socket_fd);

    listen(server_socket_fd, MAX_LISTEN_BACKLOG);

    struct server_socket_event_data* closure = malloc(sizeof(struct server_socket_event_data));
    closure->epoll_fd = epoll_fd;
    closure->backend_addr = backend_addr;
    closure->backend_port = backend_port;

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = server_socket_fd;
    result->handle = handle_server_socket_event;
    result->closure = closure;

    return result;
}


