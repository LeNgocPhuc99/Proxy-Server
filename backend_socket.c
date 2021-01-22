#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>
#include <arpa/inet.h>

#include "network.h"
#include "epoll_interface.h"
#include "backend_socket.h"
#include "client_socket.h"
#include "loadbalancer.h"
#include "log.h"

#define BUFFER_SIZE 4096



void close_backend_socket(struct epoll_event_handler* self)
{
    struct backend_socket_event_data* closure = malloc(sizeof(struct backend_socket_event_data));
    closure = (struct backend_socket_event_data*)self->closure;
    if(strcmp(closure->backend_host, closure->webload_data->webaddr1) == 0)
    {
        closure->webload_data->count_req1 -= 1;
        printf("Data at backend socket: Web addr: %s, num_res: %d\n", closure->webload_data->webaddr1, closure->webload_data->count_req1);
    }
    else if (strcmp(closure->backend_host, closure->webload_data->webaddr2) == 0)
    {
        closure->webload_data->count_req2 -= 1;
        printf("Data at backend socket: Web addr: %s, num_res: %d\n", closure->webload_data->webaddr2, closure->webload_data->count_req2);
    }
    
   

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
                //printf("Free client from backend.\n");
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
        // Print to the log file
        struct sockaddr_in raw_addr;
        socklen_t raw_addr_size = sizeof(struct sockaddr_in);
        // Get client address
        getpeername(closure->client_handler->fd, (struct sockaddr *)&raw_addr, &raw_addr_size);
        char client_ip[20];
        int client_port = (int) raw_addr.sin_port;
        strcpy(client_ip, inet_ntoa(raw_addr.sin_addr));
        // Get server address
        char server_ip[20];
        getpeername(self->fd, (struct sockaddr *)&raw_addr, &raw_addr_size);
        strcpy(server_ip, inet_ntoa(raw_addr.sin_addr));
        log_print("Server with IP: %s finished the request of Client with IP: %s and Port: %d\n", server_ip, client_ip, client_port);

        // send_log
        send_log("Server with IP: %s finished the request of Client with IP: %s and Port: %d\n", server_ip, client_ip, client_port);

        //printf("Free client from backend.\n");
        close_client_socket(closure->client_handler);
        close_backend_socket(self);
        return;
    }

}


struct epoll_event_handler* create_backend_socket_handler(int backend_socket_fd, struct epoll_event_handler* client_handler, struct webserver* webload_data, char* backend_host)
{
    make_socket_non_blocking(backend_socket_fd);

    struct backend_socket_event_data* closure = malloc(sizeof(struct backend_socket_event_data));
    closure->client_handler = client_handler;
    closure->webload_data = webload_data;
    closure->backend_host = backend_host;

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = backend_socket_fd;
    result->handle = handle_backend_socket_event;
    result->closure = closure;

    return result;
}

