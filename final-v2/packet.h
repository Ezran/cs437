#ifndef PACKET_H
#define PACKET_H

#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define SPREAD_NAME "10160"
#define SPREAD_MESS_LEN 1400

#define MAX_MESS_LEN 80
#define MAX_NAME_LEN 20
#define MAX_ROOM_LEN 34

#define TYPE_JOIN_CHATROOM   0
#define TYPE_SEND_MSG        1
#define TYPE_LIKE_MSG        2
#define TYPE_PARTITION_STATE 4
#define TYPE_LEAVE_CHATROOM  5

#define SOURCE_SERVER   0
#define SOURCE_CLIENT   1

    typedef struct lamport {
        int server_id;
        int index;
    } lamport;

    typedef struct typecheck {
        int type;
        int source;
    } typecheck;

    typedef struct join_chatroom {
        int type;
        int source;
        char u_id[MAX_NAME_LEN];
        char sp_id[MAX_NAME_LEN];
        char chatroom[MAX_ROOM_LEN];
    } join_chatroom;

    typedef struct leave_chatroom {
        int type;
        int source;
        char sp_id[MAX_NAME_LEN];
        char room[MAX_ROOM_LEN];
    } leave_chatroom;

    typedef struct message {
        int type;
        int source;
        struct lamport timestamp;
        char u_id[MAX_NAME_LEN];
        char chatroom[MAX_ROOM_LEN];
        char mess[MAX_MESS_LEN];
    } message;

    typedef struct like {
        int type;
        int source;
        struct lamport timestamp;
        struct lamport msg_timestamp;
        char u_id[MAX_NAME_LEN];
        char chatroom[MAX_ROOM_LEN];
        int like_state;         
    } like;

#endif
