#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include<string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc,char **argv)
{
    int cli_sock,n;
    char send[100];
    char recv[100];
    struct sockaddr_in serv_addr;
 
    cli_sock=socket(AF_INET,SOCK_STREAM,0);
    bzero(&serv_addr,sizeof serv_addr);
 
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(20001);
 
    inet_pton(AF_INET,"localhost",&(serv_addr.sin_addr));
 
    connect(cli_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
 
    while(1)
    {
        bzero(send, 100);
        bzero(recv, 100);

        fgets(send,100,stdin);
        write(cli_sock,send,strlen(send)+1);
        read(cli_sock,recv,100);

        printf("\n%s\n",recv);
    }
 
}
