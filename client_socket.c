#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>


#include "epoll_interface.h"
#include "network.h"
#include "backend_socket.h"

#include "client_socket.h"


#define BUFFER_SIZE 4096


struct client_socket_event_data
{
    struct epoll_event_handler* backend_handler;
    char* backend_addr;
};

void close_client_socket(struct epoll_event_handler* self)
{
    close(self->fd);
    free(self->closure);
    free(self);
}


struct epoll_event_handler* connect_to_backend(struct epoll_event_handler* client_handler, int epoll_fd, char* backend_host, char* backend_port)
{
    struct addrinfo hints;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int getaddrinfo_error;
    struct addrinfo* addrs;
    getaddrinfo_error = getaddrinfo(backend_host, backend_port, &hints, &addrs);
    if(getaddrinfo_error != 0)
    {
        perror("connect_to_backend-getaddrinfo() error!");
        exit(1);
    }

    int backend_socket_fd;
    struct addrinfo* addrs_iter;
    for(addrs_iter = addrs; addrs_iter != NULL; addrs_iter = addrs_iter->ai_next)
    {
        backend_socket_fd = socket(addrs_iter->ai_family, addrs_iter->ai_socktype, addrs_iter->ai_protocol);
        if(backend_socket_fd == -1)
        {
            continue;
        }
        if(connect(backend_socket_fd, addrs_iter->ai_addr, addrs_iter->ai_addrlen) != -1)
        {
            break;
        }
        close(backend_socket_fd);
    }

    if(addrs_iter == NULL)
    {
        perror("Couldn't connect to backend");
        exit(1);
    }
    freeaddrinfo(addrs);

    struct epoll_event_handler* backend_socket_event_handler;
    backend_socket_event_handler = create_backend_socket_handler(backend_socket_fd, client_handler);
    epoll_add_handler(epoll_fd, backend_socket_event_handler, EPOLLIN | EPOLLRDHUP);

    return backend_socket_event_handler;
}

bool make_request(char* buffer, char* backend_addr)
{

    char command[BUFFER_SIZE];                  // GET
    char url[BUFFER_SIZE];                      // http://example.com/
    char http[BUFFER_SIZE];                     // HTTP/1.1
    char host[BUFFER_SIZE];                     // example.com
    char temp[BUFFER_SIZE];                     // http://example.com/
    char *entry;

    bool flag = false;

    sscanf(buffer, "%s %s %s", command, url, http);
    if(strcmp(command,"GET") == 0)
    {
        strcpy(temp, url);

        // get hostname
        entry = strtok(buffer, "//");
        entry = strtok(NULL, "/");
        strcpy(host, entry);

        if(strcmp(host, "172.16.45.130") != 0)
        {
            return flag;
        }
        else
        {
            bzero(host, BUFFER_SIZE);
            strcpy(host, backend_addr);
        }
        entry = strtok(temp, "//");
        entry = strtok(NULL, "/");
        entry = strtok(NULL, "\0");

        // make request
        bzero(buffer, BUFFER_SIZE);
        if(entry == NULL)
        {   
            sprintf(buffer, "%s / %s\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n", command, http, host);
        }
        else
        {
            sprintf(buffer, "%s /%s %s\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n", command, entry, http, host);
        }
        printf("Request:\n %s", buffer);
        flag = true;
        return flag;
    }
    else
    {
        return flag;
    } 
}

void handle_client_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    struct client_socket_event_data* closure = (struct client_socket_event_data* ) self->closure;

    char buffer[BUFFER_SIZE];
    int bytes_read;

    // read data from client and send to backend server
    if (events & EPOLLIN) {
        bytes_read = read(self->fd, buffer, BUFFER_SIZE);
        if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }

        if (bytes_read == 0 || bytes_read == -1) {
            close_backend_socket(closure->backend_handler);
            close_client_socket(self);
            return;
        }

        if(make_request(buffer, closure->backend_addr))
            write(closure->backend_handler->fd, buffer, bytes_read);
    }

    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        close_backend_socket(closure->backend_handler);
        close_client_socket(self);
        return;
    }

}



struct epoll_event_handler* create_client_socket_handler(int client_socket_fd, int epoll_fd, char* backend_addr, char* backend_port)
{
    make_socket_non_blocking(client_socket_fd);

    struct client_socket_event_data* closure = malloc(sizeof(struct client_socket_event_data));

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = client_socket_fd;
    result->handle = handle_client_socket_event;
    result->closure = closure;

    closure->backend_handler = connect_to_backend(result, epoll_fd, backend_addr, backend_port);
    closure->backend_addr = backend_addr;
    return result;
};