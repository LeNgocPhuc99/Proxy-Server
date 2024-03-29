#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
