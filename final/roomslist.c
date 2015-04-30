#include "roomslist.h"

room_node* roomlist_head = NULL;
int   num_rooms = 0;
int   max_messages = -1; //if > -1 will be enforces by add_msg

/*
 * Parameters: (timestamp1, timestamp2)
 * Returns: -1 if first is older, 0 if equal and 1 if second is older
 */
int cmp_timestamp(lamport* stamp1,lamport* stamp2){
    if (stamp1->index < stamp2->index)
        return -1;
    else if (stamp1->index > stamp2->index)
        return 1;
    else{ //stamps equal 
        if (stamp1->server_id < stamp2->server_id)
            return -1;
        else if (stamp1->server_id > stamp2->server_id)
            return 1;
        else
            return 0;
    }
    return -2;
} 

/*
 * Parameters: (msg_timestamp, author, chatroom, message)
 * Returns: -1 on error, 0 if failure and 1 on success
 */
int add_message(lamport* stamp,char* u_id,char* room, char* mess){
    if (strlen(u_id) == 0 || strlen(u_id) > MAX_NAME_LEN || strlen(mess) > MAX_MESS_LEN)
        return -1;
    if (max_messages == 0)
    {    printf("Max messages is zero ");return 0;}

    //find room
    room_node* room_iter = find_room(room);
    if (room_iter == NULL)      //room DNE
    {    printf("Room not found ");return 0;}

    if (room_iter->num_messages == 0) { //first message in room
        msg_node* newnode = malloc(sizeof(msg_node));
        newnode->timestamp = stamp;
        strncpy(newnode->mess,mess,MAX_MESS_LEN);
        strncpy(newnode->u_id,u_id,MAX_NAME_LEN);
        newnode->like_list = NULL;

        newnode->next = NULL;
        room_iter->msg_list = newnode;   
        room_iter->num_messages++;            
        return 1; 
    }

    msg_node* msg_iter;
    for (msg_iter = room_iter->msg_list; msg_iter->next != NULL; msg_iter = msg_iter->next) {
        if (cmp_timestamp(stamp,msg_iter->next->timestamp) < 0) { //insert is older than next message
            msg_node* newnode = malloc(sizeof(msg_node));
            newnode->timestamp = stamp;
            strncpy(newnode->mess,mess,MAX_MESS_LEN);
            strncpy(newnode->u_id,u_id,MAX_NAME_LEN);
            newnode->like_list = NULL;

            newnode->next = msg_iter->next;
            msg_iter->next = newnode;   
        
            //enforce max messages if necessary
            if (max_messages > 0 && room_iter->num_messages == max_messages) {
                msg_node* rm = room_iter->msg_list;
                room_iter->msg_list = room_iter->msg_list->next;

                free(rm->timestamp);
                like_node* like_rm = rm->like_list;
                while (like_rm != NULL) {
                    like_node* tmp = like_rm;
                    like_rm = like_rm->next; 
                    free(tmp->timestamp);
                    free(tmp);
                }
                free(rm);
            }else
                room_iter->num_messages++;            
          
            return 1; 
        }    
        else if (cmp_timestamp(stamp,msg_iter->next->timestamp) == 0) //d'oops-licate
        {    printf("duplicate found\n");return 0;}
    }

    msg_node* newnode = malloc(sizeof(msg_node));  //msg is newest, insert at end
    newnode->timestamp = stamp;
    strncpy(newnode->mess,mess,MAX_MESS_LEN);
    strncpy(newnode->u_id,u_id,MAX_NAME_LEN);
    newnode->like_list = NULL;

    newnode->next = NULL;
    msg_iter->next = newnode;    
    
    //enforce max_messages -- delete oldest one if full
    if (max_messages > 0 && room_iter->num_messages == max_messages) {
        msg_node* rm = room_iter->msg_list;
        room_iter->msg_list = room_iter->msg_list->next;

        free(rm->timestamp);
        like_node* like_rm = rm->like_list;
        while (like_rm != NULL) {
            like_node* tmp = like_rm;
            like_rm = like_rm->next; 
            free(tmp->timestamp);
            free(tmp);
        }
        free(rm);
    }else
        room_iter->num_messages++;            
    return 1; 
}
/*
 * Parameters: (like_timestamp, author, chatroom, msg_timestamp)
 * Returns: -1 error, 0 fail 1 success
 */
int add_like(int likestate,lamport* like_stamp,char* u_id,char* room,lamport* msg_stamp){
    if (strlen(u_id) == 0 || (likestate != UNLIKE && likestate != LIKE) )
        return -1;

    //find msg stamp first
    room_node* room_iter = find_room(room);
    if (room_iter == NULL)      //room DNE
       {printf("room DNE "); return 0;}

    msg_node* msg_iter;
    for (msg_iter = room_iter->msg_list; msg_iter != NULL; msg_iter = msg_iter->next) {
        if (cmp_timestamp(msg_iter->timestamp,msg_stamp) == 0)
            break;
    }
    if (msg_iter == NULL)
        return -1;

    //check if like/unlike exists
    like_node* like_iter;
    for (like_iter = msg_iter->like_list; like_iter != NULL; like_iter = like_iter->next) {
        if (strncmp(u_id,like_iter->u_id,MAX_NAME_LEN) == 0) { //match
            if (cmp_timestamp(like_stamp,like_iter->timestamp) < 0) //inserting older like update than exists
               { printf("Newer like state found \n"); return 0;}
            like_iter->like_state = likestate;
            free(like_iter->timestamp);         //overriding timestamp
            like_iter->timestamp = like_stamp;
            printf("Overriding like\n");
            return 1; 
        }
    }
    //no like/unlike found, make new like
    if (likestate == LIKE) {
        like_node* newlike = malloc(sizeof(like_node));
        newlike->like_state = LIKE;
        newlike->timestamp = like_stamp;
        strncpy(newlike->u_id,u_id,MAX_NAME_LEN);

        newlike->next = msg_iter->like_list;
        msg_iter->like_list = newlike;
        printf("Adding new like...\n");
        return 1;
    }
    printf("cannot unlike a message that wasn't liked "); return 0; //trying to unlike a message that wasn't liked
}

lamport* get_msg_stamp(int index) {
    if (roomlist_head == NULL || index < 1 || index > 25)
        return NULL;

    msg_node* ret = roomlist_head->msg_list;
    for (int i = 1; i < index; i++){
        ret = ret->next;
    }
    return ret->timestamp;
}

/*
 * Parameters: (room_name)
 * Returns: -1 error, 0 fail, 1 success
 */
int add_room(char* roomname){
    if (strlen(roomname) == 0 || strlen(roomname) > MAX_ROOM_LEN)
        return -1;

    if (num_rooms == 0) {
        roomlist_head = malloc(sizeof(room_node));
        strncpy(roomlist_head->name,roomname,MAX_ROOM_LEN);
        roomlist_head->num_messages = 0;
        roomlist_head->msg_list = NULL;
        roomlist_head->next = NULL;
        
        num_rooms++;
    }
    else {
        room_node* iter;        //search for matching room or end
        for(iter = roomlist_head; iter->next != NULL; iter = iter->next){
            if (strncmp(iter->next->name,roomname,MAX_ROOM_LEN) == 0)
                return 0;
        }
        //room doesn't exist, make it
        room_node* newnode = malloc(sizeof(room_node));
        strncpy(newnode->name,roomname,MAX_ROOM_LEN);
        newnode->num_messages = 0;
        newnode->msg_list = NULL;
        newnode->next = NULL;

        iter->next = newnode;
        num_rooms++;
    }
    return 1;
}

room_node* find_room(char* name) {
    room_node* room_iter;
    for (room_iter = roomlist_head; room_iter != NULL; room_iter = room_iter->next) {
        if (strncmp(room_iter->name,name,MAX_ROOM_LEN) == 0)
            break;
    }

    return room_iter;
}

/*
 * Prints the room to stdout, NOT ROOM NAME OR PARTICIPANTS 
 *
 * Parameters: (room_name)
 * Returns: 
 */
void print_room(char* room){
    msg_node* iter;
    int counter = 1;
    if (roomlist_head == NULL) {
        printf("## No messages in room\n");
        return;
    }
    for (iter = roomlist_head->msg_list; iter != NULL; iter = iter->next) {
        printf("%d. %s: %s",counter,iter->u_id,iter->mess);
        
        like_node* l_iter;
        int l_count = 0;
        for (l_iter = iter->like_list; l_iter != NULL; l_iter = l_iter->next) {
            if (l_iter->like_state == LIKE) 
                l_count++;
        }
        if (l_count > 0)
            printf("\t\tLikes: %d",l_count);
        counter++;
    }
}
/*
 * Sends the chat history as a string on spread to a target group
 *
 * Parameters: (room_name,client_private_group)
 * Returns:
 */
void send_history(char* room,char* group){

}

void set_max_msgs(int m) {
    if (m < 0)
        max_messages = -1;
    else
        max_messages = m;   
}
