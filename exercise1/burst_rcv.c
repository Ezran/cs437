#include <arpa/inet.h>
#include "packet_queue.h"
#include "sendto_dbg.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_TIMEOUT 1000

int main(int argc, char* argv[])
{
	int 			i, active, w, write_complete, loss, sr, ss, target_num, num, received_len, from_ip, last_ack, timeout_limit;
	char* 			target_name;
	struct sockaddr_in 	my_addr, received_addr;
	fd_set			mask, reset_mask;
	struct timeval		timeout;
	FILE*			fp;
	data_packet_ex*		received;


	received_len = sizeof(received_addr);
	write_complete = 0;
	active = 0;

	if (argc < 2){
		printf("Usage: rcv <loss>\n");
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

	fp = fopen("received", "w");
	last_ack = 0;

	FD_ZERO( &reset_mask);
	FD_SET( sr, &reset_mask);

	sendto_dbg_init(loss);
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	int num_pack_recv = 0;
	int total_packets = 0;
	int * ack_array = NULL;
	for(;;) {
		mask = reset_mask;
		num = select( FD_SETSIZE, &mask, NULL, NULL, &timeout);
		if (num > 0 && FD_ISSET(sr, &mask)) { //we receive a packet from client
			timeout_limit = 0;
			active = 1;
			data_packet_ex* received = malloc(sizeof(data_packet_ex));
			recvfrom(sr, (char*)received, sizeof(data_packet_ex), 0, (struct sockaddr*)&received_addr , &received_len);
			received_addr.sin_port = htons(PORT);
			
			if (ack_array == NULL)
				ack_array = malloc(sizeof(int)*received->total_packets);
			total_packets = received->total_packets;

			//got new packet
			push(received);
				
			//check if we have all packets
			if (get_size() == received->total_packets){
				printf("success\n");
				free(received);
				free(ack_array);
				while(get_size() > 0){
					received = pop();
					fwrite(received->payload, 1, received->payload_len, fp);
				}
				free(received);	
				fclose(fp);
				exit(0);
			}
			ack_array[received->index-1] = 1;
			if (get_size() % 100 == 0)
				printf("Size of queue %d\n",get_size());
			free(received);
		}
		else {
			if(active == 1) {
				timeout_limit++;
				if(timeout_limit == MAX_TIMEOUT) {
					printf("timeout\n");
					fclose(fp);
					exit(0);
				}
				//build ack packets
				ack_packet_ex * reply = malloc(sizeof(ack_packet_ex));
				if (reply == NULL){
					printf("malloc error\n");
					exit(1);
				}
				int c = 0;
				int payload[MAX_MESS_LEN/sizeof(int)];
				for(i = 0; i < total_packets; i++) {
					if(c == (MAX_MESS_LEN/sizeof(int))){
						c = 0;
						memcpy(reply->acklist,payload,MAX_MESS_LEN/sizeof(int));
						memset(payload,-1,sizeof(payload));
						sendto_dbg(ss, (char*)reply, sizeof(reply), 0, (struct sockaddr*)&received_addr, sizeof(received_addr));
					}
					else {
						if (ack_array[i] == 1) {
							payload[c] = i;
							c++;
						}
					}
				}
			
			}
		}
	}
}

