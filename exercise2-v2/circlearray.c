#include "circlearray.h"

int length = 0;
int min_index = 0;
int ptr = 1; //holds index of 0th element
pod* array[MAX_ARRAY_SIZE] = {NULL};


//machine_index, packet_index, random
int ca_insert(int machine_index,int packet_index,int random){
    //check if the lowest insert slot is null, or the packet is within range
    if (array[ptr] == NULL && packet_index % MAX_ARRAY_SIZE == ptr){
        //inserting into lowest-index & is null
        pod* insert = malloc(sizeof(pod));
        insert->machine_index = machine_index;
        insert->packet_index = packet_index;
        insert->random = random;
        array[ptr] = insert;

        return 1;   

    }
    else if (MAX_ARRAY_SIZE + min_index >= packet_index && min_index < packet_index) {
        int insert_ind = packet_index % MAX_ARRAY_SIZE;
        if (array[insert_ind] == NULL){
            pod* insert = malloc(sizeof(pod));
            insert->machine_index = machine_index;
            insert->packet_index = packet_index;
            insert->random = random;
            array[insert_ind] = insert;

            if (min_index + length < packet_index)
                length = packet_index - min_index;
            return 1;   
        }
    } 
    return 0;
}

//pop oldest packet off, cannot pop if no packet
pod* ca_remove() {
    if (length > 0 && array[ptr] != NULL) {
        pod* ret = array[ptr];
        array[ptr] = NULL;
        ptr = (ptr + 1) % MAX_ARRAY_SIZE;
        min_index = ret->packet_index;
        length--;
        return ret;
    }
    return NULL;
}

pod* ca_get_index(int packet_index) {
    if (length > 0 && MAX_ARRAY_SIZE + array[ptr]->packet_index > packet_index) 
        return array[packet_index % MAX_ARRAY_SIZE];
    printf("index out of range");
    return NULL;
}

//return packet_index of lowest stored
int ca_get_min() {
    return min_index;
}

//return packet_index of highest stored
int ca_get_max() {
    return min_index + length;
}

//sets the length to m - min if < 1000, else sets length to 1000
//req'd in case where recv token with much higher seq than current max
int ca_set_max(int m) {
    if (m < min_index + length) //set max to lower than max
        return 0;
    if (m - min_index < MAX_ARRAY_SIZE)
        length = m - min_index;
    else
        length = MAX_ARRAY_SIZE;
    return 1;
}


//return size
int ca_get_size() {
    return length;
}

//returns a pointer to int array of missing packet_index, where size is written in passed argument
int* ca_get_missing(int* retsize) {
    int rs = 0;
   static int ret[MAX_ARRAY_SIZE];
    for (int i = 0; i < length; i++) {
        if (array[(ptr+i) % MAX_ARRAY_SIZE] == NULL) {
            ret[rs] = min_index + i + 1;
            rs++;
        }
    }
    *retsize = rs;
    return ret;
}
