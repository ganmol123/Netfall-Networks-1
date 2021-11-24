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
}

void stopandwait_client(char *host, long port, FILE *fp)
{
    int sock_conn;
    int flag = 1;
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
}

// Function called after connection is established, transfers specified files from server to client until server gets an exit message. Uses Go-Back-N Pipeline protocol.
void gbnFileTransfer(int sockfd, struct sockaddr_in clientAddress, FILE *fp)
{
    // Init Variables
    char messageBuffer[MAX]; // buffer used to transmit messages between client and server
    char fileName[MAX];      // Used to store a local copy of the file name
    int length;              // Length of UDP message
    int n;                   // Used for index identification
    clock_t begin, end;      // Used to measure transfer time.
    double runTime;          // Used to determine transfer time.

    // Infinite loop for communication between client and
    while (1)
    {

        // Init Variables Server Side
        int transferFlag = 1; // Flag used to break out of while loop
        int packetSize;       // Size of each packet being sent
        int lastAck = -1;     // Int of last received acknowledge. Set to -1 for no acks.
        int lastSegment;      // Int of last sent segnment number.

        begin = clock(); // Start File Transfer Clock

        // Reset messageBuffer
        bzero(messageBuffer, sizeof(messageBuffer));
        // printf("SERVER: Sending file to Cl \n");

        // Continuously pipeline file contents following Go-Back-N protocol until EOF
        while (transferFlag == 1)
        {

            // Send up to N Unacked Packets
            for (int i = 0; i < windowSize; i++)
            {

                // Send Current Segment's Number
                bzero(messageBuffer, sizeof(messageBuffer));
                sprintf(messageBuffer, "%d", i + j); // i + j = current possition
                sendto(sockfd, messageBuffer, sizeof(messageBuffer), 0, (const struct sockaddr *)&clientAddress, sizeof(clientAddress));

                // Update Last Segement Number
                lastSegment = atoi(messageBuffer);
                printf("SERVER: Sending segment %s \n", messageBuffer);

                // Set file read head to the bit location of the last ACK'd packet. (Maintain window)
                fseek(fp, (sizeof(char) * (MAX - 8) * (i + j)), SEEK_SET); // Offset max by 8 bits for size of checksum and '\0'

                // Reset Message Buffer and read in packet from server file
                bzero(messageBuffer, MAX);
                packetSize = fread(messageBuffer, sizeof(char), sizeof(messageBuffer), fp);

                // Send File
                if (sendto(sockfd, messageBuffer, packetSize, 0, (struct sockaddr *)&clientAddress, length) < 0)
                {
                    printf("SERVER: ERROR Failed to send file %s. Closing connection. \n", fileName);
                    exit(0);
                }

                // Receive latest acknowledge from Client.
                bzero(messageBuffer, sizeof(messageBuffer));
                n = recvfrom(sockfd, messageBuffer, sizeof(messageBuffer), 0, (struct sockaddr *)&clientAddress, &length);
                messageBuffer[n] = '\0';

                // Update Last ACK value if client does not report bit errors. If not, ignore ACK.
                if (lastAck == atoi(messageBuffer) - 1)
                {
                    lastAck = atoi(messageBuffer);
                }

                printf("SERVER: Receiving acknowledge %d \n", lastAck);

                // Testing to make sure the num of unacked packets never exceeds windowsize
                printf("SERVER: Unacked packets in flight %d \n", (i + j) - lastAck);

                // Break out of while loop if the final packet has been received and acknowledged. (Stop sending packets)
                if (packetSize == 0 || packetSize < MAX)
                {
                    if (lastAck == lastSegment)
                    {
                        transferFlag = 0;
                        break;
                    }
                }
            }

            // Slide Window to the past the last known ACK number, aka only trasnmit segments that haven't been ack'd last.
            j = lastAck + 1;
        }

        // End timer and store value in runTime.
        end = clock();
        runTime = (double)(end - begin) / CLOCKS_PER_SEC;

        // Display success message
        printf("SERVER: File successfully sent to client in %f seconds! \n", runTime);
    }

    // Close connection
    close(sockfd);
}
