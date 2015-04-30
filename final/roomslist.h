#ifndef ROOMS_LIST_H
#define ROOMS_LIST_H

#include "packet.h"

#define LIKE       0
#define UNLIKE     1

typedef struct like_node {
    int like_state;
    struct lamport* timestamp; 
    char u_id[MAX_NAME_LEN];

    struct like_node* next;
} like_node;

typedef struct msg_node {
    struct lamport* timestamp;
    char   mess[MAX_MESS_LEN];
    char   u_id[MAX_NAME_LEN];

    struct msg_node* next;
    struct like_node* like_list;
} msg_node;

typedef struct room_node {
    char name[MAX_ROOM_LEN];
    int  num_messages;
    
    struct msg_node* msg_list;
    struct room_node* next;
} room_node;
/*
 * Parameters: (timestamp1, timestamp2)
 * Returns: -1 if first is older, 0 if equal and 1 if second is older
 */
int cmp_timestamp(lamport*,lamport*); 

/*
 * Parameters: (msg_timestamp, author, message, room)
 * Returns: -1 on error, 0 if failure and 1 on success
 */
int add_message(lamport*,char*,char*,char*);

/*
 * Parameters: (like/unlike, like_timestamp, author, chatroom, msg_timestamp)
 * Returns: -1 error, 0 fail 1 success
 */
int add_like(int, lamport*,char*,char*,lamport*);

//for client: get msg timestamp from msg index
lamport* get_msg_stamp(int);
/*
 * Parameters: (room_name)
 * Returns: -1 error, 0 fail, 1 success
 */
int add_room(char*);

//helper func, ret room if exists else null
room_node* find_room(char*);
/*
 * Parameters: (room_name)
 * Returns: 
 */
void print_room(char*);

/*
 * Parameters: (room_name,client_private_group)
 * Returns:
 */
void send_history(char*,char*);

//sets the max num of messages to save
//Used by client struct
// set to -1 for infinite
void set_max_msgs(int);

#endif
