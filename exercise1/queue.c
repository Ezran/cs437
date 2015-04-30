#include "queue.h"


node* head = 0x00;
node* tail = 0x00; 
unsigned int size = 0;


int push(char* indat, int dat_len)
{
	node * newnode = malloc(sizeof(node));
	if (newnode == NULL)
		return -1;

	newnode->data = malloc(dat_len);
	memcpy(newnode->data,indat,dat_len);
	newnode->data_len = dat_len;

	if (size == 0) { /* empty queue */ 
		newnode->next = 0x00;
		newnode->prev = 0x00;
		head = newnode;
		tail = newnode;
	}
	else { 
		newnode->next = head;
		head->prev = newnode;
		head = newnode;
	}
	size++;
	return 0;
}

char* pop(int* len)
{
	if(tail != 0x00)
	{
		node * tmp = tail;
		char * ret = tail->data;
		if (size > 1) {
			tail->prev->next = tail->next;
			tail = tail->prev;
		}
		else {
			head = 0x00;
			tail = 0x00;
		}
		*len = tmp->data_len;	
		free(tmp);
		size--;
		return ret;
	}
	else
		return NULL; /* queue is empty */
}

char* peek()
{
	if(tail != 0x00) 
		return tail->data;
	else
		return NULL;
}

int get_size()
{
	return size;
}

node * get_head(){
	return head;
}
