#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

void send_ack(struct packet* pkt, unsigned short seqnum, unsigned short acknum, char last, char ack,unsigned int length, const char* payload, int send_sockfd) {
    build_packet(pkt, seqnum, acknum, last, ack, length, payload);

    // Send ACK segment to client socket
    if (send(send_sockfd, pkt, sizeof(*pkt), 0) < 0) {
	perror("Error sending ACK");
	close(send_sockfd);
	exit(1);
    }
}

void write_to_output(FILE *fp, char *buffer) {
    if (fwrite(buffer, 1, PAYLOAD_SIZE, fp) != PAYLOAD_SIZE) {
	perror("File writing couldn't be completed");
	fclose(fp);
	exit(1);
    }
}

// Slide client window, return number of segments in new window, returns number of places window slides 
size_t slide_window(struct packet *window [WINDOW_SIZE-1], int used_segs[WINDOW_SIZE-1], FILE *fp) { 
    // Find first nonused segment in window
    size_t i = 0;
    for (; i < WINDOW_SIZE-1; ++i) {
	if (!used_segs[i])
	    break;
	else
	    write_to_output(fp, (*window[i]).payload);
    }
    size_t slide_start = i;
    for (; i < WINDOW_SIZE-1; ++i) {
	if (used_segs[i]) {
    	    window[i-slide_start] = window[i];
	    used_segs[i-slide_start] = 1;
	} else {
	    used_segs[i-slide_start] = 0;
	}
    }
    for (i = WINDOW_SIZE-1-slide_start; i < WINDOW_SIZE-1; ++i)
	used_segs[i] = 0;    
    return slide_start+1;
} 

int main() {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in server_addr, client_addr_from, client_addr_to;
    struct packet buffer;
    socklen_t addr_size = sizeof(client_addr_from);
    int expected_seq_num = 0;
    unsigned short seq_num = 0;
    int recv_len;
    char ack = 1;
    char last = 0;
    struct packet ack_pkt;

    // Create a UDP socket for sending
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Could not create send socket");
        return 1;
    }

    // Create a UDP socket for listening
    listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sockfd < 0) {
        perror("Could not create listen socket");
        return 1;
    }

    // Configure the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the server address
    if (bind(listen_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(listen_sockfd);
        return 1;
    }

    // Configure the client address structure to which we will send ACKs
    memset(&client_addr_to, 0, sizeof(client_addr_to));
    client_addr_to.sin_family = AF_INET;
    client_addr_to.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    client_addr_to.sin_port = htons(CLIENT_PORT_TO);

    // Open the target file for writing (always write to output.txt)
    FILE *fp = fopen("output.txt", "w");

    // TODO: Receive file from the client and save it as output.txt
   
    // Connect send socket to the client proxy address to which we will send data
    if (connect(send_sockfd, (struct sockaddr *)&client_addr_to, sizeof(client_addr_to)) < 0) {
	perror("Client failed to connect to proxy server");
	close(send_sockfd);
	return 1;
    }

    // Inefficient window buffer implementation
    int used_segs[WINDOW_SIZE-1] = {0};
    int window_change;
    struct packet *window [WINDOW_SIZE-1]; 
    while (1) { // TODO: implement FIN sequence
	// Read from client
	if (recv(listen_sockfd, &buffer, sizeof(buffer)-1, 0) <= 0) {
	    printf("Read error or closed");
	    close(listen_sockfd);
	    return 1;
	}

	window_change = 0;
	// Handle seqnum with wraparound case included
	if (buffer.seqnum > (MAX_SEQUENCE - 2*WINDOW_SIZE) && expected_seq_num < 2*WINDOW_SIZE)
            ; // seqnum is behind expectation in wraparound case, so ignore
        else if (expected_seq_num > (MAX_SEQUENCE - 2*WINDOW_SIZE) && buffer.seqnum < 2*WINDOW_SIZE) {
	    // Expectation is behind seqnum in wraparound case, so cache if possible
	    if ((WINDOW_SIZE-expected_seq_num+buffer.seqnum)< WINDOW_SIZE) {
	    	window[(size_t)(WINDOW_SIZE-expected_seq_num+buffer.seqnum-1)] = &buffer;
		used_segs[(size_t)(WINDOW_SIZE-expected_seq_num+buffer.seqnum-1)] = 1;
	}} else if (expected_seq_num < buffer.seqnum) {
	    // Expectation is behind seqnum in traditional case, so cache if possible
	    if ((buffer.seqnum-expected_seq_num) < WINDOW_SIZE) {
		window[(size_t)(buffer.seqnum-expected_seq_num-1)] = &buffer;
		used_segs[(size_t)(buffer.seqnum-expected_seq_num-1)] = 1;
        }} else if (expected_seq_num > buffer.seqnum)
            ; // seqnum is behind expectation in traditional case, so ignore
        else if (expected_seq_num == buffer.seqnum) {
	   write_to_output(fp, buffer.payload);
           window_change += slide_window(window, used_segs, fp); // Expected_seq_num is equals seqnum, so slide the window
	} else
           printf("Don't think this is a possible case\n");

	// Send ACK
	expected_seq_num = (expected_seq_num + window_change) % MAX_SEQUENCE;
	send_ack(&ack_pkt, seq_num, expected_seq_num, last, ack, 0, NULL, send_sockfd);
    }   

    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}
