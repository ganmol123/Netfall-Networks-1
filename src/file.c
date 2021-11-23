#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define BUF_SIZE 256

/*
 *  Here is the starting point for your netster part.2 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */

void write_file(int network_socket, FILE *fp)
{
    int receive_size;
    char buffer[BUF_SIZE];
    while ((receive_size = recv(network_socket, buffer, BUF_SIZE, 0)) > 0)
    {
        //printf("%s\n",buffer);
        if (fwrite(buffer, 1, receive_size, fp) != receive_size)
        {
            //exit(1);
        }
        bzero(buffer, BUF_SIZE);
    }
    // once all data are read close the socket
    close(network_socket);
}

/* This function is for tcp_server */
void tcp_file_server(char *iface, long port, FILE *fp)
{

    //create a socket
    int server_socket, client_size;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket");
        // exit(1);
    }

    //address of the socket
    struct sockaddr_in server_address, client_address;
    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(&(server_address.sin_zero), '\0', 8);

    // Bind to the set port and IP:
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("Couldn't bind to the port\n");
    }

    // Listen for clients:
    listen(server_socket, 5);

    // accepting client socket
    int client_socket;
    client_size = sizeof(client_address);
    client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t *)&client_size);

    if (client_socket < -1)
    {
        perror("can't accept\n");
    }

    // reading data coming from client and writing it into a file
    write_file(client_socket, fp);
}

void udp_file_server(char *host, long port, FILE *fp)
{

    int network_socket, length;
    socklen_t fromlen;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_address;

    network_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (network_socket < 0)
    {
        perror("Opening socket");
    }

    length = sizeof(server_addr);
    bzero(&server_addr, length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ;
    server_addr.sin_port = htons(port);

    // bind the server with address and socket
    if (bind(network_socket, (struct sockaddr *)&server_addr, length) < 0)
    {
        perror("binding");
    }

    // Receive all messages from client
    fromlen = sizeof(struct sockaddr_in);
    int receive_size;
    char buffer[BUF_SIZE];
    while ((receive_size = recvfrom(network_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_address, &fromlen)) > 0)
    {
        //printf("inside recfrom %d",receive_size);

        // stop reading once we receive 'STOP' as message
        if (strcmp(buffer, "STOP") == 0)
        {
            break;
        }
        if (fwrite(buffer, receive_size, 1, fp) != receive_size)
        {
            //exit(1);
        }
        bzero(buffer, BUF_SIZE);
    }

    // close socket after receiving all message
    close(network_socket);
}

void file_server(char *iface, long port, int use_udp, FILE *fp)
{
    printf("use_udp %d ", use_udp);
    if (use_udp == 1)
    {
        printf("udp");
        udp_file_server(iface, port, fp);
    }
    else
    {
        tcp_file_server(iface, port, fp);
    }
}

void send_file(FILE *fp, int network_socket)
{
    //   int file_size;
    //   fseek(fp, 0L, SEEK_END);
    //   file_size = ftell(fp);
    //   printf("%d",file_size);
    //   rewind(fp);

    char buffer[BUF_SIZE] = {0};
    int size = fread(&buffer, 1, sizeof(buffer), fp);
    printf(" %d", size);
    if (send(network_socket, buffer, strlen(buffer), 0) == -1)
    {
        perror("[-]Error in sending file.");
        //exit(1);
    }
    bzero(buffer, BUF_SIZE);
}

void tcp_file_client(char *host, long port, FILE *fp)
{
    //create a socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (network_socket == -1)
    {
        perror("socket");
        // exit(1);
    }

    //address of the socket
    struct sockaddr_in server_address;
    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Socket Connection
    int connection_status = connect(network_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    if (connection_status < 0)
    {
        perror("There is an error in connection \n\n");
    }

    // read data from file and send to server in chunks
    char buffer[BUF_SIZE];
    int read;
    while ((read = fread(&buffer, 1, BUF_SIZE, fp)) > 0)
    {
        if (send(network_socket, buffer, read, 0) == -1)
        {
            perror("[-]Error in sending file.");
            //exit(1);
        }
        bzero(buffer, sizeof(buffer));
    }

    // close socket once all data are sent
    close(network_socket);
}

void udp_file_client(char *host, long port, FILE *fp)
{

    int network_socket;
    int length;
    struct sockaddr_in server_address;
    // socket
    network_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (network_socket < 0)
    {
        perror("Opening socket");
    }

    length = sizeof(struct sockaddr_in);
    bzero(&server_address, length);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    ;
    server_address.sin_port = htons(port);

    // read data from file and send to server in chunks
    char buffer[BUF_SIZE];
    int read;
    while ((read = fread(&buffer, 1, BUF_SIZE, fp)) > 0)
    {
        // printf(" udp client read %d %s", read, buffer);
        if (sendto(network_socket, buffer, read, 0, (struct sockaddr *)&server_address, length) < -1)
        {
            perror("[-]Error in sending file.");
            //exit(1);
        }
        bzero(buffer, sizeof(buffer));
    }

    // send end
    sendto(network_socket, "STOP", sizeof("STOP"), 0, (struct sockaddr *)&server_address, length);

    fclose(fp);
    close(network_socket);
}

void file_client(char *host, long port, int use_udp, FILE *fp)
{

    if (use_udp == 1)
    {
        udp_file_client(host, port, fp);
    }
    else
    {
        tcp_file_client(host, port, fp);
    }
}
