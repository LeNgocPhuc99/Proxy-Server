#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PORT 50000
#define LENQUEUE 5
#define BUFFER 256
#define SERVER_IP "127.0.0.1"
#define MAX_PATH_LENGTH 512
static FILE *fp = NULL;
void sync_init()
{
    // Create tag name
    char tag[20] = {0};
    time_t now;
    time(&now);
    strftime(tag, 20, "%y%m%d", localtime(&now));
    char fullpath[MAX_PATH_LENGTH];
    sprintf(fullpath, "%s%s_%s.%s", "./", "loadbalancer", tag, "log");
    fp = fopen(fullpath, "a");
    /* Khong mo file duoc thi in ra stdout */
    if (fp == NULL)
    {
        perror("opening log file");
        fp = stdout;
    }
}

int main(int argc, char const *argv[])
{
    sync_init();
    int waittime = 50;
    int sockfd, newsock;
    char sendmess[BUFFER], recvmess[BUFFER];
    struct sockaddr_in serveraddr, cliaddr;

    int clilen, len;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "server-socket() error!\n");
        exit(1);
    }
    else
    {
        printf("Create successful!\n");
    }

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serveraddr.sin_port = htons(PORT);
    bzero(&serveraddr.sin_zero, 8);

    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        fprintf(stderr, "server-bind() error!\n");
        exit(1);
    }
    printf("Bind successfull!\n");

    listen(sockfd, LENQUEUE);

    while (1)
    {
        clilen = sizeof(cliaddr);
        newsock = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
        if (newsock < 0)
        {
            fprintf(stderr, "server-accept() error!! \n");
            break;
        }
        
        recv(newsock, recvmess, BUFFER, 0);
        printf("Received: %s\n", recvmess);
        fputs(recvmess, fp);
        fflush(fp);
        close(newsock);
    }
    close(sockfd);
    fclose(fp);
    return 0;
}
