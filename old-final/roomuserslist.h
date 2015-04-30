#ifndef ROOM_USERS_LIST_H
#define ROOM_USERS_LIST_H

#include "packet.h"

// node struct to hold a list of users
typedef struct user_node {
    char u_id[MAX_NAME_LEN];
    char spread_id[MAX_NAME_LEN];
    struct user_node* next;
} user_node;

// node struct to hold a list of rooms + what users in rooms
typedef struct room_users_node {
    char room[MAX_ROOM_LEN];
    struct room_users_node* next;
    struct user_node* users;
} room_users_node;

room_users_node* get_head();

//return 1 if successful, 0 if collision, -1 if error
int add_user(char*,char*,char*);

//return 1 if successful, 0 if no user, -1 if error
int rm_user(char*);

//usd by client to destroy old room participation
int clear_room(char*);

//return 1 if user in a room, 0 if user is not
int user_in_room(char*,char*);

void print_users(char*);
#endif
