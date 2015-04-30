#include "net_include.h"
#include "packet.h"

#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

/* ------------ Variables -------------- */
/*
 * Node struct allows storage of Node,packet pairs to keep
 * track of the next Node.
 */

typedef struct node {
	struct node * next;
	struct node * prev;
	struct data_packet_ex* data;
} node;

/* ---------- Methods ---------- */

/* reads packet data from file and inserts into buffer */
/* returns -1 if file read error, otherwise returns value from push(newpacket) */
int read_next_packet(FILE*);

/* pops the first packet in the buffer and writes to file */
/* returns -2 if the buffer is empty, -1 if there was an error writing to file, 0 on success and 1 on successful EOF */ 
int write_next_packet(FILE*);

/* inserts packet into buffer and sorts */
/* return 0 on success, 1 if buffer is full and -1 if packet already exists in buffer */
int push(data_packet_ex*);

/* pops first packet and returns packet */
/* returns NULL if buffer is empty */
data_packet_ex* pop(void);

/* returns packet pointer but does not remove packet from buffer */
/* returns NULL if buffer is empty */
data_packet_ex* peek(void);

/* drop packet with given index from buffer */
/* return 0 on success, -1 if packet not found */
int purge(int);

/* set capacity of the buffer */
/* return 0 on success, -1 if current size of buffer is larger than new size */
int set_max_size(int);

int get_max_size(void);

int get_size(void);

/* return 0 on empty, 1 on full */
int is_full(void);

int get_max(void);

#endif
