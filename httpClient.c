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
#include "standards.h"      //Contains constants used in both server.c and client.c


int main(){    
    int end = 0;
    
    // Creating the TCP socket
    int tcp_client_socket; // Socket descriptor       
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

    while(end == 0) {
        
        // Send data to server
        char tcp_server_response[DATA_SIZE];    
        char req[20];
        char temp[BUFFER_SIZE];
        memset(temp, 0, BUFFER_SIZE);
        memset(req, 0, 20);


        // Return value of 0 means all okay, -1 means a problem        
        if (connection_status == -1){                                                  
            printf("ERROR: Failed to connect to server.\n");     
        }
        else {
            printf("Successfully connected to server.\n");
            printf("Message: ");
            scanf("%s", req);

            printf("%s", temp);

            send(tcp_client_socket, temp, strlen(temp), 0);

            // Receive data from server and print it
            recv(tcp_client_socket, &tcp_server_response, sizeof(tcp_server_response), 0); 
            printf("\n\nServer says: %s \n", tcp_server_response);    
        }
        
        memset(temp, 0, BUFFER_SIZE);
        memset(req, 0, 20);
        memset(tcp_server_response, 0, sizeof(tcp_server_response));
    }
   
    // Close socket 
    close(tcp_client_socket);    
    return 0;
}
