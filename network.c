#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


#include "network.h"

void make_socket_non_blocking(int socket_fd)
{
    int flags;

    flags = fcntl(socket_fd, F_GETFL, 0);
    if(flags == -1)
    {
        perror("fcntl() error");
        exit(1);
    }

    flags |= O_NONBLOCK;
    if(fcntl(socket_fd, F_SETFL, flags) == -1)
    {
        perror("fcntl() error");
        exit(-1);
    }
}

