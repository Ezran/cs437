#include <arpa/inet.h>
#include "packet_queue.h"
#include "sendto_dbg.h"
#include "packet.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_TIMEOUT 1000

int main(int argc, char* argv[])
{
	int 			active, w, write_complete, loss, sr, ss, target_num, num, received_len, from_ip, last_ack, timeout_limit;
	char* 			target_name;
	struct sockaddr_in 	my_addr, received_addr;
	fd_set			mask, reset_mask;
	struct timeval		timeout;
	FILE*			fp;
	data_packet*		received;
	struct timeval	start, interm, end;
	int		mb_sent,interm_mb;
	long long 		seconds;

	received_len = sizeof(received_addr);
	write_complete = 0;
	active = 0;

	if (argc < 2){
		//printf("Useage: server <loss>\n");
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
	int bytes;
	for(;;) {
		mask = reset_mask;
		num = select( FD_SETSIZE, &mask, NULL, NULL, &timeout);
		if (num > 0 && FD_ISSET(sr, &mask)) { //we receive a packet from client
			timeout_limit = 0;
			if (active == 0) {
				active = 1;
				gettimeofday(&start,NULL);   //start the timer
				interm = start;
				interm_mb = 0;
			}
			received = malloc(sizeof(data_packet));
			bytes = recvfrom(sr, (char*)received, sizeof(data_packet), 0, (struct sockaddr*)&received_addr , &received_len);
			received_addr.sin_port = htons(PORT);


			if (interm_mb >= 50000000) { //every 50MB print speed
				gettimeofday(&end, NULL);
				seconds = (end.tv_sec * (unsigned int)1e6 + end.tv_usec) - (interm.tv_sec * (unsigned int)1e6 + interm.tv_usec);
				interm = end;
				printf("[%lld] %d MB sent @ %d mb/s\n",seconds,(int)(mb_sent/1000000),8*(int)(interm_mb/seconds));
				interm_mb = 0;
			}
			if (received->index == -1){
				gettimeofday(&end, NULL);
				seconds = (end.tv_sec * (unsigned int)1e6 + end.tv_usec) - (start.tv_sec * (unsigned int)1e6 + start.tv_usec);
				printf("File send completed in %.2f seconds. Average transfer rate for %d MB was %d mb/s. \n",(double)(seconds/1e6),(int)(mb_sent/1000000),8*(int)(mb_sent/seconds));
				free(received);
				fclose(fp);
				exit(0);
			}
			else if (received->index == last_ack+1)
			{
				w = fwrite(received->payload, 1, received->payload_len, fp);
				mb_sent = mb_sent + w;
				interm_mb = interm_mb + w;
				if ( w < MAX_PAYLOAD_SIZE )
					write_complete = 1; 	//write not-full packet, might be done
				last_ack++;
				ack_packet * ret = malloc(sizeof(ack_packet));
				ret->index = received->index;
			/*ack*/	sendto_dbg(ss, (char*)ret, sizeof(ret), 0, (struct sockaddr*)&received_addr, sizeof(received_addr));
				free(ret);
			}
			free(received);
		}
		else {
			if (write_complete == 1) {
				gettimeofday(&end, NULL);
				seconds = (end.tv_sec * (unsigned int)1e6 + end.tv_usec) - (start.tv_sec * (unsigned int)1e6 + start.tv_usec);
				printf("File send completed(timeout) in %.2f seconds. Average transfer rate for %d MB was %d mb/s. \n",(double)(seconds/1e6),(int)(mb_sent/1000000),8*(int)(mb_sent/seconds));
				fclose(fp);
				exit(0);
			}
			else if (active == 1) {
				ack_packet * temp = malloc(sizeof(ack_packet));
				temp->index = last_ack;
			/*ack*/	sendto_dbg(ss, (char*)temp, sizeof(temp), 0, (struct sockaddr*)&received_addr, sizeof(received_addr));
				free(temp);
			}
		}
	}
}

