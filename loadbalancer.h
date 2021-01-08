struct webserver
{
    char* webaddr;
    int count;
};

struct loadbalancer_data
{
    struct webserver* data[3];
};
