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
    int seq;       // N size
    int ack;       // N size
    int sign_type; // ACK:0 SEQ:1 FIN:2
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

    int base = 0;
    int current = 0;
    int N = 5; // Window size
    struct packet history[N];
    int last_ack = -1; 

    do { 

    // Add this shit into a while true loop
    
    while (current < N) // While to read and send N stuff, basically read and store N stuff in a buffer and interate over it to send it
    {

        // Reads the file n stuff
        size_t bytes_read = fread(frame_send.packet.data, sizeof(char), sizeof(frame_send.packet.data), fp);
        frame_send.msg_len = bytes_read;
        if (ferror(fp))
        {
            printf("Error reading from file.\n");
            close(sock_conn);
            fclose(fp);
            exit(1);
        }

        history[current] -> data = frame_send.packet -> data; // Basically copies the data in an array before sending it. Check syntax.
        
        frame_send.seq = seqnum;
        frame_send.sign_type = 1;
        memcpy(frame_send.packet.data, buffer, 128); // Copies stuff memcpy(dest, src, size)

        // Basically send the data
        if (seqnum < base + N) {
            sendto(sock_conn, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&servaddr, addr_len);
            if (seqnum == base) {
                // Start timer here
                setsockopt(sock_conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            }
            seqnum = 1 + seqnum;
        } else {
            break;
        }

        bzero(buffer, sizeof(buffer));
        int n = recvfrom(sock_conn, &frame_recv, sizeof(Frame), 0, (struct sockaddr *)&servaddr, &address_length);

        // Received last ack of the window
        if (frame_recv.ack == N) {
            base = base + frame_recv.ack;
            memset(history, 0, sizeof(history)); // Clearing history array for next round
            if (base == seqnum - 1) {
                // stop timer
                break;
            } 
        } else if (frame_recv.ack == frame_recv.seq) {
            last_ack = frame_recv.ack;
        }

        if (timeout) {
            base = base + last_ack;
            // repeat from history array
        }
    }


        while (1)
        {
            sleep(2);

            sendto(sock_conn, &frame_send, sizeof(frame_send), 0, (struct sockaddr *)&servaddr, addr_len);
            bzero(buffer, sizeof(buffer)); // Basically resets the memory for buffer size
            setsockopt(sock_conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            int n = recvfrom(sock_conn, &frame_recv, sizeof(Frame), 0, (struct sockaddr *)&servaddr, &address_length); // Returns size of the message
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
