#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define DEF_PORT 2808 //default port number
#define DATALEN 256

int sockfd;

struct sockaddr_in serverAddr;

char buffer[DATALEN];

void* chat_read(int);
void* chat_write(int);


void* chat_read(int sockfd)
{       
        while(1){
                int j;
                j = read(sockfd, buffer, DATALEN-1);
                
                if (j==0){
                        printf("Shutting down the server\n");
                        exit(0);
                }
                
                if (j>0){
                        fpurge(stdout);  //Clear the buffer
                        
                        printf("%s", buffer);
                        memset(buffer, 0, DATALEN);
                }
        }
}

void* chat_write(int sockfd)
{
        while(1){
                fgets(buffer, DATALEN-1, stdin);

                int n;
                n = write(sockfd, buffer, strlen(buffer)+1);

                if(strncmp(buffer,"quit",4)==0)
                        exit(0);

                memset(buffer, 0, DATALEN);
        }        
}

int main(int argc, char *argv[]){
        pthread_t t1, t2;

        if( argc != 3){
                printf("Usage: ./client <IP address> <Port Number>\n");
                exit(0);
        }

        //Create a socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
                printf("Error in creating socket\n");
        else
                printf("Socket created\n");

        //Getting Info
        memset((char *) &serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(atoi(argv[2]));
        serverAddr.sin_addr.s_addr = inet_addr(argv[1]);

        //Connect to server
        if(connect(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1){
                printf("Connection Error\n");
                exit(0);
        }
        else
                printf("Welcome to server\n");

        pthread_create(&t2,NULL,(void *)chat_write,(void *)(intptr_t)sockfd);
        pthread_create(&t1,NULL,(void *)chat_read,(void *)(intptr_t)sockfd);
        pthread_join(t2,NULL);
        pthread_join(t1,NULL);

        return 0;

}

//Reference from man 7 unix, example server.c
