#include "net_include.h"

#ifndef PACKET_H
#define PACKET_H

typedef struct data {
    int type;           //type is 1
    int index;
    int payload_len;
    char payload[MAX_MESS_LEN - 3*sizeof(int)];
} data;

typedef struct ack {
    int type;           //type is 2
    int index;
} ack;

typedef struct finished {
    int type;           //type is 3
} finished;

#endif
