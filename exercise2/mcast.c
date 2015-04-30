#include "net_include.h"
#include "packet.h"
#include "recv_dbg.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <time.h>

#define EMPTY_TOKEN_SIZE 8*sizeof(int)
#define FLOW_CONTROL     50
#define CQ_CAP           1000

int main(int argc, char* argv[]) {

    typedef struct cq_wrap {
        int seq;
        int src_p;
        int random_data;
    } cq_wrap;

    cq_wrap*           circle_array[CQ_CAP];
    int                start_ptr;                
    int                cq_len;

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

    srand(time(NULL));
    /* command line args */
    if (argc < 5){
        printf("Useage: mcast <num_packets> <machine_index> <number of machines> <loss rate>\n");
        return 0;
    }
    total_packets = atoi(argv[1]);
    packets_to_send = total_packets;
    my_id = atoi(argv[2]);
    if (my_id == atoi(argv[3]))
        next_p = 1; 
    else 
        next_p = my_id + 1; 
    recv_dbg_init(atoi(argv[4]), my_id);

    /* Initialize variables */
    seq = -1;
    aru = 0;
    saved_dec_aru = -1;
    ip_list = malloc(sizeof(int)*atoi(argv[3]));
    timeout = NULL;
    start_ptr = 0;
    cq_len = 0;

    strcat(f_name,argv[2]);
    strcat(f_name,".out");
    printf("Writing to %s\n",f_name);
    fp = fopen(f_name, "w");

    for (int i = 0; i < CQ_CAP; i++)
        circle_array[i] = 0;

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

    /* receiving socket */
    sr = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sr<0) {
        perror("Mcast: socket");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( sr, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Mcast: bind");
        exit(1);
    }

    mreq.imr_multiaddr.s_addr = htonl( IPADDR );
    mreq.imr_interface.s_addr = htonl( INADDR_ANY );

    if (setsockopt(sr, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, 
        sizeof(mreq)) < 0) 
    {
        perror("Mcast: problem in setsockopt to join multicast address" );
    }

    /* sending socket */
    ss = socket(AF_INET, SOCK_DGRAM, 0); 
    if (ss<0) {
        perror("Mcast: socket");
        exit(1);
    }

    ttl_val = 1;
    loop = 0;
    if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0)
        printf("Mcast: problem in setsockopt of multicast loopback\n");

    if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val, 
        sizeof(ttl_val)) < 0) 
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d - ignore in WinNT or Win95\n", ttl_val );
    }

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(IPADDR);  /* mcast address */
    send_addr.sin_port = htons(PORT);

    next_addr.sin_family = AF_INET;
    next_addr.sin_addr.s_addr = ip_list[next_p-1];
    next_addr.sin_port = htons(PORT);

    /* set up file descriptor masks */
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );


    printf("Waiting for start_mcast...\n");

    /* Init Phase */
    int init_state = 0;
    while(init_state < 2)
    {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, timeout);
        if (num > 0 && FD_ISSET( sr, &temp_mask)) {
            if (init_state == 0) {
                recv(sr, mess_buf, MAX_PAYLOAD_LEN, 0);
                memcpy(&num,mess_buf,sizeof(int));
                if (num == 1) {                                      //type is init packet
                    printf("Received start_mcast\n");
                    init_state = 1;
                    timeout = malloc(sizeof(struct timeval));
                    timeout->tv_sec = 0;
                    timeout->tv_usec = 10000;
                    if (my_id == 1) {                                       // process 1 creates token         
                        token * tok = malloc(sizeof(token));
                        tok->type = 2;
                        tok->src_p = 1;
                        tok->src_ip = ip_list[my_id-1];
                        tok->round = 1;
                        tok->seq = 0;
                        tok->aru = 0;
                        tok->rtr_len = atoi(argv[3]);
                        memcpy(tok->rtr, ip_list, atoi(argv[3]));

                        old_token = tok;
                        seq = 0; 
                    } 
                }
            }
            else if (init_state == 1) {
                bytes = recv_dbg(sr, mess_buf, MAX_PAYLOAD_LEN, 0);
                if (bytes > 0) {
                    memcpy(&num,mess_buf, sizeof(int));
                    if (num == 1){
                        initializer * packet = (initializer*)mess_buf;
                        if (packet->src_p != my_id) {
                            ip_list[packet->src_p - 1] = packet->src_ip;
                            if (packet->src_p == next_p)
                                next_addr.sin_addr.s_addr = ip_list[next_p-1];
                        }
                    }
                    else if(num == 2) {
                        token * packet = (token*)mess_buf;
                        for (int i = 0; i < packet->rtr_len; i++) {
                                if (ip_list[i] == 0 && packet->rtr[i] > 0) {
                                    printf("Adding %x to my ip_list\n",packet->rtr[i]);
                                    ip_list[i] = packet->rtr[i];
                                    if (i == next_p-1)
                                        next_addr.sin_addr.s_addr = ip_list[next_p-1];
                                }
                        }                            

                        if (packet->round == 1) { 
                            if (my_id == 1) {
                                free(old_token);
                                old_token = malloc(EMPTY_TOKEN_SIZE);
                                old_token->type = 2;
                                old_token->src_p = my_id;
                                old_token->src_ip = ip_list[my_id-1];
                                old_token->round = 2;
                                old_token->seq = 0;
                                old_token->aru = 0;
                                old_token->dec_aru_src = 0;
                                old_token->rtr_len = 0;
                            }
                            else if (seq == -1){ 
                                old_token = malloc(sizeof(token));
                                memcpy(old_token, packet, EMPTY_TOKEN_SIZE + packet->rtr_len * sizeof(int));
                                old_token->src_p = my_id;
                                old_token->src_ip = ip_list[my_id-1];
                                memcpy(old_token->rtr, ip_list, atoi(argv[3])*sizeof(int));
                                seq = 0;
                            }
                            else {
                                memcpy(old_token->rtr, ip_list, old_token->rtr_len * sizeof(int));
                                sendto( ss, (char*)old_token, EMPTY_TOKEN_SIZE + old_token->rtr_len*sizeof(int), 0 , (struct sockaddr*)&next_addr, sizeof(next_addr));
                            }   
                        }
                        else if ( old_token->round == 2 || packet->round == 2) {
                            init_state = 2;
                            if (my_id > 1 && old_token->round < packet->round) {
                                free(old_token);
                                old_token = malloc(sizeof(token));
                                memcpy(old_token, packet, EMPTY_TOKEN_SIZE);
                                old_token->src_p = my_id;
                                old_token->src_ip = ip_list[my_id-1];
                            }
                            sendto( ss, (char*)old_token, EMPTY_TOKEN_SIZE + sizeof(int)*old_token->rtr_len, 0 , (struct sockaddr*)&next_addr, sizeof(next_addr));
                        }
                    } 
                }
            }           
        }
        else {
            if (init_state == 1) {
                // if i haven't seen token, send my stuff multicast
                if (seq == -1) {
                    initializer* packet = malloc(sizeof(initializer));
                    packet->type = 1;
                    packet->src_p = my_id;
                    packet->src_ip = ip_list[my_id-1];

                    sendto( ss, (char*)packet, sizeof(initializer), 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
                    free(packet); 
                }
                // if i have seen token, AND i have next guy's IP resend the token
                else if (seq == 0 && ip_list[next_p-1] > 0) {
                    memcpy(old_token->rtr, ip_list, old_token->rtr_len * sizeof(int));
                    bytes = sendto( ss, (char*)old_token, EMPTY_TOKEN_SIZE + old_token->rtr_len*sizeof(int), 0 , (struct sockaddr*)&next_addr, sizeof(next_addr));
               } 
                // else do nothing, have no next_ip
            }
        }
    }

    /* Begin Transmit Phase */
    for(;;) {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, timeout);
        if (num > 0 && FD_ISSET( sr, &temp_mask)) {
            if( recv_dbg(sr, mess_buf, MAX_PAYLOAD_LEN, 0) > 0 ) {
                memcpy(&num,mess_buf, sizeof(int));
                if (num == 2) {                                         //get a token
                    token* packet = (token*)mess_buf;
                    old_token->type = 2;

                    int min_aru;
                    if (packet->aru < old_token->aru)                              //generate min aru for writing
                        min_aru = packet->aru;
                    else
                        min_aru = old_token->aru;

                    if ( packet->seq >= seq ) {                         //check if token is new
                        printf("got new token. local seq %d token seq %d\n",seq,packet->seq);
                        seq = packet->seq;
                        if (my_id == 1)                                 // increment round
                            old_token->round++;
                        if ( aru < packet->aru || packet->dec_aru_src == my_id ) {     // raise token aru if i decremented it OR lower it if i'm the farthest behind
                            printf("Lower ARU, I am behind (local %d token %d)\n",aru,packet->aru);
                            old_token->aru = aru;
                            old_token->dec_aru_src = my_id;
                        }

                        int *new_rtr = malloc(sizeof(int)*255);
                        memset(new_rtr,0,255);

                        int new_rtr_ind = 0;
                        for (int i = 0; i < packet->rtr_len; i++) {     //retransmit any on rtr list
                            cq_wrap* cq_tmp = circle_array[packet->rtr[i] % CQ_CAP];
                            if (cq_tmp == 0 && new_rtr_ind < 255){ 
                                new_rtr[new_rtr_ind] = packet->rtr[i];
                                new_rtr_ind++;   
                            }
                            else {
                                data* send = malloc(sizeof(data));
                                send->type = 3;
                                send->src_p = cq_tmp->src_p;
                                send->seq = cq_tmp->seq;
                                send->rand = cq_tmp->random_data;
        
                                sendto( ss, (char*)send, sizeof(data), 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
                                free(send);
                            }        
                        }

                        for (int i = start_ptr; i <= start_ptr + cq_len; i++) {    //find my rtr orders
                            int flag = 0;
                            if(new_rtr_ind < 255 && circle_array[i % CQ_CAP] == NULL) {
                                for(int j = 0; j < new_rtr_ind; j++) {
                                    if (new_rtr[j] == i % CQ_CAP){
                                        flag = 1;
                                        break;
                                    }
                                }
                                if (flag == 0) {
                                    new_rtr[new_rtr_ind] = i;
                                    new_rtr_ind++;
                                    printf(".Don't have %d, adding to rtr\n",i);
                                }
                            }
                        }
                        memcpy(old_token->rtr, new_rtr, sizeof(int)*new_rtr_ind);
                        old_token->rtr_len = new_rtr_ind;
                        free(new_rtr);

                        for (int i = 0; packets_to_send > 0 && i < FLOW_CONTROL; i++ && packets_to_send--) {                //send new packets if available                    
                            printf("My seq %d aru %d before sending new packet\n",seq,aru);
                            if (aru == seq) {
                                old_token->dec_aru_src = -1;
                                aru++;
                            }         
                            seq++;

                            data * send = malloc(sizeof(data));
                            send->type = 3;
                            send->src_p = my_id;
                            send->seq = seq;
                            send->rand = rand() % 1000000 + 1;
                            
                            cq_wrap* save = malloc(sizeof(int)*3);
                            save->seq = send->seq;
                            save->src_p = send->src_p;
                            save->random_data = send->rand;

                            circle_array[(start_ptr + cq_len) % CQ_CAP] = save;
                            cq_len++;

                            old_token->seq = seq;
                            if (old_token->dec_aru_src == -1)
                                old_token->aru = aru;          

                            printf("Sending new data: %d <%d>, my info: seq %d aru %d start_ptr %d cq_len %d\n",send->seq,send->rand,seq,aru,start_ptr,cq_len);
                            sendto( ss, (char*)send, sizeof(data), 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
                        }
                        old_token->src_p = my_id;
                        old_token->src_ip = ip_list[my_id-1];
                                                                                // pass token
                        sendto(ss, (char*)old_token, EMPTY_TOKEN_SIZE + sizeof(int)*old_token->rtr_len, 0 , (struct sockaddr*)&next_addr, sizeof(next_addr));

                        while ( cq_len > 0 && start_ptr < min_aru ) {                   // write 
                            cq_wrap* tmp = circle_array[start_ptr % CQ_CAP];
                            printf("writing %2d, %8d, %8d\n",tmp->src_p, tmp->seq, tmp->random_data);
                            fprintf(fp, "%2d, %8d, %8d\n",tmp->src_p, tmp->seq, tmp->random_data);
                            free(tmp);
                            start_ptr++;
                            cq_len--;
                        }
                    }    
                }
                else if (num == 3) { //got a data
                    data* packet = (data*)mess_buf;
                    if ( packet->seq > seq ) {
                        if (cq_len == CQ_CAP)                                   //if array is full, drop packet
                            break;  
                        seq = packet->seq;                                      // else if packet seq > local seq 
                    }
                    if ( aru < packet->seq && packet->seq < start_ptr + CQ_CAP && circle_array[(packet->seq-1) % CQ_CAP] == NULL) {    // don't have this packet AND this packet isn't going to overwrite active data
                        printf("got valid data: seq %d, start_ptr %d, cq_len %d\n",packet->seq,start_ptr,cq_len);
                        cq_wrap* save = malloc(sizeof(int)*3);
                        save->seq = packet->seq;
                        save->src_p = packet->src_p;
                        save->random_data = packet->rand;
                        
                        circle_array[(packet->seq-1) % CQ_CAP] = save;
                        cq_len = seq - start_ptr;
                    }
                }
            }
        }
        else {
            sendto( ss, (char*)old_token, EMPTY_TOKEN_SIZE + sizeof(int)*old_token->rtr_len, 0, (struct sockaddr*)&next_addr, sizeof(next_addr)); //token got lost, resend
        }
    }
    /* End Transmission Phase */


    return 0;
}
