// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <time.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <sys/socket.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #define SIZE 256
// /*
//  *  Here is the starting point for your netster part.3 definitions. Add the
//  *  appropriate comment header as defined in the code formatting guidelines
//  */

// /* Add function definitions */

// typedef struct packet
// {
//     char data[1024];
// } Packet;

// typedef struct frame
// {
//     int frame_kind; //ACK:0, SEQ:1 FIN:2
//     int sq_no;
//     int ack;
//     Packet packet;
// } Frame;

// void stopandwait_server(char *iface, long port, FILE *fp)
// {
//     int network_socket, length;
//     socklen_t fromlen;
//     struct sockaddr_in server_addr;
//     struct sockaddr_in client_address;
//     int frame_id = 0;
//     Frame frame_recv;
//     Frame frame_send;

//     network_socket = socket(AF_INET, SOCK_DGRAM, 0);
//     if (network_socket < 0)
//     {
//         perror("Opening socket");
//     }

//     length = sizeof(server_addr);
//     bzero(&server_addr, length);
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
//     server_addr.sin_port = htons(port);

//     // bind the server with address and socket
//     if (bind(network_socket, (struct sockaddr *)&server_addr, length) < 0)
//     {
//         perror("binding");
//     }

//     // Receive all messages from client
//     fromlen = sizeof(struct sockaddr_in);
//     int f_recv_size;
//     char buffer[SIZE];

//     while ((f_recv_size = recvfrom(network_socket, &frame_recv, sizeof(Frame), 0, (struct sockaddr *)&client_address, &fromlen)) > 0)
//     {
//         if (f_recv_size > 0 && frame_recv.frame_kind == 1 && frame_recv.sq_no == frame_id)
//         {
//             printf("[+]Frame Received: %s\n", frame_recv.packet.data);
//             frame_send.sq_no = 0;
//             frame_send.frame_kind = 0;
//             frame_send.ack = frame_recv.sq_no + 1;
//             sendto(network_socket, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&client_address, fromlen);
//             printf("[+]Ack Send\n");
//         }
//         else
//         {
//             printf("[+]Frame Not Received\n");
//         }
//         frame_id++;
//         // stop reading once we receive 'STOP' as message
//         if (strcmp(buffer, "STOP") == 0)
//         {
//             break;
//         }
//         if (fwrite(buffer, f_recv_size, 1, fp) != f_recv_size)
//         {
//             //exit(1);
//         }
//         bzero(buffer, SIZE);
//     }

//     // close socket after receiving all message
//     close(network_socket);
// }

// void stopandwait_client(char *host, long port, FILE *fp)
// {
//     int network_socket;
//     int length;
//     struct sockaddr_in server_address;
//     int frame_id = 0;
//     Frame frame_send;
//     Frame frame_recv;
//     int ack_recv = 1;
//     // socket
//     network_socket = socket(AF_INET, SOCK_DGRAM, 0);
//     if (network_socket < 0)
//     {
//         perror("Opening socket");
//     }
//     length = sizeof(struct sockaddr_in);
//     bzero(&server_address, length);
//     server_address.sin_family = AF_INET;
//     server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
//     server_address.sin_port = htons(port);

//     char buffer[SIZE];
//     int read;

//     while ((read = fread(&buffer, 1, SIZE, fp)) > 0)
//     {
//         if (ack_recv == 1)
//         {
//             frame_send.sq_no = frame_id;
//             frame_send.frame_kind = 1;
//             frame_send.ack = 0;
//             strcpy(frame_send.packet.data, buffer);
//             sendto(network_socket, &frame_send, sizeof(Frame), 0, (struct sockaddr *)&server_address, length);
//             printf("[+]Frame Send\n");
//         }
//         socklen_t addr_size = sizeof(server_address);
//         int f_recv_size = recvfrom(network_socket, &frame_recv, sizeof(frame_recv), 0, (struct sockaddr *)&server_address, &addr_size);

//         if (f_recv_size > 0 && frame_recv.sq_no == 0 && frame_recv.ack == frame_id + 1)
//         {
//             printf("[+]Ack Received\n");
//             ack_recv = 1;
//         }
//         else
//         {
//             printf("[-]Ack Not Received\n");
//             ack_recv = 0;
//         }

//         frame_id++;
//         if (sendto(network_socket, &frame_send, sizeof(Frame), 0, (struct sockaddr *)&server_address, length) < -1)
//         {
//             perror("[-]Error in sending file.");
//         }

//         bzero(buffer, sizeof(buffer));
//     }

//     // send end
//     sendto(network_socket, "STOP", sizeof("STOP"), 0, (struct sockaddr *)&server_address, length);
//     fclose(fp);
//     close(network_socket);
// }

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#define MAX 256
#define MSGLEN 128
#include <sys/time.h>
#include <errno.h>
#include "stopandwait.h"
/*
 *  Here is the starting point for your netster part.3 definitions. Add the
 *  appropriate comment header as defined in the code formatting guidelines
 */
/* Add function definitions */
typedef struct packet
{
    char data[MSGLEN];
} Packet;
typedef struct frame
{
    int seq;       // 0 or 1
    int ack;       //0 or 1
    int sign_type; //ACK:0 SEQ:1 FIN:2
    ssize_t msg_len;
    Packet packet;
} Frame;
void stopandwait_server(char *iface, long port, FILE *fp)
{
    int sock_conn;
    struct sockaddr_in servaddr;
    sock_conn = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_conn == -1)
    {
        printf("[-] Socket Creation Failed !!\n");
        exit(1);
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if ((bind(sock_conn, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("[-]Socket Binding Failed\n");
        exit(1);
    }
    int nextseqnum = 0;
    Frame frame_recv;
    Frame frame_send;
    while (1)
    {
        struct sockaddr_in cli_address;
        socklen_t address_length = sizeof(cli_address);
        int bytes_read = 0;
        while ((bytes_read = recvfrom(sock_conn, &frame_recv, sizeof(Frame), 0, (struct sockaddr *)&cli_address, &address_length)) > 0)
        {
            if (frame_recv.sign_type == 2)
            {
                fclose(fp);
                close(sock_conn);
                exit(0);
            }
            if (frame_recv.sign_type == 1 && frame_recv.seq == nextseqnum)
            {
                fprintf(stderr, "%ld\n", sizeof(frame_recv.packet.data));

                fwrite(frame_recv.packet.data, sizeof(char), frame_recv.msg_len, fp);
                if (ferror(fp))
                {
                    fprintf(stderr, "Error writing ......\n");
                    close(sock_conn);
                    fclose(fp);
                    exit(1);
                }
                frame_send.seq = nextseqnum;
                frame_send.sign_type = 0;
                frame_send.ack = frame_recv.seq;
                sendto(sock_conn, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&cli_address, address_length);
                nextseqnum = 1 - nextseqnum;
            }
            else
            {
                frame_send.ack = 1 - nextseqnum;
                sendto(sock_conn, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&cli_address, address_length);
            }
        }
        fclose(fp);
        close(sock_conn);
        exit(0);
    }
}
void stopandwait_client(char *host, long port, FILE *fp)
{
    int sock_conn;
    struct sockaddr_in servaddr;
    ssize_t bytes_written = 0;
    socklen_t address_length = sizeof(servaddr);
    char buffer[MAX];
    socklen_t addr_len = sizeof(servaddr);
    sock_conn = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_conn == -1)
    {
        printf("[-] Socket creation failed\n");
        exit(0);
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(host);
    servaddr.sin_port = htons(port);
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    Frame frame_recv;
    Frame frame_send;
    int seqnum = 0;
    do
    {
        size_t bytes_read = fread(frame_send.packet.data, sizeof(char), sizeof(frame_send.packet.data), fp);
        frame_send.msg_len = bytes_read;
        if (ferror(fp))
        {
            printf("Error reading from file.\n");
            close(sock_conn);
            fclose(fp);
            exit(1);
        }
        frame_send.seq = seqnum;
        frame_send.sign_type = 1;
        memcpy(frame_send.packet.data, buffer, 128);
        while (1)
        {
            sleep(2);
            sendto(sock_conn, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&servaddr, addr_len);
            bzero(buffer, sizeof(buffer));
            setsockopt(sock_conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            int n = recvfrom(sock_conn, &frame_recv, sizeof(Frame), 0, (struct sockaddr *)&servaddr, &address_length);
            fprintf(stderr, "This is seqnum : %d\n This is ACK : %d\n", frame_recv.seq, frame_recv.ack);
            if (n < 0)
            {
                fprintf(stderr, "Sending again\n");
                continue;
            }
            if (seqnum == frame_recv.ack)
            {
                break;
            }
        }
        seqnum = 1 - seqnum;
    } while (!feof(fp) && bytes_written != -1);
    frame_send.sign_type = 2;
    sendto(sock_conn, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&servaddr, addr_len);
    fclose(fp);
    close(sock_conn);
    exit(0);
}
