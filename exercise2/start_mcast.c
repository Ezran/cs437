#include "net_include.h"
#include "packet.h"

int main()
{
    struct sockaddr_in send_addr;
    initializer*       packet_begin;

    struct ip_mreq     mreq;
    unsigned char      ttl_val;

    int                ss;

    mreq.imr_multiaddr.s_addr = htonl( IPADDR );

    /* the interface could be changed to a specific interface if needed */
    mreq.imr_interface.s_addr = htonl( INADDR_ANY );

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

    packet_begin = malloc(sizeof(initializer));
    packet_begin->type = 1;
    packet_begin->src_p = 0;
    packet_begin->src_ip = 0; 

    printf("sending...");
    sendto( ss, (char*)packet_begin, sizeof(initializer), 0, (struct sockaddr *)&send_addr, sizeof(send_addr) );

    printf("done [%d,%d,%d]\n",packet_begin->type,packet_begin->src_p,packet_begin->src_ip);
    free(packet_begin);
    return 0;
}

