struct backend_socket_event_data 
{
    struct epoll_event_handler* client_handler;
    struct webserver* webload_data;
    char* backend_host;
};

extern void handle_backend_socket_event(struct epoll_event_handler* self, uint32_t events);

extern void close_backend_socket(struct epoll_event_handler*);

extern struct epoll_event_handler* create_backend_socket_handler(int backend_socket_fd, struct epoll_event_handler* client_handler, struct webserver* webload_data, char* backend_host);
