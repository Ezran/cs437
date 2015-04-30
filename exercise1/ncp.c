#include <arpa/inet.h>
#include "packet_queue.h"
#include "sendto_dbg.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_TIMEOUT 1000
#define WINDOW_SIZE 500

int main(int argc, char* argv[])
{
	int 			i, p_array_size, window_pos, finished, loss, sr, ss, target_num, num, from_len, from_ip, last_ack, timeout_limit;
	char* 			target_name;
	struct sockaddr_in 	my_addr, target_addr;
	struct hostent 		host_ent, *host_ent_p;
	fd_set			mask, reset_mask;
	struct timeval		timeout;
	FILE*			fp;
	//data_packet**		packet_array;       declared lower

	if (argc < 4){
		printf("Useage: client <servername> <filename> <loss>\n");
		exit(0);
	}
	target_name = argv[1];
	loss = atoi(argv[3]);

	finished = 0;

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

	host_ent_p = gethostbyname(target_name);
	if (host_ent_p == NULL){
		printf("gethostbyname error\n");
		exit(1);
	}
	memcpy(&host_ent, host_ent_p, sizeof(host_ent));
	memcpy(&target_num, host_ent.h_addr_list[0], sizeof(target_num));

	fp = fopen(argv[2], "r"); //get number of packets required	
	fseek(fp, 0, SEEK_END);
	double num_packets = (double)ftell(fp);
	fclose(fp);

	fp = fopen(argv[2], "r");
	p_array_size = num_packets/MAX_PAYLOAD_SIZE;
	if(num_packets/MAX_PAYLOAD_SIZE - p_array_size > 0)
		p_array_size++;	
	data_packet* packet_array[p_array_size];

	for(i = 0; i < p_array_size; i++){  //read file into array
		packet_array[i] = malloc(sizeof(data_packet));
		if (packet_array[i] == NULL){
			printf("malloc error\n");
			exit(1);
		}
		packet_array[i]->index = i;
		packet_array[i]->payload_len = fread(packet_array[i]->payload, 1, MAX_PAYLOAD_SIZE, fp);
	}
	window_pos = 0;

	target_addr.sin_family = AF_INET;
	target_addr.sin_addr.s_addr = target_num;
	target_addr.sin_port = htons(PORT);

	FD_ZERO( &reset_mask);
	FD_SET( sr, &reset_mask);

	sendto_dbg_init(loss);
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;

	// send the first packet
	sendto_dbg(ss, (char*)packet_array[0], packet_array[0]->payload_len+2*sizeof(unsigned int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));

	for(;;) {
		mask = reset_mask;
		num = select( FD_SETSIZE, &mask, NULL, NULL, &timeout);
		if (num > 0 && FD_ISSET(sr, &mask)) { //we receive a packet from server
			timeout_limit = 0;
			data_packet * received = malloc(sizeof(ack_packet));	
			if (received == NULL){
				printf("malloc error\n");
				exit(1);
			}
			recvfrom(sr, (char*)received, sizeof(ack_packet), 0, NULL, NULL);
			if ( window_pos <= received->index );
				window_pos = received->index+1;	//move window up to highest ack
			free(received);
			if (window_pos == p_array_size){
				printf("Transfer complete.\n");
				fclose(fp);
				for(i = 0; i < p_array_size;i++)
					free(packet_array[i]);
				exit(0);
			}
			for (i = window_pos; i < window_pos + WINDOW_SIZE; i++){
				if (i == p_array_size)
					i = window_pos + WINDOW_SIZE;
				else 
					sendto_dbg(ss, (char*)(packet_array[i]), (packet_array[i])->payload_len+2*sizeof(unsigned int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
			}
		} 
		else {
			timeout_limit++;
			if (timeout_limit == MAX_TIMEOUT){
				printf("No response from server, terminating.\n");
				exit(1);
			}
			sendto_dbg(ss, (char*)(packet_array[window_pos]), (packet_array[window_pos])->payload_len+2*sizeof(unsigned int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));		//try sending the first packet in window		
		}
	}
}

