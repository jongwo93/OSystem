#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rtp.h"

/* GIVEN Function:
 * Handles creating the client's socket and determining the correct
 * information to communicate to the remote server
 */
CONN_INFO* setup_socket(char *ip, char *port){
	struct addrinfo *connections, *conn = NULL;
	struct addrinfo info;
	memset(&info, 0, sizeof(struct addrinfo));
	int sock = 0;

	info.ai_family = AF_INET;
	info.ai_socktype = SOCK_DGRAM;
	info.ai_protocol = IPPROTO_UDP;
	getaddrinfo(ip, port, &info, &connections);

	/*for loop to determine corr addr info*/
	for(conn = connections; conn != NULL; conn = conn->ai_next){
		sock = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
		if(sock <0){
			if(DEBUG)
				perror("Failed to create socket\n");
			continue;
		}
		if(DEBUG)
			printf("Created a socket to use.\n");
		break;
	}
	if(conn == NULL){
		perror("Failed to find and bind a socket\n");
		return NULL;
	}
	CONN_INFO* conn_info = malloc(sizeof(CONN_INFO));
	conn_info->socket = sock;
	conn_info->remote_addr = conn->ai_addr;
	conn_info->addrlen = conn->ai_addrlen;
	return conn_info;
}

void shutdown_socket(CONN_INFO *connection){
	if(connection)
		close(connection->socket);
}

/* 
 * ===========================================================================
 *
 *			STUDENT CODE STARTS HERE. PLEASE COMPLETE ALL FIXMES
 *
 * ===========================================================================
*/


/*
 *  Returns a number computed based on the data in the buffer.
*/
static int checksum(const char *buffer, int length){

	/*  ----  FIXME  ----
	 *
	 *  The goal is to return a number that is determined by the contents
	 *  of the buffer passed in as a parameter.  There a multitude of ways
	 *  to implement this function.  For simplicity, simply sum the ascii
	 *  values of all the characters in the buffer, and return the total.
	*/
	int retSum = 0;
	for(int i = 0; i < length; i++) {
		retSum += buffer[i];
	}

	return retSum;
}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it 
 *  contains the length of the array created.
 */
static PACKET* packetize(const char *buffer, int length, int *count){

	/*  ----  FIXME  ----
	 *  The goal is to turn the buffer into an array of packets.
	 *  You should allocate the space for an array of packets and
	 *  return a pointer to the first element in that array.  Each 
	 *  packet's type should be set to DATA except the last, as it 
	 *  should be LAST_DATA type. The integer pointed to by 'count' 
	 *  should be updated to indicate the number of packets in the 
	 *  array.
	*/
	int num_packets = length / MAX_PAYLOAD_LENGTH;
	if(length % MAX_PAYLOAD_LENGTH != 0) {
		num_packets++;
	}
	
	*count = num_packets;
	PACKET *packet_arr = (PACKET*) malloc(sizeof(PACKET) * num_packets);

	/*typedef struct _PACKET{
	int type;
	int checksum;
	int payload_length;
	char payload[MAX_PAYLOAD_LENGTH];
	} PACKET;*/

	for(int i = 0; i < num_packets; i++) {
		if(i == num_packets -1) {
			packet_arr[i].type = LAST_DATA;
			if(length % MAX_PAYLOAD_LENGTH ==0) {
				packet_arr[i].payload_length = MAX_PAYLOAD_LENGTH;
			} else {
				packet_arr[i].payload_length = length % MAX_PAYLOAD_LENGTH;
			}
		} else {
			packet_arr[i].type = DATA;
			packet_arr[i].payload_length = MAX_PAYLOAD_LENGTH;

		}
		for(int pl = 0; pl < packet_arr[i].payload_length; pl++) {
			packet_arr[i].payload[pl] = buffer[(i * MAX_PAYLOAD_LENGTH) + pl];
		}

		packet_arr[i].checksum = checksum(packet_arr[i].payload, packet_arr[i].payload_length);

	}
	return packet_arr;
}

/*
 * Send a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
int rtp_send_message(CONN_INFO *connection, MESSAGE *msg){
	/* ---- FIXME ----
	 * The goal of this function is to turn the message buffer
	 * into packets and then, using stop-n-wait RTP protocol,
	 * send the packets and re-send if the response is a NACK.
	 * If the response is an ACK, then you may send the next one

	 typedef struct _CONN_INFO{
		int socket;
		socklen_t addrlen;
		struct sockaddr *remote_addr;
	} CONN_INFO;

	typedef struct _MESSAGE{
		int length;
		const char *buffer;
	} MESSAGE;
	ssize_t sendto(int s, const void *buf, size_t len,
               int flags, const struct sockaddr *to,
               socklen_t tolen);
	*/
	int *count = (int*) malloc(sizeof(int));
	PACKET *packet_arr = packetize(msg -> buffer, msg -> length, count); // packetize.
	PACKET *packet_res = (PACKET*) malloc(sizeof(PACKET));

	for(int i = 0; i < *count; i++) {
		while(1) {
			sendto(connection -> socket, &packet_arr[i], sizeof(PACKET), 0, connection -> remote_addr, connection -> addrlen);
			recvfrom(connection -> socket, packet_res, sizeof(PACKET), 0, connection -> remote_addr, &connection -> addrlen);
			if(packet_res -> type == ACK) {
				break;
			} else if(packet_res -> type != NACK) {
				return -1;
			}

		}
		//printf("packet sent = %d \n", i);

	}
	return 1;
}

/*
 * Receive a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
MESSAGE* rtp_receive_message(CONN_INFO *connection){
	/* ---- FIXME ----
	 * The goal of this function is to handle 
	 * receiving a message from the remote server using
	 * recvfrom and the connection info given. You must 
	 * dynamically resize a buffer as you receive a packet
	 * and only add it to the message if the data is considered
	 * valid. The function should return the full message, so it
	 * must continue receiving packets and sending response 
	 * ACK/NACK packets until a LAST_DATA packet is successfully 
	 * received.
	*/
	MESSAGE *msg = (MESSAGE*) malloc (sizeof(MESSAGE));

	char *buffer = (char*) malloc(sizeof(char));
	//int buffer_index = 0;
	PACKET *incoming_packet = (PACKET*) malloc (sizeof(PACKET));
	while(1){
		recvfrom(connection -> socket, incoming_packet, sizeof(PACKET), 0, connection -> remote_addr, &connection -> addrlen);
		PACKET *packet_res = (PACKET*) malloc(sizeof(PACKET));
		if(incoming_packet -> checksum == checksum(incoming_packet -> payload, incoming_packet -> payload_length)) {
			
			packet_res -> type = ACK;
			sendto(connection -> socket, packet_res, sizeof(PACKET), 0, connection -> remote_addr, connection -> addrlen);
			buffer = (char*)realloc(buffer, (msg -> length + incoming_packet -> payload_length) * sizeof(char));
			for(int i = 0; i < incoming_packet -> payload_length; i++) {
				buffer[msg -> length + i] = incoming_packet -> payload[i];
				
			}
			msg -> length += incoming_packet -> payload_length;
			printf("BUFFFER = %s \n", buffer);
			if(incoming_packet -> type == LAST_DATA) {
				break;
			}


		} else {
			packet_res -> type = NACK;
			sendto(connection -> socket, packet_res, sizeof(PACKET), 0, connection -> remote_addr, connection -> addrlen);
		}
		
			
	}
	msg -> buffer = buffer;

	//printf("message lenth : %d", msg->length);
	return msg;
}
