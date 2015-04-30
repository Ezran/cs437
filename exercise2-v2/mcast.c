#include "net_include.h"
#include "packet.h"
#include "recv_dbg.h"
#include "circlearray.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <time.h>

#define EMPTY_TOKEN_SIZE 8*sizeof(int)
#define FLOW_CONTROL     50
#define CQ_CAP           1000

    char               f_name[80];
    FILE *             fp;

    struct sockaddr_in name;
    struct sockaddr_in send_addr;
    struct sockaddr_in next_addr;
    struct ifaddrs*    addrs;
    struct ifaddrs*    tmp;

    struct timeval*    timeout;
    struct ip_mreq     mreq;
    unsigned char      ttl_val;
    unsigned char      loop;

    int                ss,sr;
    fd_set             mask;
    fd_set             dummy_mask,temp_mask;
    int                bytes;                                   //check bytes > 0 from recv_dbg, otherwise packet was 'dropped'
    int                num;
    char               mess_buf[MAX_PAYLOAD_LEN];
    int                my_id;
    int                next_p;                                  //next process in the token ring
    int*               ip_list;
    int                total_packets;
    int                packets_to_send;
    int                seq;
    int                aru;
    token *            old_token;
    int                saved_dec_aru;
    int                init_state;
    int                num_p;

int init_token_ring();
int set_sockets();
int transmit_phase();

int main(int argc, char* argv[]) {

    init_state = 0;
    srand(time(NULL));
    /* command line args */
    if (argc < 5){
        printf("Useage: mcast <num_packets> <machine_index> <number of machines> <loss rate>\n");
        return 0;
    }
    total_packets = atoi(argv[1]);
    packets_to_send = total_packets;
    my_id = atoi(argv[2]);
    num_p = atoi(argv[3]);
    if (my_id == num_p)
        next_p = 1; 
    else 
        next_p = my_id + 1; 
    recv_dbg_init(atoi(argv[4]), my_id);

    timeout = malloc(sizeof(struct timeval));
    timeout->tv_sec = 0;
    timeout->tv_usec = 10000;

    /* Initialize variables */
    seq = 0;
    aru = 0;
    saved_dec_aru = 0;
    ip_list = malloc(sizeof(int)*num_p);
    timeout = NULL;

    strcat(f_name,argv[2]);
    strcat(f_name,".out");
    printf("Writing to %s\n",f_name);
    fp = fopen(f_name, "w");

    /* Get my IP address */
    getifaddrs(&addrs);
    tmp = addrs;
    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET && strcmp(tmp->ifa_name,"eth0") == 0){
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            printf("My IP (%s): [%x] <%s>\n", tmp->ifa_name, pAddr->sin_addr, inet_ntoa(pAddr->sin_addr));
            ip_list[my_id-1] = (int)pAddr->sin_addr.s_addr;
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);

    /* initialize sockets */
    if (set_sockets() < 1)
        exit(0);    

    /* set up file descriptor masks */
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );


    printf("Waiting for start_mcast...\n");

    /* Init Phase */
    init_token_ring();

    printf("My ID is: %d\tNext ID is: %d@%x\n",my_id,next_p,ip_list[next_p-1]);
    return 0;
    timeout->tv_usec = 100000;

    /* Begin Transmit Phase */


    return 0;
}

int set_sockets() {

    // receiving socket
    sr = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sr<0) {
        printf("Mcast: socket");
        return 0;
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( sr, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        printf("Mcast: bind");
        return 0;
    }

    mreq.imr_multiaddr.s_addr = htonl( IPADDR );
    mreq.imr_interface.s_addr = htonl( INADDR_ANY );

    if (setsockopt(sr, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, 
        sizeof(mreq)) < 0) 
    {
        printf("Mcast: problem in setsockopt to join multicast address" );
        return 0;
    }

    /* sending socket */
    ss = socket(AF_INET, SOCK_DGRAM, 0); 
    if (ss<0) {
        perror("Mcast: socket");
        return 0;
    }

    ttl_val = 1;
    loop = 0;
    if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0)
    {
        printf("Mcast: problem in setsockopt of multicast loopback\n");
        return 0;
    }

    if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val, 
        sizeof(ttl_val)) < 0) 
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d - ignore in WinNT or Win95\n", ttl_val );
        return 0;
    }

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(IPADDR);  /* mcast address */
    send_addr.sin_port = htons(PORT);

    next_addr.sin_family = AF_INET;
    next_addr.sin_addr.s_addr = ip_list[next_p-1];
    next_addr.sin_port = htons(PORT);

    return 1;
}



int init_token_ring() {

    temp_mask = mask;
    for (;init_state < 2;){
    num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
    if (num > 0 && FD_ISSET( sr, &temp_mask)) {
        if (num != 1) break;

        if (init_state == 0)
            bytes = recv(sr, mess_buf, MAX_PAYLOAD_LEN, 0);
        else
            bytes = recv_dbg(sr, mess_buf, MAX_PAYLOAD_LEN, 0);
    
        initializer* status = (initializer*)mess_buf;
        if (bytes > 0 && status->type == TYPE_INITIALIZER){
            if (status->src_p != 0) { //if from an active server save ip addr
                if (ip_list[status->src_p-1] == 0) {
                    ip_list[status->src_p-1] = status->src_ip;
                    if (status->src_p == next_p)
                        next_addr.sin_addr.s_addr = ip_list[next_p-1];
                }
            }
            init_state = 1;

            //if i am 1 and have 2's ip, move from multicast IP to send token phase
            if (my_id == 1 && ip_list[next_p-1] > 0) {    
                printf("Starting token \n");
                token * tok = malloc(sizeof(token));
                tok->type = TYPE_TOKEN;
                tok->src_p = my_id;
                tok->src_ip = ip_list[my_id-1];
                tok->round = 1;
                tok->seq = 0;
                tok->aru = 0;
                tok->rtr_len = num_p;
                for (int i = 0; i < num_p; i++)
                    tok->rtr[i] = ip_list[i];

                old_token = tok;
                sendto( ss, (char*)old_token, sizeof(token), 0 , (struct sockaddr*)&next_addr, sizeof(next_addr));
            }
            else{ //multicast my info
                printf("Multicasting my info..\n");
                status->src_p = my_id;
                status->src_ip = ip_list[my_id-1];
                sendto( ss, mess_buf, sizeof(initializer), 0 , (struct sockaddr*)&send_addr, sizeof(send_addr));
            }
        }
        else if (bytes > 0 && status->type == TYPE_TOKEN) {
            token * dat = (token*)mess_buf;
            //printf("Recv. %d %d %d \n",dat->type,dat->src_p,dat->round);
            for (int i = 0; i < dat->rtr_len; i++) {
                if (ip_list[i] == 0 && dat->rtr[i] > 0) {
                    ip_list[i] = dat->rtr[i];
                    if (i == next_p-1)
                        next_addr.sin_addr.s_addr = ip_list[next_p-1];
                }
            }                    

            if (dat->round == 1) { 
               // printf(".");
                if (old_token == NULL) { //i haven't seen a token yet
                    printf("received new token... \n");
                    old_token = malloc(sizeof(token));
                    old_token->type = TYPE_TOKEN;
                    old_token->src_p = my_id;
                    old_token->src_ip = ip_list[my_id-1];
                    old_token->round = dat->round;
                    old_token->rtr_len = num_p;
                    for (int i = 0; i < num_p; i++)
                        old_token->rtr[i] = ip_list[i];
                } 
                else if (my_id == 1) {
                    printf("received token from last_p, incrementing round\n");
                    old_token->round = 2;
                    old_token->rtr_len = 0;
                }
                //else: getting resend token from prev dude, resend my token
                printf("Resending token to %x...\n",next_addr.sin_addr.s_addr);
                sendto( ss, (char*)old_token, sizeof(token), 0 , (struct sockaddr*)&next_addr, sizeof(next_addr));
            }
            else if (dat->round == 2) {
                printf("Got round 2 token...\n");
                //token going around 2nd time, ring is set up and prepared for transmit
                if (my_id != 1) {
                    old_token->rtr_len = 0;
                    old_token->round = 2;
                }
                init_state = 2;
            }
        }
        else //got a data message, means token ring has been set up
            init_state = 2;
    } //end main if loop
    } //end main for loop
}

int transmit_phase() {

    for(;;) {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
        if (num > 0 && FD_ISSET( sr, &temp_mask) && recv_dbg(sr, mess_buf, MAX_PAYLOAD_LEN, 0) > 0 ) {
            initializer* type =  (initializer*)mess_buf;
            if (type->type == TYPE_TOKEN) {
                token* dat = (token*)mess_buf;
                if (dat->round == old_token->round && my_id == 1)  //if round is complete, 1 needs to be able to increment the token
                    dat->round++;

                if (dat->round > old_token->round ) {
                    old_token->round = dat->round;
                    
                    //do aru and rtr stuff
                } 
                else
                    sendto( ss, (char*)old_token, sizeof(token), 0, (struct sockaddr*)&next_addr, sizeof(next_addr));
            }
            else if (type->type == TYPE_DATA) {
                data* dat = (data*)mess_buf;
                ca_set_max(dat->seq);
                ca_insert(dat->src_p, dat->seq,dat->rand); 
            }


        } //end main if 
    } //end for loop
}

