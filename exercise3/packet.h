#ifndef PACKET_H
#define PACKET_H

typedef struct packet {
    int type;       //default is 1
    int process_index;
    int message_index;
    int random_data;
    char payload[1200];
} packet;

typedef struct done_sending {
    int type;       //default is 2
} done_sending;

#endif
