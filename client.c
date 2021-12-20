
//Client Usage: client <IP address> <port number>
//Citation: Linux man (7) unix
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#define DEF_PORT 2808  //idk
#define SIZE 1024 
//initialize socket file desc
int sfd; 


void * receiver(int);
void * sender(int);

struct sockaddr_in server_ad;
char buf[SIZE];


void discardOutput(FILE *fp) {
    int c;
    while ((c = getc(fp)) != EOF && c != '\n')
        putc('\0',fp);
}


void *sender(int sfd) {
    while(1) {
	fgets(buf, SIZE-1, stdin);

	int outputfd = write(sfd, buf, strlen(buf)+1);
	int comp_size = 4;
	//testing if client entered quit cmd
	if (strncmp(buf, "quit", comp_size) == 0)
	    //printf("Quit chat successfully");
	    exit(0);
	    memset(buf, 0, SIZE); //memsetting the buffer

    }
}

//Receive a message
void *receiver(int sfd) {
    while(1) {
        int inputfd;
	memset(buf, 0, SIZE);
	inputfd = read(sfd, buf, SIZE-1);
	if (inputfd == -1) { perror("Error receiving"); }
	
	if (inputfd == 0) {
	   printf("Closing server...\n");
	   exit(0);
		} 
	if (inputfd > 0) {
	    //clearing stdout stream
	    discardOutput(stdout);  
	    printf("%s", buf);
	    memset(buf, 0, SIZE); //memset the buffer
	}	
    } 
}

int main (int argc, char* argv[]) {
    while (argc != 3 ) {
	printf("Usage: ./client <IP address> <Port Number>\n");
	exit(0);
    }
    pthread_t reader, writer;

    //Establish the socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
	perror("Error: Could not create socket\n");
    }

    //Address configuration w sockets
    memset((char *) &server_ad, 0, sizeof(server_ad));
    //set family... converting port number
    server_ad.sin_family = AF_INET; 
    //set port
    server_ad.sin_port = htons(atoi(argv[2])); 
    server_ad.sin_addr.s_addr = inet_addr(argv[1]);

    //create the connection
    if(connect(sfd, (struct sockaddr *) &server_ad, sizeof(server_ad)) == -1){
       perror("Error: Could not connect to server\n");
       exit(0);
    } else {
	printf("Connection to server established\n");
    }
    pthread_create(&writer,NULL,(void *)sender,(void *)(intptr_t)sfd);
    pthread_create(&reader,NULL,(void *)receiver,(void *)(intptr_t)sfd);
    pthread_join(writer,NULL);
    pthread_join(reader,NULL);
}

