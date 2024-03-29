//link list
struct data_buffer_entry 
{
    int is_close_message;
    char* data;
    int current_offset;
    int len;
    struct data_buffer_entry* next;
};


struct client_socket_event_data 
{
    //struct epoll_event_handler* backend_handler;
    struct data_buffer_entry* write_buffer;
    int epoll_fd;
    struct webserver* webload_data;
};




extern void handle_client_socket_event(struct epoll_event_handler* self, uint32_t events);

extern void write_to_client(struct epoll_event_handler* self, char* data, int len);

extern void close_client_socket(struct epoll_event_handler* self);

extern struct epoll_event_handler* create_client_socket_handler(int client_socket_fd, int epoll_fd, struct webserver* webload_data);
