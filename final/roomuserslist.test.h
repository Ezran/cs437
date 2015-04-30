#ifndef ROOM_USER_LIST_H
#define ROOM_USER_LIST_H

#include "packet.h"

typedef struct sp_id_node {
    char sp_id[MAX_NAME_LEN];

    struct sp_id_node* next;
} sp_id_node;

typedef struct user_node {
    char u_id[MAX_NAME_LEN];
    sp_id_node* sp_ids;
    int num_sp_ids;
    
    struct user_node* next;
} user_node;

// node struct to hold a list of rooms + what users in rooms
typedef struct u_room_node {
    char name[MAX_ROOM_LEN];
    int  num_users;    

    struct u_room_node* next;
    struct user_node* users;
} u_room_node;

/*
 * 
 */
u_room_node* u_get_head();

/*
 * (room_name)
 */
int u_add_room(char*);

/*
 * (room_name)
 */
int u_rm_room(char*);

/*
 * (u_id, sp_id, room)
 */
int u_add_user(char*,char*,char*);

/*
 * (sp_id)
 */
int u_rm_user(char*);

/*
 * (room_name)
 */
void u_print_room(char*);

#endif
