#include "roomuserslist.h"

room_users_node* ruser_head;

room_users_node* get_head() {
    return ruser_head;
}

int add_user(char* u_id,char* sp_id,char* room)
{
    if (strlen(u_id) == 0 || strlen(room) == 0 || strlen(sp_id) == 0)
        return -1;
    if (user_in_room(u_id,room) == 1){
        int ret = rm_user(sp_id);
        if (ret == -1)
            return -1;
    }
    room_users_node* room_iter;

    for (room_iter = ruser_head; room_iter != NULL; room_iter = room_iter->next) {
        if (strncmp(room_iter->room,room,MAX_ROOM_LEN) == 0) {          //find the matching room

            user_node* newnode = malloc(sizeof(user_node));    //insert new user into chatroom
            strncpy(newnode->u_id, u_id, MAX_NAME_LEN);
            strncpy(newnode->spread_id,sp_id,MAX_NAME_LEN);
            if (room_iter->users == NULL)
                room_iter->users = newnode;
            else{
                newnode->next = room_iter->users;
                room_iter->users = newnode->next;
            }   
            return 1;
        }
    } 
    //room DNE, make it
    room_users_node* newroom = malloc(sizeof(room_users_node));
    strncpy(newroom->room,room,MAX_ROOM_LEN);
    newroom->next = ruser_head;
    ruser_head = newroom;

    newroom->users = malloc(sizeof(user_node));
    strncpy(newroom->users->u_id,u_id,MAX_NAME_LEN);
    strncpy(newroom->users->spread_id,sp_id,MAX_NAME_LEN);
    newroom->users->next = NULL;
    return 1;
}

int rm_user(char* sp_id)
{
    room_users_node*    room_iter;
    user_node*          user_iter;

    if (strlen(sp_id) == 0)
        return -1;

    for (room_iter = ruser_head; room_iter != NULL; room_iter = room_iter->next) {
        if (room_iter->users == NULL)  //if nobody in room
        { //do nothing, go to next room 
        } else if (strncmp(room_iter->users->spread_id,sp_id,MAX_NAME_LEN) == 0) {  //else if first person in room is target
            user_node* rm = room_iter->users;
            room_iter->users = rm->next;
            free(rm);
            return 1;
        }
        else {  //else check all other nodes
            for (user_iter = room_iter->users; user_iter->next != NULL; user_iter = user_iter->next) {
                if (strncmp(user_iter->next->spread_id,sp_id,MAX_NAME_LEN) == 0) {
                    user_node* rm = user_iter->next;
                    user_iter->next = rm->next;
                    free(rm);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int user_in_room(char* u_id, char* room)
{
    room_users_node*    room_iter;
    user_node*          user_iter;

    if (strlen(u_id) == 0 || strlen(room) == 0)
        return -1;

    for (room_iter = ruser_head; room_iter != NULL; room_iter = room_iter->next) {
        if (strncmp(room_iter->room,room,MAX_ROOM_LEN) == 0)
            break;
    }
    if (room_iter == NULL)
        return 0;

    for (user_iter = room_iter->users; user_iter != NULL; user_iter = user_iter->next) {
        if (strncmp(user_iter->u_id,u_id,MAX_NAME_LEN) == 0)
            return 1;
    }
    return 0;
}

int clear_room(char* room) {
    if(strncmp(ruser_head->room, room, MAX_ROOM_LEN) != 0){
        printf("Room name mismatch\n");
        return 0;
    }

    user_node* user_iter = ruser_head->users;
    user_node* rm;
    while (user_iter != NULL) {
        rm = user_iter;
        user_iter = user_iter->next;
        free(rm);
    }

    room_users_node* rrm = ruser_head;
    ruser_head = ruser_head->next;
    free(rrm);
    return 1;
}

void print_users(char* room) {

    room_users_node* room_iter;
    user_node*       user_iter;

    for (room_iter = ruser_head; room_iter != NULL; room_iter = room_iter->next) {
        if (strncmp(room_iter->room,room,MAX_ROOM_LEN) == 0) {
            break;
        }
    }
    if (room_iter == NULL){
        printf("Print error: No such room"); return;}

    printf("Users: ");
    for (user_iter = room_iter->users; user_iter != NULL; user_iter = user_iter->next) {
        if (user_iter->next != NULL)
            printf("%s, ",user_iter->u_id);
        else
            printf("%s \n",user_iter->u_id); 
    }
}
