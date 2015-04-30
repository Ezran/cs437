#include "packet_queue.h"


node* head = 0x00;
node* tail = 0x00; 
unsigned int max_size = 0;
unsigned int size = 0;
unsigned int index_count = 0;



int read_next_packet(FILE* src) {
	index_count++; /* the current packet's index */
	
	data_packet_ex * newpack = malloc(sizeof(data_packet_ex));
	newpack->index = index_count;
	newpack->payload_len = fread(newpack->payload, 1, MAX_PAYLOAD_SIZE, src);
	if (newpack->payload_len < MAX_PAYLOAD_SIZE && !feof(src)) {
		printf("There was an error reading from file. \n");
		return -1;
	}
				
	return push(newpack);	
}

int write_next_packet(FILE* dest) {
	data_packet_ex * writepack = pop();
	if (writepack == NULL)
		return -2;
	
	int retval = 0;
	int write_count = fwrite(writepack->payload, 1, writepack->payload_len, dest);	
	if (writepack->payload_len != write_count){
		printf("There was an error writing to file.\n");
		retval = -1;
	}
	else if (write_count < MAX_PAYLOAD_SIZE) 
		retval = 1;
	
	free(writepack);
	return retval;
}

int push(data_packet_ex* inpacket)
{
	if (max_size > 0 && size == max_size) 
		return 1;

	node* iter = head;
	while (iter != 0x00)
	{
		if ( iter->data->index == inpacket->index)
			return -1; //packet index collision
		else if (iter->data->index < inpacket->index)
		{				//found a smaller value, insert here 
			node * newnode = malloc(sizeof(node));
			newnode->next = iter;
			newnode->prev = iter->prev;
			iter->prev = newnode;
			if (newnode->prev)
				newnode->prev->next = newnode;
			else
				head = newnode; /*at back of queue, move head back */
			newnode->data = inpacket;
			size++;
			return 0;
				
		}
		else				//continue searching
			iter = iter->next;
			
	}
	node * newnode = malloc(sizeof(node));
	newnode->data = inpacket;
	if (size == 0) { /* empty queue */ 
		newnode->next = 0x00;
		newnode->prev = 0x00;
		head = newnode;
		tail = newnode;
	}
	else { /* at front, move tail forward */
		tail->next = newnode;
		newnode->prev = tail;
		tail = newnode;
	}
	size++;
	return 0;
}

data_packet_ex* pop()
{
	if(tail != 0x00)
	{
		node * pop_node = tail;
		data_packet_ex * ret = tail->data;
		if (size > 1) {
			tail->prev->next = tail->next;
			tail = tail->prev;
		}
		else {
			head = 0x00;
			tail = 0x00;
		}	
		free(pop_node);
		size--;
		return ret;
	}
	else
		return NULL; /* queue is empty */
}

data_packet_ex* peek()
{
	if(tail != 0x00) 
		return tail->data;
	else
		return NULL;
}

int purge(int pindex)
{
	if (size == 1){
		if (head->data->index == pindex){
			free(head->data); /*free the packet*/
			free(head);
			head = 0x00;
			tail = 0x00;
			size--;
			return 0;
		}
		return -1; /* not found */
	}

	node * iter = head;
	while (iter != 0x00) {
		if (iter->data->index == pindex){
			if (iter == head){
				head = head->next;
				head->prev = 0x00;	
			}
			else if (iter == tail){
				tail = tail->prev;
				tail->next = 0x00;
			}
			else {
				iter->prev->next = iter->next;
				iter->next->prev = iter->prev;
			}
			free(iter->data);
			free(iter);
			size--;
			return 0;
		}
		else
			iter = iter->next;
	}
	return -1; /* did not find packet */
}

int set_max_size(int newsize)
{
	if (newsize > -1 && size <= newsize)
		max_size = newsize;
	else
		return -1;
	return 0;
}

int get_max_size()
{
	return max_size;
}

int get_size()
{
	return size;
}
int is_full() {
	if (size == max_size)
		return 1;
	else
		return 0;	
}

int get_max() {
	if (size > 0)
		return head->data->index;
	else
		return -1;
}
