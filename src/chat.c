#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
typedef struct pthread_arg_t
{
    char client_message[256];
    char server_message[256];
    int client_socket;
    struct sockaddr_in client_address;

} connec;

void *handle_connection(void *connect)
{
    connec *connection = (connec *)connect;
    char server_message[256];
    int socket = connection->client_socket;
    struct sockaddr_in client_address = connection->client_address;

    while (1)
    {
        if (recv(socket, server_message, 256, 0) > 0)
        {
            printf("got message from ('%s', %hu)\n", inet_ntoa(client_address.sin_addr), client_address.sin_port);
            if (strcmp(server_message, "hello") == 0)
            {
                strcpy(server_message, "world");
                if (send(socket, server_message, sizeof(server_message), 0) == -1)
                {
                    perror("can't send");
                }
            }
            else if (strcmp(server_message, "goodbye") == 0)
            {
                strcpy(server_message, "farewell");
                if (send(socket, server_message, sizeof(server_message), 0) == -1)
                {
                    perror("can't send");
                }
                // close(client_socket);
                break;
            }
            else if (strcmp(server_message, "exit") == 0)
            {
                strcpy(server_message, "ok");
                if (send(socket, server_message, sizeof(server_message), 0) == -1)
                {
                    perror("can't send");
                }
                exit(0);
                //break;
            }
            else
            {
                //  strcpy(server_message,server_message );
                if (send(socket, server_message, sizeof(server_message), 0) == -1)
                {
                    perror("can't send");
                }
            }
            bzero(server_message, sizeof(server_message));
        }
    }

    pthread_exit("exit");
}

/* This function is for tcp_server */
void tcp_server(char *iface, long port)
{
    //create a socket
    int server_socket;
    connec *con;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1)
    {
        perror("socket");
        exit(1);
    }

    //address of the socket
    struct sockaddr_in server_address;
    // client_address;
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
    // listen to socket
    if (listen(server_socket, 10) == 0)
    {
    }

    int client_socket;
    int counter = -1;
    pthread_t thread;
    socklen_t caddr;
    // Array for thread
    //  pthread_t tid[10];
    while (1)
    {

        con = (connec *)malloc(sizeof *con);
        caddr = sizeof con->client_address;
        client_socket = accept(server_socket, (struct sockaddr *)&con->client_address, &caddr);
        counter++;

        if (client_socket == -1)
        {
            perror("can't accept\n");
            continue;
        }

        con->client_socket = client_socket;
        printf("connection %d from ('%s', %hu)\n", counter, inet_ntoa(con->client_address.sin_addr), con->client_address.sin_port);

        pthread_create(&thread, NULL, &handle_connection, (void *)con);
    }
    close(client_socket);
}

/** This function configures UDP server */
void udp_server(char *iface, long port)
{

    int network_socket, length, n;
    socklen_t fromlen;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_address;
    char server_message[256], client_message[256];

    network_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (network_socket < 0)
    {
        perror("Opening socket");
    }

    length = sizeof(server_addr);
    bzero(&server_addr, length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port);

    // bind the server with address and socket
    if (bind(network_socket, (struct sockaddr *)&server_addr, length) < 0)
    {
        perror("binding");
    }

    fromlen = sizeof(struct sockaddr_in);
    while (1)
    {
        if (recvfrom(network_socket, client_message, sizeof(client_message), 0, (struct sockaddr *)&client_address, &fromlen) < 0)
        {
            perror("recvfrom");
        }
        else
        {

            printf("got message from ('%s', %hu)\n", inet_ntoa(client_address.sin_addr), client_address.sin_port);
            if (strcmp(client_message, "hello") == 0)
            {
                strcpy(server_message, "world");
            }
            else if (strcmp(client_message, "goodbye") == 0)
            {
                strcpy(server_message, "farewell");
            }
            else if (strcmp(client_message, "exit") == 0)
            {
                strcpy(server_message, "ok");
            }
            else
            {
                strcpy(server_message, client_message);
            }
        }

        n = sendto(network_socket, server_message, sizeof(server_message), 0, (struct sockaddr *)&client_address, fromlen);
        if (n < 0)
        {
            perror("sendto");
        }
        if (strcmp(server_message, "ok") == 0)
        {
            break;
        }
    }
}

void chat_server(char *iface, long port, int use_udp)
{
    if (use_udp == 1)
    {
        udp_server(iface, port);
    }
    else
    {
        tcp_server(iface, port);
    }
}

/** This function configures TCP client */
void tcp_client(char *host, long port)
{
    //create a socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (network_socket == -1)
    {
        perror("socket");
        exit(1);
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

    //receive data from server
    char server_response[256], client_message[256];

    while (1)
    {
        if (scanf("%s", &client_message[0]))
        {
        }

        // Send the message to server:
        if (send(network_socket, client_message, sizeof(client_message), 0) < 0)
        {
            printf("Unable to send message\n");
        }

        // Receive the server's response:
        if (recv(network_socket, server_response, sizeof(server_response), 0) < 0)
        {
            printf("Error while receiving server's msg\n");
        }
        else
        {
            printf("%s\n", server_response);

            if (strcmp(client_message, "goodbye") == 0 && strcmp(server_response, "farewell") == 0)
            {
                break;
            }
            else if (strcmp(client_message, "exit") == 0 && strcmp(server_response, "ok") == 0)
            {
                break;
            }
        }
    }
    close(network_socket);
}

/** This function configures UDP client */
void udp_client(char *host, long port)
{
    int network_socket, n;
    int length;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    char server_message[256], client_message[256];
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

    bzero(client_message, length);

    while (1)
    {
        if (scanf("%s", &client_message[0]))
        {
        }
        n = sendto(network_socket, client_message, sizeof(client_message), 0, (struct sockaddr *)&server_address, length);
        if (n < 0)
        {
            perror("send error");
        }
        if (recvfrom(network_socket, server_message, sizeof(server_message), 0, (struct sockaddr *)&client_address, (socklen_t *)&length) < 0)
        {
            perror("error in receive server_message");
        }
        else
        {
            printf("%s\n", server_message);
            if (strcmp(client_message, "goodbye") == 0 && strcmp(server_message, "farewell") == 0)
            {
                break;
            }

            if (strcmp(client_message, "exit") == 0 && strcmp(server_message, "ok") == 0)
            {
                break;
            }
        }
    }
}

void chat_client(char *host, long port, int use_udp)
{

    if (use_udp == 1)
    {
        udp_client(host, port);
    }
    else
    {
        tcp_client(host, port);
    }
}
