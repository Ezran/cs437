#ifndef PACKET_H
#define PACKET_H

#define MAX_PAYLOAD_LEN 1400

/* this is sent by mcast_start,
 * and during ring setup.
   type is 1 */
typedef struct initializer {
	
	int type;
        int  src_p;
        int  src_ip;

} initializer;

/* type is 2 */
typedef struct token { 

	int type;
	int src_p;
        int src_ip;
        int round;
        int seq;
	int aru;
        int dec_aru_src;
	int rtr_len;
	int rtr[255];

} token;

/* type is 3 */
typedef struct data {

	int type;
        int src_p;
	int seq;
	int rand;
	char payload[1200];

} data;

#endif
