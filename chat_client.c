/** 
 * client.c: This program runs a client which connects to a running server.
 * It creates the socket, connects to the server, receives data, displays
 * the data, and then closes the connection.
 *
 * Author(s): Mason Riley, Cesar Santiago
 * Course: COP4635
 * Project #: 1
 * Last Updated: 2/16/2020
 */

#include <stdio.h>          //Standard library
#include <stdlib.h>         //Standard library
#include <sys/socket.h>     //API and definitions for the sockets
#include <sys/types.h>      //more definitions
#include <netinet/in.h>     //Structures to store address information
#include <unistd.h>         //Defines misc. symbolic constants and types
#include <string.h>         //String methods
#include <pthread.h>
#include <signal.h>

#include "standards.h"      //Contains constants used in both server.c and client.c

#define USERNAME_SIZE 32

int tcp_client_socket; // Socket descriptor      
int end = 0;

pthread_mutex_t lock;

void *handleResponse() {
    char tcp_server_response[DATA_SIZE];

    while(end == 0) {
	pthread_mutex_lock(&lock);
	recv(tcp_client_socket, &tcp_server_response, sizeof(tcp_server_response), 0); 
	pthread_mutex_unlock(&lock);
	printf("\nServer: %s\n", tcp_server_response);
	
	if(strncmp(tcp_server_response, "Goodbye", 7) == 0)
	    end = -1; 
	memset(tcp_server_response, 0, sizeof(tcp_server_response));

    }
    return NULL;
}

void mainMenu(){
	printf("-=| MAIN MENU |=-\n");
	printf("1. View current online number.\n");
	printf("2. Enter the group chat.\n");
	printf("3. Enter the private chat.\n");
	printf("4. View chat history.\n");
	printf("5. File transfer.\n");
	printf("6. Change the password.\n");
	printf("7. Logout.\n");
	printf("8. Administrator.\n");
	printf("0. Return to the login screen.\n\n");
	printf("Enter an action: ");
}

void loginMenu(){
	printf("-=| ONLINE CHAT ROOM |=-\n");
	printf("1. Register.\n");
	printf("2. Login.\n");
	printf("0. Quit.\n\n");
	printf("Enter an action: ");
}

int authenticate(FILE* fp, char* username, char* password) {
	char line[4086];
	char* fileUsername;//[USERNAME_SIZE];
	char* filePassword;//[USERNAME_SIZE];

	fileUsername = (char*)malloc(USERNAME_SIZE * sizeof(char));
	filePassword = (char*)malloc(USERNAME_SIZE * sizeof(char));

	while(fgets(line, 4086, fp) != NULL){
		fileUsername = strtok(line, ",");
		if(strcmp(username, fileUsername) == 0){
            filePassword = strtok(NULL, "\n");
			if(strcmp(password, filePassword) == 0){
				return 1;
			}
		}
	}
	return 0;
}

int main(int argc, char** argv){    
	char* loginFile;
	FILE* lfp;
	char *username;//[USERNAME_SIZE];
	char *password;//[USERNAME_SIZE];
    username = (char*)malloc(USERNAME_SIZE);
    password = (char*)malloc(USERNAME_SIZE);
	pthread_t tid;

	if(argc > 1)
		loginFile = argv[1];
	else
		loginFile = "loginFile.csv";

	if(pthread_mutex_init(&lock, NULL) != 0) {
		printf("ERROR: Failed to initialize lock\n");
		exit(0);
	}
	int authenticated = 0;
	do{
		char choice[2];
		loginMenu();
		fgets(choice, 2, stdin);
		if(strncmp(choice, "1", 1) == 0){
			lfp = fopen(loginFile, "a");
			memset(username, 0, USERNAME_SIZE);
			memset(password, 0, USERNAME_SIZE);
			printf("Registering\nEnter a username: ");
			scanf(" %s", username);
			printf("Enter a password: ");
			scanf(" %s", password);
			fprintf(lfp, "%s,%s\n", username, password);
			fclose(lfp);
			authenticated = 1;
		}
		else if(strncmp(choice, "2", 1) == 0){
			memset(username, 0, USERNAME_SIZE);
			memset(password, 0, USERNAME_SIZE);
			printf("Logging in\nEnter your username: ");
			scanf(" %s", username);
			printf("Enter your password: ");
			scanf(" %s", password);
			lfp = fopen(loginFile, "r");
			authenticate(lfp, username, password);
			fclose(lfp);
		}
		else if(strncmp(choice, "0", 1) == 0){
			printf("EXITING.");
			return 0;
		}
	}while(username == NULL);

	// Creating the TCP socket
	tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0);

	// Specify address and port of the remote socket
	struct sockaddr_in tcp_server_address; // Declaring a structure for the address           

	// Structure Fields' definition: Sets the address family 
	// of the address the client would connect to    
	tcp_server_address.sin_family = AF_INET;

	// Specify and pass the port number to connect - converting in right network byte order    
	tcp_server_address.sin_port = htons(PORT_NUMBER);   

	// Connection uses localhost
	tcp_server_address.sin_addr.s_addr = INADDR_ANY;      

	// Connecting to the remote socket
	int connection_status = connect(tcp_client_socket, 
	    (struct sockaddr *) &tcp_server_address, sizeof(tcp_server_address));        

	char req[BUFFER_SIZE];

	if (connection_status == -1) {
		end = -1;        
		printf("ERROR: Failed to connect to server.\n");     
	}
	else { 
	printf("Successfully connected to server.\n");

	//Print first message from server and then reset it
	char tcp_server_response[DATA_SIZE];    
	recv(tcp_client_socket, &tcp_server_response, sizeof(tcp_server_response), 0); 
	printf("\n\nServer: %s \n", tcp_server_response);   

	//Create thread that handles server responses
	pthread_create(&tid, NULL, &handleResponse, NULL);
	}

	while(end == 0) { 
		memset(req, 0, BUFFER_SIZE);
		printf("$ ");
		fgets(req, BUFFER_SIZE, stdin);
		send(tcp_client_socket, req, strlen(req), 0);
	}

	// Close socket and thread
	pthread_join(tid, NULL);
	pthread_mutex_destroy(&lock); 
	close(tcp_client_socket);    
	return 0;
}
