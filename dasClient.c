#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "utils.h"

int main(int argc, char *argv[]) {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in client_addr, server_addr_to, server_addr_from;
    socklen_t addr_size = sizeof(server_addr_to);
    struct timeval tv;
    struct packet pkt;
    struct packet ack_pkt;
    char buffer[PAYLOAD_SIZE];
    unsigned int seq_num = 0;
    unsigned int ack_num = 0;
    char last = 0;
    char ack = 0;
    int expected_ack = 1;

    //tv.tv_sec = TIMEOUT;
    //tv.tv_usec = 0;

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
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        close(listen_sockfd);
        close(send_sockfd);
        return 1;
    }
    
    //int file_length;
    //fseek(fp, 0, SEEK_END);
    //file_length = ftell(fp);
    //int last_packet_length = file_length % PAYLOAD_SIZE;
    //int num_packets = file_length/PAYLOAD_SIZE + (file_length % PAYLOAD_SIZE != 0);

    //fseek(fp, 0, SEEK_SET);

    ssize_t bytes_read;
    //struct packet segments[num_packets];
    //while((bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp)) > 0) {
      //  if (bytes_read < PAYLOAD_SIZE) {
      //      last = 1;
      //  }
      //  build_packet(&pkt, seq_num, ack_num, last, ack, bytes_read, (const char*) buffer);
      //  segments[seq_num] = pkt;
      //  seq_num++;
    //}
    //close(fp);
    
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    
    //for (size_t i = 0; i < PAYLOAD_SIZE; ++i) {
    //    printf("%c", segments[1].payload[i]);
    //}
    //printf("\n");

    
    // TODO: Read from file, and initiate reliable data transfer to the server
    ssize_t bytes_recv;
    struct queue *window_start;
    struct queue *cur_window_seg;
    struct queue nodes[WINDOW_SIZE];
    for (size_t i = 0; i < WINDOW_SIZE-1; ++i) {
        nodes[i].next = &nodes[i+1];
        nodes[i].used = i;
	*(nodes[i].payload) = malloc(PAYLOAD_SIZE);
	nodes[i].seqnum = 0;
    }
    nodes[WINDOW_SIZE-1].next = &nodes[0];
    nodes[WINDOW_SIZE-1].used = WINDOW_SIZE-1;
    nodes[WINDOW_SIZE-1].seqnum = 0;
    window_start = &(nodes[0]);

    seq_num = 0;
    float cwnd = 1;
    int ssthresh = 6;
    int dup_ack = 0;
    int prev_ack = 0;
    int in_transit = 0;
    int fr_phase = 0;
    float cwnd_change = 0;
    int high_seq_num = 0;
    int start_seq_num = 0;
    int cur_seq_num = 0;
    char in_timeout = 0;
    int listen_socket_state;
    while(1){
	//if (start_seq_num > 15)
	 //   return;
	// Transmit appropriate packets
      //  if (window_start->seqnum == seq_num) { // Retransmission
    //	build_packet(&pkt, seq_num, ack_num, last, ack, cur_window_seg->length, cur_window_seg->payload);
    //	sendto(send_sockfd, &(pkt), sizeof(pkt), 0, (struct sockaddr *)&server_addr_to, addr_size);
      //  }
	//else {
//	printf("Seqnum's of all nodes:\n");
//	cur_window_seg = window_start;
//	for (size_t i = 0; i < WINDOW_SIZE; i++) {
//	    printf("%i     ", cur_window_seg->seqnum);
//	    cur_window_seg = cur_window_seg->next;
//	} 
//	printf("\n");
//	cur_window_seg = window_start;
  //      for (size_t i = 0; i < WINDOW_SIZE; i++) {
    //        printf("%i     ", cur_window_seg->used);
      //      cur_window_seg = cur_window_seg->next;
       // }
	if (in_timeout) {
	    build_packet(&pkt, window_start->seqnum, ack_num, last, ack, window_start->length, window_start->payload);
	    sendto(send_sockfd, &(pkt), sizeof(pkt), 0, (struct sockaddr *)&server_addr_to, addr_size);
	    printSend(&pkt, 0);
//	    setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
//	    listen_socket_state = recv(listen_sockfd, &ack_pkt, sizeof(ack_pkt), 0);
//	    if (listen_socket_state < 0) {
//		if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
//		    perror("Read error or closed from server");
//		    close(listen_sockfd);
//		    close(send_sockfd);
//		    fclose(fp);
//		    return 1;
//		}
//	    }
//	    printf("in timeout\n");
//	    printRecv(&ack_pkt);
//	    if (ack_pkt.acknum > start_seq_num)
//		in_timeout = 0;
//	    start_seq_num = ack_pkt.acknum;
//	    high_seq_num = start_seq_num;
//	    continue;
	} else if (in_transit < (int)cwnd) {
	    cur_window_seg = window_start;
	    for (int i = start_seq_num; i < (high_seq_num); ++i) cur_window_seg = cur_window_seg->next;
	    for (; high_seq_num < start_seq_num + (int)cwnd && in_transit < (int)cwnd; ++high_seq_num)
	    {
		cur_seq_num = high_seq_num % MAX_SEQUENCE;
		bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp);
		if (bytes_read == -1) {
		    perror("Error reading from file");
		    fclose(fp);
		    close(listen_sockfd);
		    close(send_sockfd);
		    return 1;
		}
		//else if (bytes_read == 0)
	    //	break;
		else if (bytes_read < PAYLOAD_SIZE) {
		    pkt.last = 1;
		    printf("Last packet found! %i bytes read!\n", bytes_read);
	    //	return;
		}
		memcpy(cur_window_seg->payload, (const char *)buffer, bytes_read);
		cur_window_seg->seqnum = cur_seq_num;
		cur_window_seg->length = bytes_read;
		build_packet(&pkt, cur_seq_num, ack_num, last, ack, cur_window_seg->length, cur_window_seg->payload);
		sendto(send_sockfd, &(pkt), sizeof(pkt), 0, (struct sockaddr *)&server_addr_to, addr_size);
		printSend(&pkt, 0);
		in_transit++;
		cur_window_seg = cur_window_seg->next;
	    }
//	    high_seq_num += (start_seq_num + (int)cwnd - high_seq_num);
	//    printf("Start_seq_num: %i, Actual first window seqnum: %i, high_seq_num: %i\n", start_seq_num, window_start->seqnum, high_seq_num);
	}

	// Handle timeouts	
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(listen_sockfd, &read_fds);

	int select_result = select(listen_sockfd + 1, &read_fds, NULL, NULL, &timeout);
	if (select_result == -1) {
		    perror("Error in select");
		    fclose(fp);
		    close(listen_sockfd);
		    close(send_sockfd);
		    return 1;
	}
	else if (select_result == 0) {
	    printf("Timeout occurred. Retransmitting packet: %d\n", start_seq_num);
	    if (cwnd/2 > 2)
		ssthresh = cwnd/2;
	    else
		ssthresh = 2;
	    cwnd = 1;
	    high_seq_num = start_seq_num;
	    printf("Start_seq_num, %i, Actual first window seqnum: %i, high_seq_num: %i\n", start_seq_num, window_start->seqnum, high_seq_num);
	    in_timeout = 1;
	    continue;
	}
	bytes_recv = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)&server_addr_from, &addr_size);
	printRecv(&ack_pkt);
	in_transit--;

	// Slow start
	if ((int)cwnd <= ssthresh) {
	    if (ack_pkt.acknum > start_seq_num) {
		cwnd_change = (ack_pkt.acknum - start_seq_num);
		cur_window_seg = window_start;
		for (size_t i = 0; i < (int)cwnd_change; ++i) {
	//	    printf("cur_window_seg->seqnum: %i\n", cur_window_seg->seqnum);
		    cur_window_seg = cur_window_seg->next;
		} if (cwnd+cwnd_change > WINDOW_SIZE)
		    cwnd_change = WINDOW_SIZE-cwnd;
		cwnd += cwnd_change;
		start_seq_num = ack_pkt.acknum;
		window_start = cur_window_seg;
	    }
	    else if (start_seq_num > (MAX_SEQUENCE-WINDOW_SIZE) && ack_pkt.acknum < WINDOW_SIZE) {
	    	cwnd_change = MAX_SEQUENCE-start_seq_num+ack_pkt.acknum;	 
		cur_window_seg = window_start;
                for (size_t i = 0; i < (int)cwnd_change; ++i) {
         //           printf("cur_window_seg->seqnum: %i\n", cur_window_seg->seqnum);
		    cur_window_seg = cur_window_seg->next;
		} if (cwnd+cwnd_change > WINDOW_SIZE)
                    cwnd_change = WINDOW_SIZE-cwnd;
                cwnd += cwnd_change;
                start_seq_num = ack_pkt.acknum;
                window_start = cur_window_seg;
	    }	
	}
	else { // Congestion Avoidance
	    if (ack_pkt.acknum > start_seq_num) {
	        cwnd_change = (float)(ack_pkt.acknum - prev_ack)/cwnd;
		cur_window_seg = window_start;
                for (size_t i = 0; i < (int)cwnd_change; ++i) {
           //         printf("cur_window_seg->seqnum: %i\n", cur_window_seg->seqnum);
		    cur_window_seg = cur_window_seg->next;
		} if (cwnd + cwnd_change > WINDOW_SIZE)
		    cwnd_change = WINDOW_SIZE-cwnd;
		cwnd += cwnd_change;
		start_seq_num = ack_pkt.acknum;
		window_start = cur_window_seg;
	    }
	    else if (start_seq_num > (MAX_SEQUENCE-WINDOW_SIZE) && ack_pkt.acknum < WINDOW_SIZE) {
		cwnd_change = MAX_SEQUENCE-start_seq_num+ack_pkt.acknum;
		cur_window_seg = window_start;
                for (size_t i = 0; i < (int)cwnd_change; ++i) {
            //        printf("cur_window_seg->seqnum: %i\n", cur_window_seg->seqnum);
		    cur_window_seg = cur_window_seg->next;
		} if (cwnd + cwnd_change > WINDOW_SIZE)
                    cwnd_change = WINDOW_SIZE-cwnd;
                cwnd += cwnd_change;
                start_seq_num = ack_pkt.acknum;
                window_start = cur_window_seg;
	    }	
	}

	if (in_timeout && ack_pkt.acknum > start_seq_num) {
            in_timeout = 0;
	    high_seq_num = start_seq_num;
        }


	//printf("prev_ack: %i\n", prev_ack);
//	if (ack_pkt.seqnum) // FINACK
//	    break;
 //       if (ack_pkt.acknum != prev_ack){
  //          if (fr_phase == 1){
   //             cwnd = ssthresh;
    //            fr_phase = 0;
     //       }
      //      else{
       //         if ((int)cwnd <= ssthresh) {
//		    if (ack_pkt.acknum > prev_ack)
 //                   	cwnd_change = (ack_pkt.acknum - prev_ack); //possible this stretches slow start past the cwnd <= ssthresh condition
//		    else
///			cwnd_change = WINDOW_SIZE-prev_ack+ack_pkt.acknum;
//		    //seq_num = ((int)(seq_num + cwnd_change)) % MAX_SEQUENCE;
//		    if (cwnd+cwnd_change > WINDOW_SIZE)
//			cwnd_change = WINDOW_SIZE-cwnd;
//		    cwnd += cwnd_change;
//		    cur_window_seg = window_start;
//		    start_seq_num += (int)cwnd_change;
//		    if (prev_ack >= ack_pkt.acknum) {
//		        for (size_t i = 0; i < (int)cwnd_change; ++i) cur_window_seg = cur_window_seg->next;
//		        window_start = cur_window_seg;
//		    }
  //              }
  //              else {
//		    if (ack_pkt.acknum > prev_ack)
 //                       cwnd_change = (float)(ack_pkt.acknum - prev_ack)/cwnd;
//		    else
//			cwnd_change = (float)(WINDOW_SIZE-prev_ack+ack_pkt.acknum)/cwnd;
//		    //seq_num = ((int)(seq_num + cwnd_change)) % MAX_SEQUENCE;
//		    if (cwnd + cwnd_change > WINDOW_SIZE)
//			cwnd_change = WINDOW_SIZE-cwnd;
//		    cwnd += cwnd_change;
//		    cur_window_seg = window_start;
//		    for (size_t i = 0; i < (int)(ack_pkt.acknum-prev_ack)/cwnd; ++i) cur_window_seg = cur_window_seg->next;
//		    window_start = cur_window_seg;
 //               }
  //         }
	   // if (ack_pkt.acknum > prev_ack)
            	//in_transit -= (ack_pkt.acknum - prev_ack);
	   // else
	//	in_transit -= (WINDOW_SIZE-prev_ack+ack_pkt.acknum);
   //         prev_ack = ack_pkt.acknum;
     //       dup_ack = 0;
       // }
        //else
        //    dup_ack++;
       // if (dup_ack == 3) {
        //    fr_phase = 1;
         //   if (cwnd/2 > 2)
         //       ssthresh = cwnd/2;
          //  else
            //    ssthresh = 2;
          //  if (ssthresh+3 <= WINDOW_SIZE)
//		cwnd = ssthresh + 3;
//	    else
//		cwnd = WINDOW_SIZE;
//	    build_packet(&pkt, window_start->seqnum, ack_num, last, ack, window_start->length, window_start->payload);
  //          sendto(send_sockfd, &(pkt), sizeof(pkt), 0, (struct sockaddr *)&server_addr_to, addr_size);
    //    }
      //  if (dup_ack > 3){
       //     if (cwnd+1 <= WINDOW_SIZE)
//		cwnd++;
  //      }
            
        /*if (ack_pkt.acknum == seq_num + 1) {
            printf("Received acknowledgment for sequence number %hu\n", seq_num);
            seq_num = ack_pkt.acknum;
            cwnd++;
        }
        else {
            printf("Received acknowledgment for unexpected sequence number %hu (out-of-order acknowledgment)\n", ack_pkt.acknum);
        }*/
	printf("Window size: %f, window start seqnum: %i\n", cwnd, start_seq_num);
    }
    
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

