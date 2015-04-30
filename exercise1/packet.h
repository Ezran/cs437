#ifndef PACKET_H
#define PACKET_H
#define MAX_PAYLOAD_SIZE (MAX_MESS_LEN - 2*sizeof(unsigned int))

typedef struct data_packet {
	unsigned int index;
	unsigned int payload_len;
	char payload[MAX_PAYLOAD_SIZE];	
} data_packet;

typedef struct ack_packet {
	unsigned int index;
} ack_packet;

typedef struct data_packet_ex {
	unsigned int index;
	unsigned int total_packets;
	unsigned int payload_len;
	char payload[MAX_PAYLOAD_SIZE-sizeof(unsigned int)];
} data_packet_ex;

typedef struct ack_packet_ex {
	int acklist[MAX_MESS_LEN/sizeof(int)];
} ack_packet_ex;

#endif
