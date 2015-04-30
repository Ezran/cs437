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
	int 			finished, loss, sr, ss, target_num, num, from_len, from_ip, last_ack, timeout_limit;
	char* 			target_name;
	struct sockaddr_in 	my_addr, target_addr;
	struct hostent 		host_ent, *host_ent_p;
	fd_set			mask, reset_mask;
	struct timeval		timeout;
	struct timeval	start, interm, end;
	FILE*			fp;
	data_packet*		pending;
	int		mb_sent,interm_mb;
	long long		 seconds;

	if (argc < 4){
		//printf("Useage: client <servername> <filename> <loss>\n");
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

	printf("sockets initialized.\n");

	host_ent_p = gethostbyname(target_name);
	if (host_ent_p == NULL){
		printf("gethostbyname error\n");
		exit(1);
	}
	memcpy(&host_ent, host_ent_p, sizeof(host_ent));
	memcpy(&target_num, host_ent.h_addr_list[0], sizeof(target_num));

	fp = fopen(argv[2], "r");
	//fseek(fp, 0, SEEK_END);
	//unsigned long fsize = (unsigned long)ftell(fp);
	last_ack = -1;

	target_addr.sin_family = AF_INET;
	target_addr.sin_addr.s_addr = target_num;
	target_addr.sin_port = htons(PORT);

	FD_ZERO( &reset_mask);
	FD_SET( sr, &reset_mask);

	sendto_dbg_init(loss);
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;

	gettimeofday(&start,NULL);   //start the timer
	interm = start;
	interm_mb = 0;
	// send the first packet
	pending = malloc(sizeof(data_packet));
	pending->index = 1;
	pending->payload_len = fread(pending->payload, 1, MAX_PAYLOAD_SIZE, fp);
	sendto_dbg(ss, (char*)pending, pending->payload_len+2*sizeof(unsigned int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
	
	mb_sent = mb_sent + pending->payload_len;
	interm_mb = interm_mb + pending->payload_len;

	for(;;) {
		mask = reset_mask;
		num = select( FD_SETSIZE, &mask, NULL, NULL, &timeout);
		if (num > 0 && FD_ISSET(sr, &mask)) { //we receive a packet from server
			timeout_limit = 0;
			ack_packet * received = malloc(sizeof(ack_packet));
			recvfrom(sr, (char*)received, sizeof(ack_packet), 0, NULL, NULL);
			if (received->index == pending->index)
			{
				if (feof(fp))
					pending->index = -1;
				else
					pending->index++;
				pending->payload_len = fread(pending->payload, 1, MAX_PAYLOAD_SIZE, fp);
				mb_sent = mb_sent + pending->payload_len;
				interm_mb = interm_mb + pending->payload_len;
			}
			else if (received->index > pending->index) {
				printf("an error has occured, terminating..\n");
				exit(1);
			}
			sendto_dbg(ss, (char*)pending, pending->payload_len+2*sizeof(unsigned int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
			if (interm_mb >= 50000000) { //every 50MB print speed
				gettimeofday(&end, NULL);
				seconds = (end.tv_sec * (unsigned int)1e6 + end.tv_usec) - (interm.tv_sec * (unsigned int)1e6 + interm.tv_usec);
				interm = end;
				printf("[%lld] %d MB sent @ %d mb/s\n",seconds,(int)(mb_sent/1000000),8*(int)(interm_mb/seconds));
				interm_mb = 0;
			}
			free(received);
			if (pending->index == -1){
				gettimeofday(&end, NULL);
				seconds = (end.tv_sec * (unsigned int)1e6 + end.tv_usec) - (start.tv_sec * (unsigned int)1e6 + start.tv_usec);
				printf("File send completed in %.2f seconds. Average transfer rate for %d MB was %d mb/s. \n",(double)(seconds/1e6),(int)(mb_sent/1000000),8*(int)(mb_sent/seconds));
				fclose(fp);
				free(pending);
				exit(0);
			}
		}
		else {
			timeout_limit++;
			if (timeout_limit == MAX_TIMEOUT){
				printf("No response from server, terminating.\n");
				exit(1);
			}
			sendto_dbg(ss, (char*)pending, pending->payload_len+2*sizeof(unsigned int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
		}
	}
}

