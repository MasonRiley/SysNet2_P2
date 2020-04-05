#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include "standards.h"

/* Client structure */
typedef struct {
    struct sockaddr_in tcp_client_address; //Client's address
    int tcp_client_socket; //Client's socket
    int id;                 /* Client unique identifier */
    char name[32];           /* Client name */
} Client;

int numClients = 0;
int id = 1;
Client *clients[MAX_CLIENT];
pthread_mutex_t lock;

/* Send message to all clients but the sender */
void send_message(char *s, int id){
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENT; ++i) {
        if (clients[i]) {
            if (clients[i]->id != id) {
                if (write(clients[i]->tcp_client_socket, s, strlen(s)) < 0) {
                    perror("Write to descriptor failed");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&lock);
}

/* Send message to all clients */
void send_message_all(char *s){
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENT; ++i){
        if (clients[i]) {
            if (write(clients[i]->tcp_client_socket, s, strlen(s)) < 0) {
                perror("Write to descriptor failed");
                break;
            }
        }
    }
    pthread_mutex_unlock(&lock);
}

/* Send message to sender */
void send_message_self(const char *s, int tcp_client_socket){
    if (write(tcp_client_socket, s, strlen(s)) < 0) {
        perror("Write to descriptor failed");
        exit(-1);
    }
}

/* Send message to client */
void send_message_client(char *s, int id){
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENT; ++i){
        if (clients[i]) { 
            if (clients[i]->id == id) {
                if (write(clients[i]->tcp_client_socket, s, strlen(s))<0) {
                    perror("Write to descriptor failed");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&lock);
}

/* MAY BE REMOVABLE Strip CRLF */
void strip_newline(char *s){
    while (*s != '\0') {
        if (*s == '\r' || *s == '\n') {
            *s = '\0';
        }
        s++;
    }
}

/* Handle all communication with the client */
void *clientManager(void *arg){
    char buff_out[BUFFER_SIZE];
    char buff_in[BUFFER_SIZE / 2];
    int rlen;

    numClients++;
    Client *client = (Client *)arg;

    printf("<< Welcome client #%d\n ", client->id);
    printf(" referenced by %d\n", client->id);

    //Receiving the initial username
    read(client->tcp_client_socket, buff_in, sizeof(buff_in) -1);
    strcpy(client->name, buff_in);

    sprintf(buff_out, "<< %s has joined\n", client->name);
    send_message_all(buff_out);
    
    send_message_self("<< see /help for assistance\n", client->tcp_client_socket);

    /* Receive input from client */
    while ((rlen = read(client->tcp_client_socket, buff_in, sizeof(buff_in) - 1)) > 0) {
        buff_in[rlen] = '\0';
        buff_out[0] = '\0';
        strip_newline(buff_in);

        /* Ignore empty buffer */
        if (!strlen(buff_in)) {
            printf("Found empty buffer line 193\n");
            continue;
        }

        printf("Client: %s\n", buff_in);
        /* Special options */
        if(buff_in[0] == '/') {
            char *command, *param;
            command = strtok(buff_in," ");
            if(strcmp(command, "/quit") == 0) {
                send_message_self("Goodbye\n", client->tcp_client_socket);
                break;
            } 
            else if(strcmp(command, "/nick") == 0) {
                param = strtok(NULL, " ");
                
                if(param) {
                    char *old_name = (char*)malloc(sizeof(client->name) + 1);// = strdup(client->name);
                    strcpy(old_name, client->name);
                    
                    if(!old_name) {
                        perror("Cannot allocate memory");
                        continue;
                    }
                    
                    strcpy(client->name, param);
                    sprintf(buff_out, "<< %s is now known as %s\n", old_name, client->name);
                    free(old_name);
                    send_message_all(buff_out);
                } 
                else {
                    send_message_self("<< name cannot be null\n", client->tcp_client_socket);
                }
            } 
            else if(strcmp(command, "/msg") == 0) {
                param = strtok(NULL, " ");
                
                if(param) {
                    int id = atoi(param);
                    param = strtok(NULL, " ");
                    if(param) {
                        sprintf(buff_out, "[PM][%s]", client->name);
                        while (param != NULL) {
                            strcat(buff_out, " ");
                            strcat(buff_out, param);
                            param = strtok(NULL, " ");
                        }
                        strcat(buff_out, "\r\n");
                        send_message_client(buff_out, id);
                    }
                    else {
                        send_message_self("<< message cannot be null\n", client->tcp_client_socket);
                    }
                } 
                else {
                    send_message_self("<< reference cannot be null\n", client->tcp_client_socket);
                }
            } 
            else if(strcmp(command, "/list") == 0) {
                sprintf(buff_out, "There are %d client(s) in the chatroom.\n", numClients);
                send_message_self(buff_out, client->tcp_client_socket);
            } 
            else if (strcmp(command, "/help") == 0) {
                strcat(buff_out, "<< /quit     Quit chatroom\n");
                strcat(buff_out, "<< /nick     <name> Change nickname\n");
                strcat(buff_out, "<< /msg      <reference> <message> Send private message\n");
                strcat(buff_out, "<< /list     Show active clients\n");
                strcat(buff_out, "<< /help     Show help\n");
                send_message_self(buff_out, client->tcp_client_socket);
            } 
            else {
                send_message_self("<< unknown command\n", client->tcp_client_socket);
            }
        } else {
            /* Send message */
            snprintf(buff_out, sizeof(buff_out), "[%s] %s\n", client->name, buff_in);
            send_message(buff_out, client->id);
        }
    }

    /* Close connection */
    sprintf(buff_out, "<< %s has left\r\n", client->name);
    send_message_all(buff_out);
    close(client->tcp_client_socket);
   
    /* Delete client from queue and yield thread */
    //queue_delete(client->id);
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENT; ++i) {
        if (clients[i]) {
            if (clients[i]->id == id) {
                clients[i] = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    
    printf("<< quit ");
    free(client);
    numClients--;
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char *argv[]) {

    // Create the server socket using the socket function
    int tcp_server_socket; 
    int tcp_client_socket;
    tcp_server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Define the server address
    struct sockaddr_in tcp_server_address;
    struct sockaddr_in tcp_client_address;
    tcp_server_address.sin_family = AF_INET;
    
    // Passing the port number, converting in right network byte order
    tcp_server_address.sin_port = htons(PORT_NUMBER);

    // Connecting to 0.0.0.0
    tcp_server_address.sin_addr.s_addr = htonl(INADDR_ANY); 

    // Bind the socket to the IP address & port
    bind(tcp_server_socket, (struct sockaddr*)&tcp_server_address, sizeof(tcp_server_address));

    // Listen for simultaneous connections
    listen(tcp_server_socket, MAX_CLIENT);

    // ****TEST AND REMOVEIgnore pipe signals
    //signal(SIGPIPE, SIG_IGN);

    pthread_t tid;
    if(pthread_mutex_init(&lock, NULL) != 0) {
        printf("ERROR: Failed to initialize lock\n");
        exit(0);
    }

    printf("-=[ SERVER STARTED ]=-\n");

    while (1) {
        socklen_t clilen = sizeof(tcp_client_address);
        //tcp_client_socket = accept(tcp_server_socket, (struct sockaddr*)&tcp_client_address, &clilen);
        tcp_client_socket = accept(tcp_server_socket, &tcp_client_address, &clilen);
        /* Check if max clients is reached */
        if ((numClients + 1) == MAX_CLIENT) {
            printf("<< max clients reached\n");
            printf("<< reject ");
            printf("\n");
            close(tcp_client_socket);
            continue;
        }

        /* Client settings */
        Client *client = (Client *)malloc(sizeof(Client));
        client->tcp_client_address = tcp_client_address;
        client->tcp_client_socket = tcp_client_socket;
        client->id = id++;
        sprintf(client->name, "%d", client->id);

        /* Add client to the queue and fork thread */
        //queue_add(cli);
        pthread_mutex_lock(&lock);
        for (int i = 0; i < MAX_CLIENT; ++i) {
            if (!clients[i]) {
                clients[i] = client;
                i = MAX_CLIENT;
            }
        }
        pthread_mutex_unlock(&lock);
        
        pthread_create(&tid, NULL, &clientManager, (void*)client);

        sleep(1);
    }
    pthread_mutex_destroy(&lock);
    return 0;
}
