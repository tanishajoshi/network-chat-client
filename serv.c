#define _GNU_SOURCE
#include <sys/socket.h>
#include <ldap_features.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
//#include <malloc.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#ifndef SOCK_NONBLOCK
 #include <fcntl.h>
 # define SOCK_NONBLOCK O_NONBLOCK
 #endif

#define MY_SOCK_PATH "/somepath"
#define LISTEN_BACKLOG 50
#define BUFFER_SIZE 256
#define NAMESIZE 32
#define handle_error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while (0)

//Create a structure to keep track of the clients in the server

typedef struct client {
        struct client *next;
        char *name;
        int cfd; //connection file descriptor
} Client;

void clientExit(Client *);
void *ignore();

Client *first = NULL;
int c;
char *quit = "quit";
char *name = "name";

//void *ignore() {}

//To print the message along with the name. For example User 1: Hi

char* strCombine(const char *s1, const char *s2)
{
        char *retStr = malloc(strlen(s1) + strlen(s2) + 1);
        strcpy(retStr, s1);
        strcat(retStr, s2);
        return retStr;

}

//New Client join the server

Client *clientJoin(int cfd){
        //If a client is first to join the server
        if (first == NULL){
                first = malloc(sizeof(Client));
               first->name = malloc(NAMESIZE);
                snprintf(first->name, NAMESIZE, "User %d", c++);
                first->cfd = cfd; //Creates new connected socket in the server
                first->next = NULL;

                return first;
        }
        //Add new client to the end of the list
        else{
                Client *ptr = first;
                while (ptr ->next != NULL)
                        ptr = ptr->next;

                Client *next = malloc(sizeof(Client));
                next->name = malloc(NAMESIZE);
                snprintf(next->name, NAMESIZE, "User %d", c++);
                next->cfd = cfd;
                next->next = NULL;

                ptr->next = next;

                return next;
        }

}

Client *clientRemove(int cfd){
        if(first != NULL){
                Client *ptr = first;
                Client *prev = NULL;

                while (ptr != NULL && ptr->cfd != cfd){
                        prev = ptr;
                        ptr = ptr->next;
                }

                if (ptr == NULL){
                        return NULL; //Can't find Client
                }

                if (prev == NULL){ //Client is the first in the list
                        first = ptr->next;
                }

                else if (ptr->next == NULL) //Client is the last in the list
                        prev->next = NULL;

                else
                        prev->next = ptr->next;

                return ptr;
        }

        return NULL;
}
//Try to write a message

void writeAttempt(char *msg, Client *ptr){
        if(ptr != NULL){
                int res = write(ptr->cfd, msg, strlen(msg)+1);
        }
}

void writeMess(char *msg,char *msg2, Client *ct){
        Client *ptr = first;
        Client *temp;

        bool val = false;

        Client *fds[BUFFER_SIZE] = {};

        while (ptr != NULL){
                temp = ptr->next;

                if (ptr->cfd != ct->cfd){
                        writeAttempt(msg,ptr);  //Transfer the message to every clients in the server
                }

                ptr = temp;
        }

        printf("%s",msg);
}

void chatExit(Client *ptr){
        char *msg = malloc(BUFFER_SIZE);
        sprintf(msg,"\n%s has disconnected\n", ptr->name);
        writeMess(msg, NULL, ptr);

        shutdown(ptr->cfd, SHUT_RDWR);
        clientRemove(ptr->cfd);
        free(ptr);
}

//Check to see if the client attemp to change their name
int nameCheck(char *buffer){
        if (strlen(buffer) >= 4){
                return !strncmp(buffer, name, strlen(name));
        }

        return 0;
}

void nameChange(Client *ct, char *buffer){
        int nameLen = strlen(buffer);
        char *msg = malloc(BUFFER_SIZE);
        char *name;
        if (nameLen > 5 && buffer[4] == ' '){ // The client tries to change name with the syntax name yourName
                char temp[nameLen-4];
                int i = 5;
                int k = 0;
                for (i; i < nameLen-1; i++){
                        if (buffer[i] != ' ')
                                temp[k++] = buffer[i]; //The name will start after the space
                }

                if (k == 0){
                        sprintf(msg,"Invalid\n");
                        writeAttempt(msg, ct); //If there's nothing after the parameter name, the client will continue with the default name
                }
                else{
                        temp[k] = '\0';

                        sprintf(msg, "%s has changed name to: \"%s\"\n", ct->name, temp);
                        writeMess(msg, NULL, ct);

                        for (k; k > -1; k--)
                                ct->name[k] = temp[k];
                }
        }
        else{
                sprintf(msg, "Invalid\n");
                writeAttempt(msg, ct);
        }
}

void readPort(){
        Client *ptr = first;
        char buffer[BUFFER_SIZE];
        bool del = false;
        Client *temp = NULL;

        while(ptr != NULL){
                temp = ptr -> next;

                while(1){
                        int size = read(ptr->cfd, buffer, BUFFER_SIZE);
                        if (size == 0){
                                chatExit(ptr);
                                break;
                        }

                        else if (size < 0) break;

                        else{
                                if(!strncmp(buffer, quit, strlen(quit))){
                                        chatExit(ptr);
                                        break;
                                }

                                else if (nameCheck(buffer))
                                        nameChange(ptr, buffer);
                                else{
                                        char *sta = strCombine("",ptr->name);
                                        sta = strCombine(sta, ": ");
                                        sta = strCombine(sta, buffer);

                                        char *name = strCombine(ptr->name, ":");
                                        writeMess(sta, name, ptr);
                                }
                        }

                        memset(buffer, 0, BUFFER_SIZE);
                }

                ptr = temp;

        }
}

int main(int argc, char *argv[]){

        if (argc != 3){
                printf("Usage: ./server <IP address> <Port Number>\n");
                return 0;
        }

        signal(SIGPIPE, (void *)ignore);

        int portNum = atoi(argv[2]);

        int sfd, cfd;
        int yes = 1;

        char text[BUFFER_SIZE];
        char *msg = malloc(BUFFER_SIZE);
        char *msg2 = malloc(BUFFER_SIZE);

        struct sockaddr_in my_addr, peerAddr;

        socklen_t peerAddrSize;

        sfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (sfd == -1)
                handle_error("socket");

        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
                handle_error("failed");
        memset(&my_addr, 0, sizeof(struct sockaddr_in));
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(portNum);

        if (inet_aton(argv[1], (struct in_addr *) &my_addr.sin_addr) == 0)
                handle_error("Failed");
        if(bind(sfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in)) == -1)
                handle_error("bind");

        if (listen(sfd, LISTEN_BACKLOG) == -1)
                handle_error("listen");
        peerAddrSize = sizeof(struct sockaddr_in);

        while (1) {
                readPort();

                cfd = accept(sfd, (struct sockaddr *) &peerAddr, &peerAddrSize);
                if (cfd != -1){
                        Client *new = clientJoin(cfd);

                        if (new != NULL){
                                sprintf(msg, "%s has joined the chat room\n", new->name);
                                sprintf(msg2, "You joined the chat as: \"%s\"\n", new->name);
                                writeMess(msg, msg2, new);
                                writeAttempt(msg2, new);
                        }

                }

        }

        unlink(MY_SOCK_PATH);
}


