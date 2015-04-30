#include "net_include.h"

#ifndef QUEUE_H
#define QUEUE_H

/* ------------ Variables -------------- */
/*
 * Node struct allows storage of Node,packet pairs to keep
 * track of the next Node.
 */

typedef struct node {
	struct node * next;
	struct node * prev;
	int   data_len;
	char* data;
} node;

/* ---------- Methods ---------- */

/* inserts packet into buffer and sorts */
/* return 0 on success, 1 if buffer is full and -1 if packet already exists in buffer */
int push(char*,int);

/* pops first packet and returns packet */
/* returns NULL if buffer is empty */
char* pop(int*);

/* returns packet pointer but does not remove packet from buffer */
/* returns NULL if buffer is empty */
char* peek(void);

int get_size(void);

node * get_head(void);

#endif
