#ifndef CIRCLE_ARRAY_H
#define CIRCLE_ARRAY_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_ARRAY_SIZE 1000

typedef struct pod {
    int machine_index;
    int packet_index;
    int random;
} pod;

//machine_index, packet_index, random
int ca_insert(int,int,int);

//pops oldest packet
pod* ca_remove();

pod* ca_get_index(int);

//return packet_index of lowest stored
int ca_get_min();

//return packet_index of highest stored
int ca_get_max();

int ca_set_max(int);

//return size
int ca_get_size();

//returns a pointer to int array of missing packet_index, where size is written in passed argument
int* ca_get_missing(int*);

#endif
