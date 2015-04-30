#include <arpa/inet.h>
#include "packet_queue.h"
#include "sendto_dbg.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_TIMEOUT 1000

void check_write_complete(int wc, data_packet* rcv, FILE* fp);

int main(int argc, char* argv[])
{
	int 			active, w, write_complete, loss, sr, ss, target_num, num, received_len, from_ip, last_ack, timeout_limit;
	char* 			target_name;
	struct sockaddr_in 	my_addr, received_addr;
	fd_set			mask, reset_mask;
	struct timeval		timeout;
	FILE*			fp;
	data_packet*		received;
	ack_packet*		ret;

	received_len = sizeof(received_addr);	
	write_complete = 0;
	active = 0;

	if (argc < 2){
		printf("Useage: server <loss>\n");
		exit(0);
	}
	loss = atoi(argv[1]);

	sr = socket(AF_INET, SOCK_DGRAM, 0);
	if (sr < 0){
		printf("SR Error\n");
		exit(1);
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	my_addr.sin_port = htons(PORT);

	if ( bind(sr, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0){
		printf("SR Bind error\n");
		exit(1);
	}	
	
	ss = socket(AF_INET, SOCK_DGRAM, 0);
	if (ss < 0){
		printf("SS error\n");
		exit(1);
	}

	printf("sockets initialized.\n");

	fp = fopen("received", "w");
	last_ack = -1;

	ret = malloc(sizeof(ack_packet));  //for ack response
	if (ret == NULL){
		printf("malloc error\n");
		exit(1);
	}

	FD_ZERO( &reset_mask);
	FD_SET( sr, &reset_mask);

	sendto_dbg_init(loss);
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	for(;;) {
		mask = reset_mask;
		num = select( FD_SETSIZE, &mask, NULL, NULL, &timeout);
		if (num > 0 && FD_ISSET(sr, &mask)) { //we receive a packet from client 
			timeout_limit = 0;
			active = 1;
			received = malloc(sizeof(data_packet));	
			recvfrom(sr, (char*)received, sizeof(data_packet), 0, (struct sockaddr*)&received_addr , &received_len);
			received_addr.sin_port = htons(PORT);			
			
			if (received->index == last_ack+1){ //next chunck, write direct to file
				check_write_complete(fwrite(received->payload, 1, received->payload_len, fp), received, fp);
				last_ack++;
			}  
			else if (received->index > last_ack+1) //chunk out of order, store in queue
				push(received);
			while(get_size() > 0 && peek()->index == last_ack+1) {  //try to empty the queue
				data_packet * tmp = pop();
				check_write_complete(fwrite(tmp->payload, 1, tmp->payload_len, fp), received, fp);
				last_ack++;
			}	
			ret->index = last_ack;  //send ack
			sendto_dbg(ss, (char*)ret, sizeof(ret), 0, (struct sockaddr*)&received_addr, sizeof(received_addr));
		} 
		else {
			if (active == 1) 
				sendto_dbg(ss, (char*)ret, sizeof(ret), 0, (struct sockaddr*)&received_addr, sizeof(received_addr));
		}
	}
}

void check_write_complete(int wc, data_packet * rcv, FILE* fp) {
	if (wc < MAX_PAYLOAD_SIZE){
		printf("File transfer complete.\n");
		free(rcv);
		fclose(fp);
		exit(0);
	}
	free(rcv);
}
