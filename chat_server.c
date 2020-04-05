/** 
 * chat_server.c: This program runs a server which connects to clients.
 * It creates the socket, accepts client connections, creates a thread, 
 * receives data, displays the data, and then closes the connection.
 *
 * Author(s): Mason Riley, Cesar Santiago
 * Course: COP4635
 * Project #: 2 
 * Last Updated: 4/5/2020
 */

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

typedef struct {
    struct sockaddr_in tcp_client_address;
    int tcp_client_socket; 
    int id;               
    char name[32];       
} Client;

int numClients = 0;
int id = 1;
Client *clients[MAX_CLIENT];
pthread_mutex_t lock;
FILE *gch;

void sendMessage(char *message, char *id, int option, int tcp_client_socket) {
    if(option == SELF) {
        write(tcp_client_socket, message, strlen(message));
    }
    else if(option == PRIVATE) {
        pthread_mutex_lock(&lock);
        for (int i = 0; i < MAX_CLIENT; ++i){ 
            if(clients[i] != NULL && strcmp(id, clients[i]->name) == 0) {
                // Add to chat history
                char *fileName = (char*)malloc(sizeof(clients[i]->name) + sizeof("Log.txt") + 1);
                strcpy(fileName, clients[i]->name);
                strcat(fileName, "Log.txt");
                FILE *pmHistory = fopen(fileName, "a");
                fprintf(pmHistory, "%s", message);
                fclose(pmHistory);    
                // Send message
                write(clients[i]->tcp_client_socket, message, strlen(message));
            }
        }
        pthread_mutex_unlock(&lock);
    }
    else if(option == GLOBAL) {
        pthread_mutex_lock(&lock);
        for (int i = 0; i < MAX_CLIENT; ++i){
            if(clients[i] != NULL) {
                gch = fopen("globalChatHistory.txt", "a");
                fprintf(gch, "%s", message);
                fclose(gch);
                write(clients[i]->tcp_client_socket, message, strlen(message));
            }
        }
        pthread_mutex_unlock(&lock);
    }
}

void stripNewline(char *s){
    while (*s != '\0') {
        if (*s == '\r' || *s == '\n') {
            *s = '\0';
        }
        s++;
    }
}
void handleGlobalMessaging(char *param, char* buffer, Client *client) {
    if(param) {
        sprintf(buffer, "[PM][%s]", client->name);
        while (param != NULL) {
            strcat(buffer, " ");
            strcat(buffer, param);
            param = strtok(NULL, " ");
        }
        strcat(buffer, "\n");
        fprintf(gch, "%s", buffer);
        sendMessage(buffer, NULL, GLOBAL, 0);
    } 
    else {
            strcat(buffer, "ERROR: invalid command\n");
            sendMessage(buffer, NULL, SELF, client->tcp_client_socket);
    }
}

void handlePrivateMessaging(char *param, char* buffer, Client *client) { 
    if(param) {
        char *id = (char*)malloc(sizeof(param) + 1);
        strcpy(id, param);
        param = strtok(NULL, " ");
        if(param) {
            sprintf(buffer, "[PM][%s]", client->name);
            while (param != NULL) {
                strcat(buffer, " ");
                strcat(buffer, param);
                param = strtok(NULL, " ");
            }
            strcat(buffer, "\n");
            sendMessage(buffer, id, PRIVATE, 0);
        }
        else {
            strcat(buffer, "ERROR: invalid command\n");
            sendMessage(buffer, NULL, SELF, client->tcp_client_socket);
        }
    } 
    else {
        strcat(buffer, "ERROR: invalid command\n");
        sendMessage(buffer, NULL, SELF, client->tcp_client_socket);
    }
}

void printMenu(Client *client) {
    char buff_out[BUFFER_SIZE];
    strcat(buff_out, "-=| MAIN MENU |=-\n");
    strcat(buff_out, "1. View current online number\n");
    strcat(buff_out, "2. Enter the group chat\n");
    strcat(buff_out, "3. Enter the private chat\n");
    strcat(buff_out, "4. View chat history.\n");
    strcat(buff_out, "5. File transfer.\n");
    strcat(buff_out, "6. Change the password.\n");
    strcat(buff_out, "7. Logout.\n");
    strcat(buff_out, "8. Administrator.\n");
    strcat(buff_out, "0. Return to the login screen.\n\n");
    strcat(buff_out, "Enter an action: ");

    sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
}

void historyManager(Client *client) {
    char buff_in[BUFFER_SIZE];
    char buff_out[BUFFER_SIZE];
    int rlen;

    strcat(buff_out, "1. Group Chat\n");
    strcat(buff_out, "2. Private Chat\n");
    strcat(buff_out, "0. Quit\n");
    sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);

    while ((rlen = read(client->tcp_client_socket, buff_in, sizeof(buff_in) - 1)) > 0) {
        buff_in[rlen] = '\0';
        buff_out[0] = '\0';
        stripNewline(buff_in);

        // Check for empty buffer
        if (!strlen(buff_in)) {
            continue;
        }

        char *command;
        command = strtok(buff_in," ");
        if(strcmp(command, "1") == 0) {
            FILE *gchIn = fopen("globalChatHistory.txt", "r");
            if(gchIn == NULL) {
                strcat(buff_out, "ERROR: History does not exists.\n");
                sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
            }
            else {        
                strcat(buff_out, "===========PM HISTORY===========\n");
                char c;
                c = fgetc(gchIn);
                while(c != EOF) {
                    strncat(buff_out, &c, 1);
                    c = fgetc(gchIn);
                }
                sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
                fclose(gchIn);
            }
        } 
        else if(strcmp(command, "2") == 0) {
            char *fileName = (char*)malloc(sizeof(client->name) + sizeof("Log.txt") + 1);
            strcpy(fileName, client->name);
            strcat(fileName, "Log.txt");
            FILE *pmhIn = fopen(fileName, "r");
            if(pmhIn == NULL) {
                strcat(buff_out, "ERROR: History does not exists.\n");
                sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
            }
            else {        
                strcat(buff_out, "===========GLOBAL HISTORY===========\n");
                char c;
                c = fgetc(pmhIn);
                while(c != EOF) {
                    strncat(buff_out, &c, 1);
                    c = fgetc(pmhIn);
                }
                sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
                fclose(pmhIn);
            }
        }
        else if(strcmp(command, "0") == 0) {
            printMenu(client);
            break;
        }
    }
}

void adminManager(Client *client) {
    char buff_in[BUFFER_SIZE];
    char buff_out[BUFFER_SIZE];
    int rlen;

    strcat(buff_out, "1. Ban a member\n");
    strcat(buff_out, "2. Dismiss a member\n");
    strcat(buff_out, "3. Kick a member\n");
    strcat(buff_out, "0. Quit\n");
    sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);

    while ((rlen = read(client->tcp_client_socket, buff_in, sizeof(buff_in) - 1)) > 0) {
        buff_in[rlen] = '\0';
        buff_out[0] = '\0';
        stripNewline(buff_in);

        // Check for empty buffer
        if (!strlen(buff_in)) {
            continue;
        }

        char *command;
        command = strtok(buff_in," ");
        if(strcmp(command, "1") == 0) {
            strcat(buff_out, "Enter the ID you want to ban:\n");
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
            
            read(client->tcp_client_socket, buff_in, sizeof(buff_in) - 1);
            FILE *blacklist = fopen("blacklist.txt", "a");
            fprintf(blacklist, "%s", buff_in);
            fclose(blacklist);
            
            for(int i = 0; i < numClients; ++i) {
                if(strcmp(buff_in, clients[i]->name) == 0) {
                    close(clients[i]->tcp_client_socket);
                }
            }

            buff_out[0] = '\0';
            strcat(buff_out, "1. Ban a member\n");
            strcat(buff_out, "2. Dismiss a member\n");
            strcat(buff_out, "3. Kick a member\n");
            strcat(buff_out, "0. Quit\n");
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
        }
        else if(strcmp(command, "2") == 0) { 
            strcat(buff_out, "Enter the ID you want to dismiss:\n");
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
            
            read(client->tcp_client_socket, buff_in, sizeof(buff_in) - 1);
            
            int i;
            for(i = 0; i < MAX_CLIENT; ++i) {
                if(clients[i] != NULL && strncmp(buff_in, clients[i]->name, strlen(clients[i]->name)) == 0) {
                    close(clients[i]->tcp_client_socket);
                }
            }

            buff_out[0] = '\0';
            strcat(buff_out, "1. Ban a member\n");
            strcat(buff_out, "2. Dismiss a member\n");
            strcat(buff_out, "3. Kick a member\n");
            strcat(buff_out, "0. Quit\n");
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
        }
        else if(strcmp(command, "3") == 0) { 
            strcat(buff_out, "Enter the ID you want to kick:\n");
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
            
            read(client->tcp_client_socket, buff_in, sizeof(buff_in) - 1);
            
            int i;
            for(i = 0; i < MAX_CLIENT; ++i) {
                if(clients[i] != NULL && strncmp(buff_in, clients[i]->name, strlen(clients[i]->name)) == 0) {
                    close(clients[i]->tcp_client_socket);
                }
            }

            buff_out[0] = '\0';
            strcat(buff_out, "1. Ban a member\n");
            strcat(buff_out, "2. Dismiss a member\n");
            strcat(buff_out, "3. Kick a member\n");
            strcat(buff_out, "0. Quit\n");
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
        }
        else if(strcmp(command, "0") == 0) {
            printMenu(client);
            break;
        }
    }
}

// Handle all communication with the client 
void *clientManager(void *arg){
    char buff_out[BUFFER_SIZE];
    char buff_in[BUFFER_SIZE / 2];
    int rlen;

    numClients++;
    Client *client = (Client *)arg;

    //Receiving the initial username
    read(client->tcp_client_socket, buff_in, sizeof(buff_in) -1);
    strcpy(client->name, buff_in);

    char badName[255];
    FILE *blacklist = fopen("blacklist.txt", "r");
    while(fgets(badName, 255, blacklist) != NULL) {
        if(strncmp(badName, client->name, strlen(client->name)) == 0) {
            close(client->tcp_client_socket);
            return NULL;
        } 
    } 

    sprintf(buff_out, "%s has joined\n", client->name);
    printf("Welcome %s!\n ", client->name);
    sendMessage(buff_out, NULL, GLOBAL, 0);
    
    printMenu(client);    

    // Receive input from client 
    while ((rlen = read(client->tcp_client_socket, buff_in, sizeof(buff_in) - 1)) > 0) {
        buff_in[rlen] = '\0';
        buff_out[0] = '\0';
        stripNewline(buff_in);

        // Check for empty buffer
        if (!strlen(buff_in)) {
            continue;
        }

        printf("%s: %s\n", client->name, buff_in);

        char *command, *param;
        command = strtok(buff_in," ");
        if(strcmp(command, "1") == 0) {
            sprintf(buff_out, "There are %d client(s) in the chatroom.\n", numClients);
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
        } 
        else if(strcmp(command, "2") == 0) {
            param = strtok(NULL, " ");
            handleGlobalMessaging(param, buff_out, client);
        } 
        else if(strcmp(command, "3") == 0) {
            param = strtok(NULL, " ");
            handlePrivateMessaging(param, buff_out, client);
        }
        else if(strcmp(command, "4") == 0) {
            historyManager(client);
        }
        else if(strcmp(command, "7") == 0) {
            strcat(buff_out, "Goodbye - pres any key to exit.\n");
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
            break;
        }
        else if(strcmp(command, "8") == 0) {
            adminManager(client);
        }
        else if(strcmp(command, "0") == 0) {
            strcat(buff_out, "Goodbye - pres any key to exit.\n");
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
            break;
        }
        else if(strcmp(command, "~") == 0) {
           printMenu(client);
        } 
        else {
            strcat(buff_out, "ERROR: invalid command\n");
            sendMessage(buff_out, NULL, SELF, client->tcp_client_socket);
        }
    }

    // Close connection
    sprintf(buff_out, "%s has left\r\n", client->name);
    sendMessage(buff_out, NULL, GLOBAL, 0);
    close(client->tcp_client_socket);
   
    // Delete client from queue
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


    pthread_t tid;
    if(pthread_mutex_init(&lock, NULL) != 0) {
        printf("ERROR: Failed to initialize lock\n");
        exit(0);
    }

    printf("-=[ SERVER STARTED ]=-\n");

    while (1) {
        socklen_t len = sizeof(tcp_client_address);
        tcp_client_socket = accept(tcp_server_socket, &tcp_client_address, &len);
        
        if (numClients == MAX_CLIENT) {
            printf("ERROR: Maximum clients reached\n");
            close(tcp_client_socket);
            continue;
        }

        // Setup client
        Client *client = (Client *)malloc(sizeof(Client));
        client->tcp_client_address = tcp_client_address;
        client->tcp_client_socket = tcp_client_socket;
        client->id = id++;

        // Add client to the queue and create thread
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

    // Destroy the lock when done
    pthread_mutex_destroy(&lock);
    return 0;
}
