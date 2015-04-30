#include <arpa/inet.h>
#include "queue.h"
#include "sendto_dbg.h"
#include "packet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_TIMEOUT 1000

int main(int argc, char* argv[])
{
	int 			filesize, finished, loss, sr, ss, target_num, num, from_len, from_ip, last_ack, timeout_limit;
	char* 			target_name;
	struct sockaddr_in 	my_addr, target_addr;
	struct hostent 		host_ent, *host_ent_p;
	fd_set			mask, reset_mask;
	struct timeval		timeout;
	FILE*			fp;
	int 			i, conf, iter_ind;

	if (argc < 4){
		printf("Useage: ncp <loss> <source> <dest>@<comp>");
		exit(0);
	}
	//target_name = argv[1];
	//loss = atoi(argv[3]);
	char *hoststr;
	hoststr = (char *) malloc(strlen(argv[3]) + 1);
	strcpy(hoststr, argv[3]);
	target_name = strchr(hoststr, '@') + 1;
	loss = atoi(argv[1]);

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

	host_ent_p = gethostbyname(target_name);
	if (host_ent_p == NULL){
		printf("gethostbyname error\n");
		exit(1);
	}
	memcpy(&host_ent, host_ent_p, sizeof(host_ent));
	memcpy(&target_num, host_ent.h_addr_list[0], sizeof(target_num));

	fp = fopen(argv[2], "r");
	last_ack = -1;

	target_addr.sin_family = AF_INET;
	target_addr.sin_addr.s_addr = target_num;
	target_addr.sin_port = htons(PORT);

	FD_ZERO( &reset_mask);
	FD_SET( sr, &reset_mask);

	sendto_dbg_init(loss);
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;

	// read file into data struct
	while (!feof(fp)) {	
		char newstr[MAX_PAYLOAD_SIZE-sizeof(unsigned int)];
		push(newstr,fread(newstr, 1, MAX_PAYLOAD_SIZE-sizeof(unsigned int), fp)); 
	}
	fclose(fp);
	printf("file read complete, %d nodes\n",get_size());

	node * iter = get_head();
	int index_count = get_size();
	int packet_count = get_size();

	data_packet_ex * sending = malloc(sizeof(data_packet_ex));
	if (sending == NULL){
		printf("malloc error\n");
		exit(1);
	}
	// send first round
	while (iter != NULL) {
		sending->index = index_count;
		sending->total_packets = packet_count;
		sending->payload_len = iter->data_len;
		memcpy(sending->payload, iter->data,iter->data_len);

		sendto_dbg(ss, (char*)sending, sizeof(sending->payload)+3*sizeof(unsigned int) , 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
		iter = iter->next;
		index_count--;
	}

	int ack_array[packet_count];
	for(;;) {
		mask = reset_mask;
		num = select( FD_SETSIZE, &mask, NULL, NULL, &timeout);
		if (num > 0 && FD_ISSET(sr, &mask)) { //we receive a packet from server
			timeout_limit = 0;
			ack_packet_ex * received = malloc(sizeof(ack_packet_ex));
			recvfrom(sr, (char*)received, sizeof(ack_packet_ex), 0, NULL, NULL);
			
			// update the ack array
			for (i = 0; i < sizeof(received->acklist); i++){
				if (received->acklist[i] > -1)
					ack_array[received->acklist[i]] = 1;	
			}
			free(received);
		
			//send the missing packets	
			conf = 1;
			iter = get_head();
			iter_ind = packet_count-1;
			while (iter != NULL) {
				if (ack_array[iter_ind] == 0) {
					conf = 0;
					sending->index = iter_ind;
					sending->total_packets = packet_count;
					sending->payload_len = iter->data_len;
					memcpy(sending->payload,iter->data,iter->data_len);
					
					sendto_dbg(ss, (char*)sending, sending->payload_len+3*sizeof(unsigned int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
				}	
				iter = iter->next;
				iter_ind--;
			}

			//check if all ack
			if (conf == 1) {
				while(get_size > 0) 
					pop(&i);
				printf("success\n");
				exit(0);
			}
		}
		else {
			timeout_limit++;
			printf("timeout %d\n",timeout_limit);
			if (timeout_limit == MAX_TIMEOUT){
				printf("No response from server, terminating.\n");
				while(get_size() > 0)
					pop(&i);
				exit(1);
			}
			//send the missing packets	
			conf = 1;
			iter = get_head();
			iter_ind = packet_count-1;
			while (iter != NULL) {
				if (ack_array[iter_ind] == 0) {
					conf = 0;
					sending->index = iter_ind;
					sending->total_packets = packet_count;
					sending->payload_len = iter->data_len;
					memcpy(sending->payload,iter->data,iter->data_len);
					
					sendto_dbg(ss, (char*)sending, sending->payload_len+3*sizeof(unsigned int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
				}	
				iter = iter->next;
				iter_ind--;
			}
		}
	}
}

