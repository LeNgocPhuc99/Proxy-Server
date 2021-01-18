#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>


#include "epoll_interface.h"
#include "server_socket.h"

#include "loadbalancer.h"
#include "log.h"



int main(int argc, char* argv[])
{
    if (argc != 4) 
    {
        fprintf(stderr, "Usage: %s <server_port> <backend_addr> <backend_port>\n", argv[0]);
        exit(1);
    }
    char* server_port = argv[1];
    char* backend_addr1 = argv[2];
    char* backend_addr2 = argv[3];

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) 
    {
        perror("Couldn't create epoll FD");
        exit(1);
    }
    log_init(".", "loadbalancer", "log");

    struct webserver* webload_data;
    webload_data = init_loadbalancer(backend_addr1, backend_addr2);

    //webload_data->count_res = 0;

    struct epoll_event_handler* server_socket_event_handler;
    server_socket_event_handler = create_server_socket_handler(epoll_fd, server_port, webload_data);

    epoll_add_handler(epoll_fd, server_socket_event_handler, EPOLLIN | EPOLLET);

    printf("Started.  Listening on port %s.\n", server_port);
    epoll_reactor_loop(epoll_fd);

    return 0;
}

