#include "string.h"
#include "room.h"

room_node* head;
int num_rooms;

int get_num_rooms(){
    return num_rooms;
}

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

room_node* get_head(){
    return head;
}

// params: room_name
int add_room(char* name){
    if (find_room(name) != NULL)
        return 0;
    
    room_node* insert = malloc(sizeof(room_node));
    strcpy(insert->name,name);
    insert->num_msgs = 0;
    insert->num_users = 0;
    
    //deal with empty room
    if (num_rooms == 0)
    {
        head = insert;
        insert->next = insert;
        insert->prev = insert;
    }
    else {  
        insert->next = head;
        insert->prev = NULL;
        head->prev = insert;

        head = insert;
    }

    num_rooms++;

    return 1;
}

//params: room_name
int rm_room(char* name){
    room_node* target = find_room(name);
    if (target == NULL)
        return 0;

    while (target->users != NULL) {  //free all users
        while (target->users->sp_ids != NULL) {  //free all ^ sp_ids
            sp_id_node* sprm = target->users->sp_ids;
            target->users->sp_ids = target->users->sp_ids->next;
            free(sprm);
        }
        user_node* urm = target->users;
        target->users = target->users->next;
        free(urm);
    }
    while (target->msgs != NULL) { //free all msgs
        while( target->msgs->likes != NULL) { //free all ^ likes
            like_node* lrm = target->msgs->likes;
            target->msgs->likes = target->msgs->likes->next;
            free(lrm);
        }
        msg_node* mrm = target->msgs;
        target->msgs = target->msgs->next;
        free(mrm);
    }
    //room is empty

    target->next->prev = target->prev;
    target->prev->next = target->next;
    free(target);
    num_rooms--;

    //deal with head
    if (num_rooms == 0) 
        head = NULL;

    return 1;
}

//params: room_name
room_node* find_room(char* name) {
    room_node* iter = head; 
    for (int i = 0;i < num_rooms;i++, iter = iter->next) {
        if (strcmp(iter->name, name) == 0)
            return iter;
    }
    return NULL;
}

//params: room_name
void print_room(char* name){
    room_node* target = find_room(name);
    if (target == NULL)
        return;

    printf("=== %s ===\n",name);
    print_users(target->users);
    print_msgs(target->msgs);
}

//------ user functions --------
//params: u_id, sp_id, room
int add_user(char* u_id,char* sp_id,char* name)
{
    room_node* q = find_room(name);
    if (q == NULL){
        add_room(name);
        q = find_room(name);
    }

    if (q->num_users > 0 && strcmp(q->users->u_id,u_id) == 0) {
        if (strcmp(q->users->sp_ids->sp_id,sp_id) == 0) //duplicate
            return 0;
        sp_id_node* sp_iter = q->users->sp_ids;
        for (; sp_iter->next != NULL; sp_iter = sp_iter->next) {
            if (strcmp(sp_iter->next->sp_id,sp_id) == 0) 
                return 0;
        }
        sp_id_node* insert = malloc(sizeof(sp_id_node));
        strcpy(insert->sp_id,sp_id);
        insert->next = NULL;
        insert->prev = sp_iter;
        sp_iter->next = insert;
        q->users->next->num_ids++;
        return 1;  
    }
    user_node* u_iter = q->users;
    for (; q->users != NULL && u_iter->next != NULL; u_iter = u_iter->next) {
        if (strcmp(u_id,u_iter->next->u_id) == 0) {
            if (strcmp(u_iter->next->sp_ids->sp_id,sp_id) == 0) //duplicate
                return 0;
            sp_id_node* sp_iter = u_iter->next->sp_ids;
            for (; sp_iter->next != NULL; sp_iter = sp_iter->next) {
                if (strcmp(sp_iter->next->sp_id,sp_id) == 0) 
                    return 0;
            }
            sp_id_node* insert = malloc(sizeof(sp_id_node));
            strcpy(insert->sp_id,sp_id);
            insert->next = NULL;
            insert->prev = sp_iter;
            sp_iter->next = insert;
            u_iter->next->num_ids++;
            return 1;  
        }
    }
    user_node* insert = malloc(sizeof(user_node));
    strcpy(insert->u_id,u_id);
    sp_id_node* sin = malloc(sizeof(sp_id_node));
    strcpy(sin->sp_id,sp_id);
    insert->sp_ids = sin;
    insert->num_ids = 1;

    if (q->num_users == 0)
        q->users = insert;
    else {
        insert->next = u_iter->next;
        u_iter->next = insert;
        insert->prev = u_iter;
    }
    q->num_users++;
    return 1;
}

//params: sp_id 
int rm_user(char* sp_id) {
    room_node* q = head;
    if (q == NULL)
        return 0;

    for (int i = 0; i < num_rooms; i++, q = q->next) {  //check every room
        //check if anyone in room
        if (q->num_users > 0) {
            
            for ( user_node* u_iter = q->users; u_iter != NULL; u_iter = u_iter->next) {
                //check if matching user needs to be removed
                int ret = rm_id(u_iter, sp_id);
                if (ret == 1 && u_iter->num_ids == 0) {
                    if (strcmp(u_iter->u_id, q->users->u_id) == 0)
                        q->users = u_iter->next;
                    if (u_iter->next != NULL)
                        u_iter->next->prev = u_iter->prev;
                    if (u_iter->prev != NULL)
                        u_iter->prev->next = u_iter->next;
                    free(u_iter);
                    q->num_users--;
                    return 1;
                } 
            }    
        }
    }
   return 0;
}

user_node* find_user(room_node* root,char* u_id) {
    for (user_node* iter = root->users; iter != NULL; iter = iter->next) {
        if (strcmp(iter->u_id,u_id) == 0)
            return iter;
    }
    return NULL;
}

void print_users(user_node* root) {
    printf("Users: ");
   for (user_node* i = root; i != NULL; i = i->next) {
        if (i->next == NULL)
            printf("%s ",i->u_id);
        else
            printf("%s, ",i->u_id);
    }
    printf("\n");
} 
//------ msg  functions --------
//params: msg_timestamp, u_id, room, mess
int add_msg(lamport* ts, char* u_id, char* room, char* mess){
    room_node* root = find_room(room);
    if (root == NULL) {
        add_room(room);
        root = find_room(room);
    }

    msg_node* insert = malloc(sizeof(msg_node));
    insert->timestamp = ts;
    strcpy(insert->mess,mess);
    strcpy(insert->u_id,u_id);
    insert->num_likes = 0;    

    //check if first in chatroom
    if (root->num_msgs == 0) {
        insert->prev = NULL;
        insert->next = NULL;
        root->msgs = insert;
        root->num_msgs++;
        return 1;
    }

    //check if oldest message
    if (cmp_timestamp(ts,root->msgs->timestamp) < 0) {
        insert->prev = NULL;
        insert->next = root->msgs;
        insert->next->prev = insert;
        root->msgs = insert;
        root->num_msgs++;
        return 1;
    }
    //check all other msg for where next->ts > insert
    msg_node* iter = root->msgs;
    for (; iter->next != NULL; iter = iter->next) {
        // next is newer than insert
        if (cmp_timestamp(ts,iter->next->timestamp) < 0) {
            insert->next = iter->next;
            insert->prev = iter;
            iter->next = insert;
            insert->next->prev = insert;
            root->num_msgs++;
            return 1;
        }
        else if (cmp_timestamp(ts,iter->next->timestamp) == 0)
            return 0;
    }
    //insert is newest
    insert->prev = iter;
    insert->next = NULL;
    iter->next = insert;
    root->num_msgs++;
    return 1;
}

//params; room, line number
lamport* get_timestamp_from_index(room_node* root,int index) {
    msg_node* ret = root->msgs;
    for (int i = 1; i < index; i++){
        ret = ret->next;
    }
    return ret->timestamp;
}

msg_node* find_msg(char* room, lamport* ts) {

    room_node* root = find_room(room);
    if (root == NULL) 
        return 0;

    for (msg_node* iter = root->msgs;iter != NULL; iter = iter->next) {
        if (cmp_timestamp(iter->timestamp,ts) == 0)
            return iter;
    }
    printf("no msg found\n");
    return NULL;
}

void print_msgs(msg_node* root) {
    int ind = 1;
    for(msg_node* i = root; i != NULL; ind++, i = i->next) {
        printf("%d. %s: %s ",ind, i->u_id, i->mess);
        if (i->num_likes > 0)
            printf("\t\tLikes: %d \n",i->num_likes);
        else
            printf("\n");
    }
}
//------ sp_id functions -------

int rm_id(user_node* root, char* sp_id) {

    sp_id_node* rm = root->sp_ids;
    // check first id
    if (strcmp(root->sp_ids->sp_id, sp_id) == 0) {
        if (root->num_ids == 1)
            root->sp_ids = NULL;
        else {
            root->sp_ids = rm->next;
            root->sp_ids->prev = NULL;
        }
        root->num_ids--;
        free(rm);
        return 1;
    }
    for (; rm != NULL;rm = rm->next) {
        if (strcmp(rm->sp_id, sp_id) == 0) {
            if (rm->next != NULL)  //not last node
                rm->next->prev = rm->prev;
            rm->prev->next = rm->next;
            root->num_ids--;
            free(rm);
            return 1;
        }
    }
    return 0;
}
//------ like functions --------
//params; STATE, u_id(of liker), timestamp, target-msg
int add_like(int state, char* u_id, lamport* ts, msg_node* root) {
    like_node* iter = find_like(root, ts);
    if (iter != NULL) //if duplicate exit
        return 0;

    iter = find_like_by_uname(root, u_id);
    if (iter == NULL) //if like/unlike isn't getting overridden
    {
        iter = malloc(sizeof(like_node));
        iter->state = state;
        iter->timestamp = ts;
        strcpy(iter->u_id, u_id);
        if (root->num_likes == 0){
            root->likes = iter;
            iter->next = NULL;
            iter->prev = NULL;
        }
        else {
            root->likes->prev = iter;
            iter->next = root->likes;
            root->likes = iter;
            iter->prev = NULL;
        }
        if (state == LIKE)
            root->num_likes++;
    }
    else { //if like/unlike getting overridden
        iter->state = state;
        free(iter->timestamp);
        iter->timestamp = ts;
        if (iter->state == LIKE && state == UNLIKE)
            root->num_likes--;
        else if (iter->state == UNLIKE && state == LIKE)
            root->num_likes++;
    }
    return 1;
} 

like_node* find_like_by_uname (msg_node* root, char* u_id) {

    like_node* iter = root->likes;
    for (; iter != NULL; iter = iter->next) {
        if (strcmp(u_id,iter->u_id) == 0)
            return iter;
    }
    return NULL;
}


like_node* find_like (msg_node* root, lamport* ts) {
    like_node* iter = root->likes;
    for (; iter != NULL; iter = iter->next) {
        if (cmp_timestamp(ts,iter->timestamp) == 0)
            return iter;
    }
    return NULL;
}

