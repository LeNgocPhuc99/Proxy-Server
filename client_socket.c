#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>


#include "network.h"
#include "epoll_interface.h"
#include "backend_socket.h"
#include "client_socket.h"
#include "loadbalancer.h"


#define BUFFER_SIZE 4096


void really_close_client_socket(struct epoll_event_handler* self)
{
    close(self->fd);
    free(self->closure);
    free(self);
}

struct epoll_event_handler* connect_to_backend(struct epoll_event_handler* client_handler, int epoll_fd, char* backend_host, char* backend_port, struct webserver* webload_data)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int getaddrinfo_error;
    struct addrinfo* addrs;
    getaddrinfo_error = getaddrinfo(backend_host, backend_port, &hints, &addrs);
    free(backend_host);
    if (getaddrinfo_error != 0) 
    {
        if (getaddrinfo_error == EAI_SYSTEM) 
        {
            fprintf(stderr, "Couldn't find backend: system error: %s\n", strerror(errno));
        } 
        else 
        {
            fprintf(stderr, "Couldn't find backend: %s\n", gai_strerror(getaddrinfo_error));
        }
        exit(1);
    }

    int backend_socket_fd;
    struct addrinfo* addrs_iter;
    for (addrs_iter = addrs; addrs_iter != NULL; addrs_iter = addrs_iter->ai_next)
    {
        backend_socket_fd = socket(addrs_iter->ai_family, addrs_iter->ai_socktype, addrs_iter->ai_protocol);
        if (backend_socket_fd == -1) 
        {
            continue;
        }

        if (connect(backend_socket_fd, addrs_iter->ai_addr, addrs_iter->ai_addrlen) != -1)
        {
            break;
        }

        close(backend_socket_fd);
    }

    if (addrs_iter == NULL) 
    {
        fprintf(stderr, "Couldn't connect to backend");
        exit(1);
    }

    freeaddrinfo(addrs);

    struct epoll_event_handler* backend_socket_event_handler;
    backend_socket_event_handler = create_backend_socket_handler(backend_socket_fd, client_handler, webload_data);
    epoll_add_handler(epoll_fd, backend_socket_event_handler, EPOLLIN | EPOLLRDHUP | EPOLLET);

    webload_data->count_req1 += 1;
    printf("Data at client socket: Web addr: %s, num_req: %d\n", webload_data->webaddr1, webload_data->count_req1);
    return backend_socket_event_handler;
}


bool make_request(char* buffer, char** backend_addr)
{

    char command[BUFFER_SIZE];                  // GET
    char url[BUFFER_SIZE];                      // http://example.com/
    char http[BUFFER_SIZE];                     // HTTP/1.1
    char host[BUFFER_SIZE];                     // example.com
    char temp[BUFFER_SIZE];                     // http://example.com/
    char *entry;

    bool flag = false;
    // Search for cookie in http header
    char *needle = strstr(buffer, "Cookie: ");
    char cookie[40];
    // If cookie is present, send to that server
    if(needle != NULL)
    {
        for (size_t i = 0; i < 40; i++)
        {
            if (needle[i] == '\r')
            {
                break;
            }
            cookie[i] = needle[i];
        }
        char *token;
        token = strtok(cookie, "=");
        token = strtok(NULL, "=");
        strcpy(cookie, token);
        *backend_addr = strdup(cookie);
    }
    
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
            strcpy(host, *backend_addr);
        }
        entry = strtok(temp, "//");
        entry = strtok(NULL, "/");
        entry = strtok(NULL, "\0");

        // make request
        bzero(buffer, BUFFER_SIZE);
        if(entry == NULL)
        {   
            sprintf(buffer, "%s / %s\r\nHost: %s\r\nConnection: close\r\n\r\n", command, http, host);
        }
        else
        {
            sprintf(buffer, "%s /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", command, entry, http, host);
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


// fix bugs write()

void add_write_buffer_entry(struct client_socket_event_data* closure, struct data_buffer_entry* new_entry) 
{
    struct data_buffer_entry* last_buffer_entry;
    if (closure->write_buffer == NULL) 
    {
        closure->write_buffer = new_entry;
    } 
    else 
    {
        last_buffer_entry = closure->write_buffer;
        while (last_buffer_entry->next != NULL)
        {
            last_buffer_entry->next;
        }
        
        //for (last_buffer_entry=closure->write_buffer; last_buffer_entry->next != NULL; last_buffer_entry=last_buffer_entry->next);
        last_buffer_entry->next = new_entry;
    }
}


void write_to_client(struct epoll_event_handler* self, char* data, int len)
{
    struct client_socket_event_data* closure = (struct client_socket_event_data* ) self->closure;

    int written = 0;
    if (closure->write_buffer == NULL) 
    {
        written = write(self->fd, data, len);
        if (written == len) 
        {
            return;
        }
    }
    if (written == -1) 
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK) 
        {
            perror("Error writing to client");
            exit(-1);
        }
        written = 0;
    }

    int unwritten = len - written;
    struct data_buffer_entry* new_entry = malloc(sizeof(struct data_buffer_entry));
    new_entry->is_close_message = 0;
    new_entry->data = malloc(unwritten);
    memcpy(new_entry->data, data + written, unwritten);
    new_entry->current_offset = 0;
    new_entry->len = unwritten;
    new_entry->next = NULL;

    add_write_buffer_entry(closure, new_entry);
}

// check buffer before close
void close_client_socket(struct epoll_event_handler* self)
{
    struct client_socket_event_data* closure = (struct client_socket_event_data* ) self->closure;
    // no data in buffer
    if (closure->write_buffer == NULL) 
    {
        really_close_client_socket(self);
    } 
    else 
    {
        // add new_entry with flag is_close_message = 1 and no data
        struct data_buffer_entry* new_entry = malloc(sizeof(struct data_buffer_entry));
        new_entry->is_close_message = 1;
        new_entry->next = NULL;

        add_write_buffer_entry(closure, new_entry);
    }
}

void handle_client_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    struct client_socket_event_data* closure = (struct client_socket_event_data* ) self->closure;
    

    // send data in the buffer to the client if event is EPOLLOUT and buffer has data
    if ((events & EPOLLOUT) && (closure->write_buffer != NULL)) 
    {
        int written;
        int to_write;
        struct data_buffer_entry* temp;
         // send until buffer is empty
        while (closure->write_buffer != NULL) 
        {
            // check close message
            if (closure->write_buffer->is_close_message) 
            {
                // close socket 
                really_close_client_socket(self);
                free(closure->write_buffer);
                closure->write_buffer = NULL;
                return;
            }

            to_write = closure->write_buffer->len - closure->write_buffer->current_offset;
            // write data to client from current_offset
            written = write(self->fd, closure->write_buffer->data + closure->write_buffer->current_offset, to_write);
            // error
            if (written != to_write) 
            {
                if (written == -1) 
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) 
                    {
                        perror("Error writing to client");
                        exit(-1);
                    }
                    written = 0;
                }
                closure->write_buffer->current_offset += written;
                break;
            } 
            else 
            {
                // all data in a buffer write success
                // delete first node in link list
                temp = closure->write_buffer;
                closure->write_buffer = closure->write_buffer->next;
                free(temp->data);
                free(temp);
            }
        }
    }

    char read_buffer[BUFFER_SIZE];
    int bytes_read;

    // read data from client and send to backend server if event is EPOLLIN
    if (events & EPOLLIN) 
    {
        while ((bytes_read = read(self->fd, read_buffer, BUFFER_SIZE)) != -1 && bytes_read != 0) 
        {
            if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
            {
                return;
            }

            if (bytes_read == 0 || bytes_read == -1) 
            {
                printf("Close when end recv data from client\n");
                close_client_socket(self);
                return;
            }
            // check cookie && connect to backend

            char* backend_addr = select_backend_addr(closure->webload_data);
            if(make_request(read_buffer, &backend_addr))
            {
                struct epoll_event_handler* backend_handler = connect_to_backend(self, closure->epoll_fd, backend_addr, closure->webload_data->backend_port, closure->webload_data);
                write(backend_handler->fd, read_buffer, bytes_read);
            }
            else
            {
                return;
            }

        }
    }
 
}


struct epoll_event_handler* create_client_socket_handler(int client_socket_fd, int epoll_fd, struct webserver* webload_data)
{
    
    make_socket_non_blocking(client_socket_fd);

    struct client_socket_event_data* closure = malloc(sizeof(struct client_socket_event_data));

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = client_socket_fd;
    result->handle = handle_client_socket_event;
    result->closure = closure;

    
    closure->write_buffer = NULL;
    //closure->backend_host = backend_host;
    //closure->backend_port = backend_port;
    closure->epoll_fd = epoll_fd;
    closure->webload_data = webload_data;

    return result;
}
