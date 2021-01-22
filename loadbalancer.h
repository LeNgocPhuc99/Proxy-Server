struct webserver
{
    // webserver 1
    char* webaddr1;
    int count_req1;
    
    // webserver 2 
    char* webaddr2;
    int count_req2;

    char* backend_port;
};

#define PORT 50000
#define BUFFER 256
#define SERVER_IP "127.0.0.1"

extern struct webserver* init_loadbalancer(char* backend_addr1, char* backend_addr2);

extern char* select_backend_addr(struct webserver* webload_data);

extern  void send_log();