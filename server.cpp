#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include<string.h>
#include <unistd.h>

int main()
{
    int listen_fd, comm_fd;
    struct sockaddr_in serv_addr;
    char input[100];
 
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    bzero( &serv_addr, sizeof(serv_addr));
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
    serv_addr.sin_port = htons(20001);
 
    bind(listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    listen(listen_fd, 10);
    comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);
 
    while(1)
    {
        bzero( input, 100);
        read(comm_fd,input,100);
        write(comm_fd, input, strlen(input)+1);
    }
}
