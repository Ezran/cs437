#ifndef ROOM_H
#define ROOM_h

#include "packet.h"

#define LIKE       0
#define UNLIKE     1

typedef struct room_node {
    char name[MAX_ROOM_LEN];
    int  num_msgs;
    int  num_users;    
    struct msg_node*  msgs;
    struct user_node* users;
    
    struct room_node* next;
    struct room_node* prev;
} room_node;

typedef struct user_node {
    char u_id[MAX_NAME_LEN];
    struct sp_id_node* sp_ids;
    int num_ids;
    
    struct user_node* next;
    struct user_node* prev;
} user_node;

typedef struct msg_node {
    struct lamport* timestamp;
    char   mess[MAX_MESS_LEN];
    char   u_id[MAX_NAME_LEN];
    int    num_likes;
    struct like_node* likes;

    struct msg_node* next;
    struct msg_node* prev;
} msg_node;

typedef struct sp_id_node {
    char sp_id[MAX_NAME_LEN];

    struct sp_id_node* next;
    struct sp_id_node* prev;
} sp_id_node;

typedef struct like_node {
    int state;
    struct lamport* timestamp; 
    char u_id[MAX_NAME_LEN];

    struct like_node* next;
    struct like_node* prev;
} like_node;


// Params: (timestamp1, timestamp2)
// Returns: -1 if first is older, 0 if equal and 1 if second is older
int cmp_timestamp(lamport*,lamport*); 

int get_num_rooms();
//------ room functions ---------
room_node* get_head();

// params: room_name
int add_room(char*);

//params: room_name
int rm_room(char*);

//params: room_name
room_node* find_room(char*);

//params: room_name
void print_room(char*);

//------ user functions --------
//params: u_id, sp_id, room
int add_user(char*,char*,char*);

user_node* find_user(room_node*,char*);

//params: sp_id 
int rm_user(char*);


void print_users(user_node*);
//------ msg  functions --------
//params: msg_timestamp, u_id, room, mess
int add_msg(lamport*,char*, char*, char*);

//params:  room, msg-timetamp
msg_node* find_msg(char*,lamport*);

//params; room, line number
lamport* get_timestamp_from_index(room_node*,int);

void print_msgs(msg_node*);
//------ sp_id functions -------

//params; target-u_id
int rm_id(user_node*, char*);

//------ like functions --------
//params; STATE, timestamp, target-msg
int add_like(int, char*, lamport*, msg_node*); 
 
like_node* find_like(msg_node*, lamport*);

like_node* find_like_by_uname(msg_node*, char*);
#endif
