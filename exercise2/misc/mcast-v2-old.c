#include "net_include.h"
#include "packet.h"
#include "recv_dbg.h"

#include <arpa/inet.h>
#include <ifaddrs.h>

int main(int argc, char* argv[]) {

    struct sockaddr_in name;
    struct sockaddr_in send_addr;
    struct sockaddr_in next_addr;
    struct ifaddrs*    addrs;
    struct ifaddrs*    tmp;

    struct timeval     timeout;
    struct ip_mreq     mreq;
    unsigned char      ttl_val;

    int                ss,sr;
    fd_set             mask;
    fd_set             dummy_mask,temp_mask;
    int                bytes;
    int                num;
    char               mess_buf[MAX_PAYLOAD_LEN];
    char               init_state;
    int                my_id;
    int                my_ip;
    char               next_p;
    int                next_p_ip;
    int                total_packets;
    int                packets_sent;    
    int                seq;
    int                aru;
    int                last_tok_aru;
    int                last_tok_seq;
    int *              rtr_list;
    int                my_dec;

    init_state = 0;
    last_tok_seq = 0;
    last_tok_aru = 0;
    seq = 0;
    aru = 0;
    packets_sent = 0;
    my_dec = -1;

    /* command line args */
    if (argc < 5){
        printf("Useage: mcast <num_packets> <machine_index> <number of machines> <loss rate>\n");
        return 0;
    }
    total_packets = atoi(argv[1]);
    my_id = atoi(argv[2]);
    if (my_id == atoi(argv[3]))
        next_p = 1; 
    else 
        next_p = my_id + 1; 
    recv_dbg_init(atoi(argv[4]), my_id);

    /* Get my IP address */
    getifaddrs(&addrs);
    tmp = addrs;
    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET && strcmp(tmp->ifa_name,"eth0") == 0){
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            printf("My IP (%s): [%x] <%s>\n", tmp->ifa_name, pAddr->sin_addr, inet_ntoa(pAddr->sin_addr));
            my_ip = (int)pAddr->sin_addr.s_addr;
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
    if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val, 
        sizeof(ttl_val)) < 0) 
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d - ignore in WinNT or Win95\n", ttl_val );
    }

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(IPADDR);  /* mcast address */
    send_addr.sin_port = htons(PORT);

    next_addr.sin_family = AF_INET;
    next_addr.sin_addr.s_addr = next_p_ip;
    next_addr.sin_port = htons(PORT);

    /* set up file descriptor masks */
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );

    timeout.tv_sec = 0;
    timeout.tv_usec = 1000 * atoi(argv[3]);                                     //wait n milliseconds before trying to resend a token

    printf("Waiting for mcast_start...\n");
    for(;;)
    {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
               if (init_state == 0) {
                    recv(sr, mess_buf, MAX_PAYLOAD_LEN, 0);
                    if (mess_buf[0] == 1){                                      //type is init packet
                        init_state = 1;
                    }                    

                    /* send out my ip address */
                    initializer * packet = malloc(sizeof(initializer));
                    packet->type = 1;
                    packet->src_p = my_id;
                    packet->src_ip = my_ip; 
                    for ( int i = 0; i < 3; i++ )                               //increase chance of successful delivery
                        sendto(ss, (char*)packet, sizeof(packet), 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
                    free(packet);
                }
                else if (init_state == 1) {
                    bytes = recv_dbg(sr, mess_buf, MAX_PAYLOAD_LEN, 0);
                    if( bytes > 0) {
                        initializer * tmp = (initializer*)mess_buf;
                        if(tmp->src_p == next_p){
                            next_p_ip = tmp->src_ip; 
                            next_addr.sin_addr.s_addr = next_p_ip;
                            init_state = 2;

                            if(my_id == 1) {                                    // if this is first process we need to start the token now
                                token * packet = malloc(sizeof(char) + 6*sizeof(int));
                                packet->type    = 2;
                                packet->src_p   = my_id;
                                packet->src_ip  = my_ip;
                                packet->seq     = 0;
                                packet->aru     = 0;
                                packet->fcc     = 100;
                                packet->rtr_len = 0; 

                                for ( int i = 0; i < 3; i++ )                   //increase chance of successful delivery
                                    sendto(ss, (char*)packet, sizeof(packet), 0, (struct sockaddr*)&next_addr, sizeof(next_addr));
                                free(packet);
                            }
                        }
                    }
                }
                else if (init_state == 2) {                                     //we have our next-process-ip, ready for send/receive
                    if ( recv_dbg(sr, mess_buf, MAX_PAYLOAD_LEN, 0) > 0 ) {
                        initializer * tmp = (initializer*)mess_buf;
                        if (tmp->type == 2) {                                   //I got a token
                            token * packet = (token*)mess_buf;
                            if (packet->seq >= seq) {
                                if ( my_dec == packet->aru ) {                  //check if I decremented the token.aru last round
                                    packet->aru = aru;
                                    my_dec = -1;
                                } 
                                if (aru < packet->aru) {                        //check if I need to decrement the token.aru this round
                                    packet->aru = aru;
                                    my_dec = aru;
                                }
                                else if (aru >= packet->aru) {                  // deliver the data in the queue up to packet.aru
                                    printf("Can deliver up to index %8d\n",packet->aru); 
                                }
                                /* UPDATE THE RTR LIST HERE */
                            
                                /* SEND NEW PACKETS HERE, STORE INTO QUEUE */

                                /* MAKE SURE LOCAL.ARU (AND TOKEN.ARU) IS UP-TO-DATE HERE */
                            }
                        }
                        else if (tmp->type == 3) {                              // I got a data packet
                            data * packet = (data*)mess_buf;
                            if (packet->seq > seq)
                                seq = packet->seq;
                            if (aru < packet->seq)
                                printf("%2d, %8d, %8d\n",packet->src_p, packet->seq, packet->rand); //enqueue(data->seq, data->src_p, data->rand);
                            // need function to update local.aru
                        }
                    }
                }
            }
        } else {
            if (init_state > 1) {                                               // haven't heard anything, need to resend token
                token * packet = malloc(sizeof(char) + 6*sizeof(int));
                packet->type    = 2;
                packet->src_p   = my_id;
                packet->src_ip  = my_ip;
                packet->seq     = seq;
                packet->aru     = aru;
                packet->fcc     = 100;
                packet->rtr_len = 0; 

                sendto(ss, (char*)packet, sizeof(packet), 0, (struct sockaddr*)&next_addr, sizeof(next_addr));
                free(packet);
            }
        }
    }

    return 0;
}
