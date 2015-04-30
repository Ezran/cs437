#ifndef CIRQUE_H
#define CIRQUE_H

/* wrapper for data in circle queue */
typedef struct cq_wrap {

    int seq;
    int src_id;
    int random_data;

} cq_wrap;

/* set variables to 0, allocates queue size */
void init(int);

/* return 1 success, -1 fail */
int enqueue(cq_wrap*);

/* return wrapper on success, NULL on fail
   return value needs to be free'd */
cq_wrap* dequeue(void);

/* returns 1 if matching index is found, 0 if not */
int is_in_queue(int);

int get_size(void);

#endif
