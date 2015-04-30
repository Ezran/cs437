#include "sendto_dbg.h"
#include "net_include.h"
#include "packet.h"

#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_TIMEOUT 1000

int main(int argc, char* argv[])
{
    int 			loss, sr, ss, target_num, num, from_ip, ack;
    char* 			target_name,target_filename;
    struct sockaddr_in 	        my_addr, target_addr;
    struct hostent 		host_ent, *host_ent_p;
    fd_set			mask, reset_mask;
    struct timeval		timeout;
    struct timeval	        start, interm, end;
    FILE*			fp;
    int	        	        mb_sent,interm_mb;
    long long	        	seconds;
    char                        mess[MAX_MESS_LEN];

    if (argc != 4){
            printf("Useage: ncp <loss> <sourcefile> <dest>@<comp>\n");
            exit(0);
    }
    char *hoststr;
    hoststr = (char *) malloc(strlen(argv[3]) + 1);
    strcpy(hoststr, argv[3]);
    
    target_name = strchr(hoststr, '@') + 1;
    strchr(hoststr, '@') = '\0';
    target_filename = hoststr;
    loss = atoi(argv[1]);
    ack = 0;

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

    target_addr.sin_family = AF_INET;
    target_addr.sin_addr.s_addr = target_num;
    target_addr.sin_port = htons(PORT);

    FD_ZERO( &reset_mask);
    FD_SET( sr, &reset_mask);

    sendto_dbg_init(loss);
    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;

    gettimeofday(&start,NULL);   //start the timer
    interm = start;
    interm_mb = 0;

    // send the first packet
    data* fpacket = (data*)mess;
    fpacket->type = 0;
    fpacket->payload_len = strlen(target_filename);
    fpacket->index = 0;
    strcpy(fpacket->payload,target_filename);
    for(;;) {
        mask = reset_mask;
        num = select( FD_SETSIZE, &mask, NULL, NULL, &timeout);
        if (num > 0 && FD_ISSET(sr, &mask))  //we receive a confirm from server
            break;
        else
            sendto_dbg(ss, mess, fpacket->payload_len+3*sizeof(int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
    
    }

    int eof = 0;
    for(;;) {
        mask = reset_mask;
        num = select( FD_SETSIZE, &mask, NULL, NULL, &timeout);
        if (num > 0 && FD_ISSET(sr, &mask)) { //we receive a packet from server
            recv(sr, mess, MAX_MESS_LEN, 0);
            data* dat = (data*)mess;
            if (dat->type == 2) {
                if (dat->index == ack + 1)
                    ack++;
                fseek(fp, ack*(MAX_MESS_LEN - 3*sizeof(int)),SEEK_SET);
                for (int i = 1; i <= BURST_SIZE; i++) {
                    dat->type = 1;
                    dat->index = ack+i;
                    dat->payload_len = fread(dat->payload, char, MAX_MESS_LEN - 3*sizeof(int),fp);
                    if (dat->payload_len < MAX_MESS_LEN - 3*sizeof(int)){
                        eof = 1; 
                        i = BURST_SIZE+1;
                    }
                    sendto_dbg(ss, mess, dat->payload_len+3*sizeof(int), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
                }
            }
            else if (dat->type == 3) {
                //send finished and exit
            }
        }
        else {

        }
    }
}

