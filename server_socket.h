struct server_socket_event_data 
{
    int epoll_fd;
    struct webserver* webload_data;
};


extern struct epoll_event_handler* create_server_socket_handler(int epoll_fd, char* server_port, struct webserver* webload_data);