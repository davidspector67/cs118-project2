#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "utils.h"

// Slide client window, return number of segments in new window
void slide_window(struct packet *window [WINDOW_SIZE], size_t positions_to_slide, short *num_segments_in_window) {
    for (size_t i = positions_to_slide; i < WINDOW_SIZE; ++i)
	window[i-positions_to_slide] = window[i];
    *num_segments_in_window -= positions_to_slide;
}

int main(int argc, char *argv[]) {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in client_addr, server_addr_to, server_addr_from;
    socklen_t addr_size = sizeof(server_addr_to);
    struct timeval tv;
    struct packet pkt;
    struct packet ack_pkt;
    char buffer[PAYLOAD_SIZE];
    unsigned short seq_num = 0;
    unsigned short ack_num = 0;
    char last = 0;
    char ack = 0;

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    // read filename from command line argument
    if (argc != 2) {
        printf("Usage: ./client <filename>\n");
        return 1;
    }
    char *filename = argv[1];

    // Create a UDP socket for listening
    listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sockfd < 0) {
        perror("Could not create listen socket");
        return 1;
    }

    // Listen socket timeout
    if (setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Could not set listen socket timeout");
        return 1;
    }

    // Create a UDP socket for sending
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Could not create send socket");
        return 1;
    }

    // Configure the server address structure to which we will send data
    memset(&server_addr_to, 0, sizeof(server_addr_to));
    server_addr_to.sin_family = AF_INET;
    server_addr_to.sin_port = htons(SERVER_PORT_TO);
    server_addr_to.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Configure the client address structure
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the client address
    if (bind(listen_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(listen_sockfd);
        return 1;
    }

    // Open file for reading
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error opening file");
        close(listen_sockfd);
        close(send_sockfd);
        return 1;
    }

    // TODO: Read from file, and initiate reliable data transfer to the server

    // Connect send socket to the server address to which we will send data
    if (connect(send_sockfd, (struct sockaddr *)&server_addr_to, sizeof(server_addr_to)) < 0) {
	perror("Server failed to connect to proxy server");
	close(send_sockfd);
	return 1;
   }

    // Partition file into packets and send to server
    size_t bytes_read;
    ssize_t listen_socket_state;
    struct packet *window_packets [WINDOW_SIZE];
    short num_segments_in_window = 0;
    while (1) {
	// If room in our window, send a new segment
	if (num_segments_in_window < WINDOW_SIZE) {
	    bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp);
	    // Done reading file
	    if (!bytes_read) {
		break;
	    }
	    

	    // Create packet to send to server
	    build_packet(&pkt, seq_num, ack_num, last, ack, bytes_read, (const char*)buffer);

	    // Send segment to server socket
	    if (send(send_sockfd, &pkt, sizeof(pkt), 0) < 0) {
		perror("Error sending request");
		close(send_sockfd);
		return 1;
	    }
	    window_packets[num_segments_in_window] = &pkt;
    	    ++num_segments_in_window;
	}

        // Read ACK from server
	listen_socket_state = recv(listen_sockfd, &ack_pkt, sizeof(ack_pkt), 0);	
        if (listen_socket_state < 0) {
	    if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
		perror("Read error or closed from server");
	    	close(listen_sockfd);
       	        close(send_sockfd);
                fclose(fp);
		return 1;
	    }
	    // Retransmit first segment from window upon ACK timeout
	    printf("Listen socket timed out! Retransmitting first segment in window\n");
	    if (send(send_sockfd, window_packets[0], sizeof(*(window_packets[0])), 0) < 0) {
		perror("Error sending request");
		close(send_sockfd);
		return 1;
	    }
	    continue;
	}

	// ACK policy --> all cases are written out right now in case of errors
	if (!ack_pkt.ack) {
	    printf("No ACK received in client. Current seqnum is %i\n", seq_num);
	}

	if (ack_pkt.acknum > (MAX_SEQUENCE - 2*WINDOW_SIZE) && seq_num < 2*WINDOW_SIZE) 
	    ++seq_num; // ACK is behind seqnum in wraparound case, so prepare to send next segment
	else if (seq_num > (MAX_SEQUENCE - 2*WINDOW_SIZE) && ack_pkt.acknum < 2*WINDOW_SIZE) {
	    slide_window(window_packets, (size_t) (WINDOW_SIZE-seq_num + ack_pkt.acknum), &num_segments_in_window);
	    seq_num = ack_pkt.acknum; // seq_num is behind ACK in wraparound case
	} else if (seq_num < ack_pkt.acknum) {
	    slide_window(window_packets, (size_t)(ack_pkt.acknum-seq_num), &num_segments_in_window);
	    seq_num = ack_pkt.acknum; // seq_num is behind ACK in traditional case
	} else if (seq_num > ack_pkt.acknum)
	    ++seq_num; // ACK is behind seqnum in traditional case
	else if (seq_num == ack_pkt.acknum)
	   continue; // perfect
	else
	   printf("Don't think this is a possible case\n");

	printf("Num segs in window: %i\n", num_segments_in_window);	
	  
    }
         
    printf("%zu\n", bytes_read);
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

