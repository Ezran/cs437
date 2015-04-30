#include "net_include.h"
#include "packet.h"
#include "recv_dbg.h"

#include <arpa/inet.h>
#include <ifaddrs.h>

#define MAX_TIMEOUT 100000

int main(int argc, char* argv[])
{
    struct sockaddr_in name;
    struct sockaddr_in send_addr;
    struct sockaddr_in next_addr;
    struct ifaddrs*    addrs;
    struct ifaddrs*    tmp;

    int                mcast_addr;

    struct ip_mreq     mreq;
    unsigned char      ttl_val;

    int                ss,sr;
    fd_set             mask;
    fd_set             zero_mask;
    fd_set             dummy_mask;
    int                bytes;
    int                num;
    char               mess_buf[MAX_PAYLOAD_LEN];
    char               init_state;
    char               my_id;
    int                my_ip;
    char               next_p;
    int                next_p_ip;
    int                timeout;
    int                total_packets;

    /* command line args */
    if (argc < 5){
        printf("Useage: mcast <num_packets> <machine_index> <number of machines> <loss rate>\n");
        exit(0);
    }
    total_packets = atoi(argv[1]);
    my_id = (char)atoi(argv[2]);
    if (my_id == atoi(argv[3]))
        next_p = 1;
    else
        next_p = my_id++;
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
    /*---------------------*/
/*
    struct in_addr* t = malloc(sizeof(struct in_addr));
    t->s_addr = htonl(IPADDR); 
    printf("Multicast IP: %s\n",inet_ntoa(*t));
    free(t);
*/
    init_state = 0;
    timeout = 0;    

    sr = socket(AF_INET, SOCK_DGRAM, 0); /* socket for receiving */
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

    /* the interface could be changed to a specific interface if needed */
    mreq.imr_interface.s_addr = htonl( INADDR_ANY );

    if (setsockopt(sr, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, 
        sizeof(mreq)) < 0) 
    {
        perror("Mcast: problem in setsockopt to join multicast address" );
    }

    ss = socket(AF_INET, SOCK_DGRAM, 0); /* Socket for sending */
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

    FD_ZERO( &dummy_mask );
    FD_ZERO( &zero_mask );
    FD_SET( sr, &mask );

    printf("Waiting for mcast_start...\n");
    for(;;)
    {
        printf("?\n");
        mask = zero_mask;
        num = select( FD_SETSIZE, &mask, &dummy_mask, &dummy_mask, NULL);
        printf("^\n");
        if (num > 0) {
            timeout = 0;
            printf("% \n");
            if ( FD_ISSET( sr, &mask) ) { /* received message */
                printf(" !!!!!! \n");
               if (init_state == 0) {
                    recv(sr, mess_buf, MAX_PAYLOAD_LEN, 0);
                    if (mess_buf[0] == 1) //type is init packet
                        init_state = 1;
                    
                    // now send out my ip address
                    initializer * packet = malloc(sizeof(initializer));
                    packet->type = 1;
                    packet->src_p = my_id;
                    packet->src_ip = my_ip; //already in network format
                    for ( int i = 0; i < 3; i++ ) //increase chance of successful delivery
                        sendto(ss, (char*)packet, sizeof(initializer), 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
                }
                else if (init_state == 1) {
                    bytes = recv_dbg(sr, mess_buf, MAX_PAYLOAD_LEN, 0);
                    if( bytes > 0) {
                        initializer * tmp = (initializer*)mess_buf;
                        if(tmp->src_p == next_p){
                            next_p_ip = tmp->src_ip; 
                            next_addr.sin_addr.s_addr = next_p_ip;
                            init_state = 2;

                            printf("Process %d <%s>\n",next_p,next_p_ip);
                            return 0;
                            if(my_id == 1) { // if this is first process we need to start the token now
/* 222222222222222222222222222222222222222222222 */
                            }
                        }
                    }
                }
                else if (init_state == 2) {
                    //do token ring stuff here
                }
            }
        }
	else { /* timeout */
            printf(".\n");
            if (init_state > 0) {
                timeout++;
                if (timeout >= MAX_TIMEOUT) {
                    printf("Timed out\n");
                    return 0;
                }
            }
	}
    }

    return 0;

}

