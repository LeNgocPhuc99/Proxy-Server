struct webserver
{
    char* webaddr;
    int count_req;
    int count_res;
};

struct loadbalancer_data
{
    struct webserver* data[3];
};
