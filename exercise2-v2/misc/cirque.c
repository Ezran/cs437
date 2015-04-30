#include "cirque.h"

int             start_ptr;
int             end_ptr;
int             size;
int             capacity;
cq_wrap**       array;

void init(int cap) {
    start_ptr = 0;
    end_ptr = 0;
    size = 0;
    capacity = cap;
    array = malloc(cap+1);
}

int enqueue(int seq, int src_id, int random_data) {
    if (is_in_queue(seq) == 1 || size == capacity)
        return -1;
    else {
        cq_wrap * tmp = malloc(sizeof(cq_wrap));
        tmp->seq = seq;
        tmp->src_id = src_id;
        tmp->random_data = random_data;

        end_ptr = (end_ptr + 1) % (capacity+1);
        array[end_ptr] = tmp;
        size++;
        return 1;
    }
    return -1;
}

cq_wrap* dequeue() {
    if (size > 0) {
        cq_wrap* ret = array[start_ptr];
        start_ptr = (start_ptr + 1) % (capacity + 1);
        size--;
        return ret;
    } 
    return NULL;
}

int is_in_queue(int find) {
    for (int i = 0; i < size; i++) {
        if (array[(start_ptr+i) % (capacity+1)]->seq == find)
            return 1;
    }
    return 0;
}

int get_size() {
    return size;
}
