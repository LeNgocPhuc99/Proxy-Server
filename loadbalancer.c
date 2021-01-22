#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "loadbalancer.h"

struct webserver* init_loadbalancer(char* backend_addr1, char* backend_addr2) 
{
    struct webserver* webload_data = malloc(sizeof(struct webserver));

    webload_data->webaddr1 = backend_addr1;
    webload_data->count_req1 = 0;

    webload_data->webaddr2 = backend_addr2;
    webload_data->count_req2 = 0;

    webload_data->backend_port = "80";
    
    return webload_data;
}

char* select_backend_addr(struct webserver* webload_data)
{
    if (webload_data->count_req1 > webload_data->count_req2)
    {
        return strdup(webload_data->webaddr2);
    }
    else
    {
        return strdup(webload_data->webaddr1);
    }
}


void send_log(char* log)
{
    int waittime = 50;

    int consock;
    struct sockaddr_in serveraddr;

    int len;
    consock = socket(AF_INET, SOCK_STREAM, 0);
    if(consock < 0)
    {
        fprintf(stderr, "client - socket() error !!\n");
        exit(1);
    }
    else
    {
        printf("created!!\n");
    }
    
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serveraddr.sin_port = htons(PORT);
    bzero(&serveraddr.sin_zero, 8);


    if(connect(consock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        fprintf(stderr, "client-connect() error!\n");
        exit(1);
    }
    else
    {
        printf("connected!!\n");
    }
    send(consock, log, BUFFER, 0);

    wait(&waittime);
    close(consock);
}
