#define _GNU_SOURCE 
#include <sys/un.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdbool.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#ifndef SOCK_NONBLOCK
 #include <fcntl.h>
 # define SOCK_NONBLOCK O_NONBLOCK
 #endif
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define MY_SOCK_PATH "/somepath"
#define LISTEN_BACKLOG 50
#define BUF_SIZE 256
#define NAMELEN 32
void * ignore();
void renameClient();
void atEndpoint();
int accept4(int socket, struct sockaddr *restrict address,
     socklen_t *restrict address_len, int flags);

//Struct with Client attributes- to be used in LL 
typedef struct client {
        //pointer to next node
        struct client *next;
        //User name
        char *name;
        //Connection fd
        int cfd; 
} Client;


int clientCount;
Client *head = NULL;
char *quit = "quit";
char *name = "name";


// ~ Message write attempt ~
void tryWrite(char *m, Client *ptr){
    if(ptr != NULL){
        int w = write(ptr->cfd, m, strlen(m)+1);
    }
}

//Akin to adding a node (client ) on to the list i think
Client *addClient(int cfd){
    //We instantiate the head of the linked list of clients, if none yet
    if (head == NULL){
        //allocating exact amt of memory aka size of Client struct
        head = malloc(sizeof(Client));
        head->name = malloc(NAMELEN);
        snprintf(head->name, NAMELEN, "User %d", clientCount++);
        //create a new connection
        head->cfd = cfd; 
        head->next = NULL;
        return head;
    } else { 
        //Add new client to the end of the list
        Client *ptr = head;
        while (ptr ->next != NULL)
            ptr = ptr->next;
            Client *next = malloc(sizeof(Client));
            next->name = malloc(NAMELEN);
            
            snprintf(next->name, NAMELEN, "User %d", clientCount++);
            next->cfd = cfd;
            next->next = NULL;
            ptr->next = next;
            return next;
        }

}

Client *rmvClient(int cfd){
        if(head != NULL){
            Client *ptr = head;
            Client *prev = NULL;

            while (ptr != NULL && ptr->cfd != cfd){
                prev = ptr;
                ptr = ptr->next;
            }
            if (ptr == NULL){
                return NULL; //Can't find Client
            }
            if (prev == NULL){ //Client is the first in the list
                head = ptr->next;
            }
            else if (ptr->next == NULL) { //Client is the last in the list
                prev->next = NULL; 
            }
            else { prev->next = ptr->next;}
                return ptr;
        }
        return NULL;
    }


void write_Msg(char *m,char *m2, Client *cli){
    Client *ptr = head;
    Client *tmp;
    while (ptr != NULL){
        tmp = ptr->next;
        if (ptr->cfd != cli->cfd){
            //Send the message to all present on server??
            tryWrite(m,ptr);  
        }
        ptr = tmp;
    }
    printf("%s",m);
}

void closeChat(Client *ptr){
        char *msg = malloc(BUF_SIZE);
        sprintf(msg,"\n%s has disconnected\n", ptr->name);
        write_Msg(msg, NULL, ptr);

        shutdown(ptr->cfd, SHUT_RDWR);
        rmvClient(ptr->cfd);
        free(ptr);
}
//To print the message along with the name. For example User 1: Hi
char* str_Concat(const char *a, const char *b) {       
        char *joined = malloc(strlen(a) + strlen(b) + 1);
        strcpy(joined, a);
        strcat(joined, b);
        return joined;

}
int verify(char *buf) {
    if(strlen(buf) >= 4) {
        int result = !strncmp(buf, name, strlen(name));
        return !result;
    }
    return 0;
}
//read from the port
void atEndPoint(){
    Client *c = head;  // pointer to head of linked list
    Client *tmp = NULL; //tmp pointer
    char buf[BUF_SIZE]; //allocate buffer of size buf_size
    while(c != NULL){  //while there exists first client
        tmp = c -> next; 

        while(1){
            int len = read(c->cfd, buf, BUF_SIZE);
            if (len == 0){
                closeChat(c);
                break;
            } else if (len < 0) break;
            else {
                if(!strncmp(buf, quit, strlen(quit))){
                    closeChat(c);
                    break;
                }
                else if (verify(buf))
                    renameClient(buf,c);
                else{
                    //Concatenating the user string annd the action taken
                    char *toCombine = str_Concat("",c->name);
                    toCombine = str_Concat(toCombine, ": ");
                    toCombine = str_Concat(toCombine, buf);
                    char *name = str_Concat(c->name, ":");
                    write_Msg(toCombine, name, c);
                }
            }
            memset(buf, 0, BUF_SIZE);
        }
        c = tmp;

    }
}


void renameClient(char *buf, Client *cli){
    char *name;
    int len = 4;
    int len_Name = strlen(buf);
    char *message = malloc(BUF_SIZE);
    // 
    if (len_Name > 5 && buf[len] == ' '){ 
        char arr[len_Name-len];
        int ctr = 5;
        int ctr1 = 0;
        for (ctr; ctr < len_Name-1; ctr++){
            // Use Case 'name' command: Client enters phrase name fllwd 
            //by a space & check for at least 1 char
            if (buf[ctr] != ' ')
                arr[ctr1++] = buf[ctr]; 
            }
            if (ctr1 == 0){
                sprintf(message,"Invalid\n");
                tryWrite(message, cli); 
            }
            else{
                arr[ctr1] = '\0';

                sprintf(message, "%s has changed name to: \"%s\"\n", cli->name, arr);
                write_Msg(message, NULL, cli);
                for (ctr1; ctr1 > -1; ctr1--)
                    cli->name[ctr1] = arr[ctr1];
                }
    } else{
        sprintf(message, "Invalid\n");
        tryWrite(message, cli);
    }
}

int main(int argc, char *argv[]){
    if (argc != 3){
        printf("Usage: ./server <IP address> <Port Number>\n");
        return 0;
    }

    signal(SIGPIPE, (void *)ignore);

    int port_no = atoi(argv[2]);
    char *msg = malloc(BUF_SIZE);
    char *msg2 = malloc(BUF_SIZE);
    char msgContent[BUF_SIZE];
    
    int sfd, cfd;
    int option_value = 1; //used for setting socket option
    struct sockaddr_in self_ad, peer_ad;
    socklen_t peer_adSize;
    peer_adSize = sizeof(struct sockaddr_in);
    sfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sfd == -1) {perror("socket"); exit(EXIT_FAILURE); }
    // allow reuse of local addresses 
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) < 0) {
        perror("Error: Unable to set socket options for reuse of addresses");
        exit(EXIT_FAILURE);
    }
    memset(&self_ad, 0, sizeof(struct sockaddr_in));    

    self_ad.sin_family = AF_INET;    
    self_ad.sin_port = htons(port_no);    

    if (inet_aton(argv[1], (struct in_addr *) &self_ad.sin_addr) == 0) {
        perror("Error: Could not convert to network byte address");
        exit(EXIT_FAILURE);
    }
    if(bind(sfd, (struct sockaddr *) &self_ad, sizeof(struct sockaddr_in)) == -1) {
        perror("Error: Bind failed");
        exit(EXIT_FAILURE);
    }       
    
    if (listen(sfd, LISTEN_BACKLOG) == -1)
        perror("Error: Incoming connections not accpeted");
    
    
    while (1) {
        atEndpoint(); //read from the port!
        cfd = accept4(sfd, (struct sockaddr *) &peer_ad, &peer_adSize, SOCK_NONBLOCK);
        if (cfd != -1){
            Client *new = addClient(cfd);

            if (new != NULL){
                sprintf(msg, "%s has joined the chat\n", new->name);
                sprintf(msg2, "You joined the chat as: \"%s\"\n", new->name);
                write_Msg(msg, msg2, new);
                tryWrite(msg2, new);
            }
        }
    }
    unlink(MY_SOCK_PATH);
}
