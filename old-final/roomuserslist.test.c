#include "roomuserslist.h"

u_room_node* users_head;
int num_rooms;


/*
 * (room_name)
 */
int u_add_room(char* name){
    if (strlen(name) == 0)
        return -1;    

    u_room_node* iter = users_head;
    if (num_rooms > 0){ //at least 1 room
        if (strncmp(iter->name,name,MAX_NAME_LEN) != 0) { //not the first room
            while (iter->next != NULL){
                if (strncmp(iter->next->name,name,MAX_NAME_LEN) == 0)
                    return 0;
                iter = iter->next;
            }
            //iter is on last element, no matches
            u_room_node* insert = malloc(sizeof(u_room_node));
            strncpy(insert->name,name,MAX_NAME_LEN);
            insert->next = NULL;
            iter->next = insert;
            insert->num_users = 0;
            num_rooms++;
            return 1;
        }
        else  //is the first room
            return 0;
    }
    else { //iter = null, room list is empty
        u_room_node* insert = malloc(sizeof(u_room_node));
        strncpy(insert->name,name,MAX_NAME_LEN);
        insert->next = NULL;
        users_head = insert;
        insert->num_users = 0;
        num_rooms++;
        return 1;
    }
}

/*
 * (room_name)
 */
int u_rm_room(char* room){
    if (strlen(room) == 0)
        return -1;

    if (users_head == NULL)
        return 0;
    
    u_room_node* iter = users_head;
    if (strncmp(iter->name,room,MAX_ROOM_LEN) == 0) { //first node is target
        while(iter->users != NULL) {  //kill its users
            user_node* rm = iter->users;
            iter->users = iter->users->next;
            
            while(rm->sp_ids != NULL) { //kill its clients
                sp_id_node* sprm = rm->sp_ids;
                rm->sp_ids = sprm->next;
                free(sprm);
            }
            free(rm);
        }
        users_head = users_head->next; //kill it
        free(iter);
        num_rooms--;
        return 1;
    } else {
        while (iter->next != NULL) { //find target
            if (strncmp(iter->next->name,room,MAX_ROOM_LEN) == 0) { //iter->next is target
                while(iter->next->users != NULL) {  //kill its users
                    user_node* rm = iter->next->users;
                    iter->next->users = iter->next->users->next;

                    while(rm->sp_ids != NULL) { //kill its clients
                        sp_id_node* sprm = rm->sp_ids;
                        rm->sp_ids = sprm->next;
                        free(sprm);
                    }
                    free(rm);
                }
                u_room_node* rrm = iter->next;  //kill it
                iter->next = rrm->next;
                free(rrm);
                num_rooms--;
                return 1;
            }
            iter = iter->next;
        }
    }
    return 0; //room not found
}
/*
 * (u_id, sp_id, room)
 */
int u_add_user(char* u_id,char* sp_id,char* room){
    if (strlen(u_id) == 0 || strlen(sp_id) == 0 || strlen(room) == 0)
        return -1;

    if (num_rooms == 0)
        return 0;

    u_room_node* iter = users_head;  //find the room
    while (iter != NULL && strncmp(room,iter->name,MAX_ROOM_LEN) != 0)
        iter = iter->next;
    if (iter == NULL)
        return 0;
    
    //find the user if exists, if not create it
    user_node* u_iter = iter->users;  
    if (iter->num_users > 0 && strncmp(u_id,u_iter->u_id,MAX_NAME_LEN) == 0) { //first u_id match
        sp_id_node* sp_iter = u_iter->sp_ids;
        if (strncmp(sp_id,sp_iter->sp_id,MAX_NAME_LEN) == 0) //check first sp_id
            return 2; //duplicate insert
        while(sp_iter->next != NULL) { //check all other sp_id for dups
            if (strncmp(sp_iter->next->sp_id,sp_id,MAX_NAME_LEN) == 0)
                return 2;
            sp_iter = sp_iter->next;
        }
        //sp_iter @ end of list, insert new sp_id node
        sp_id_node* insert = malloc(sizeof(sp_id_node));
        strncpy(insert->sp_id,sp_id,MAX_NAME_LEN);
        sp_iter->next = insert;
        u_iter->num_sp_ids++;
        return 1;
    }
    while (iter->num_users > 0 && u_iter->next != NULL) { //check all other u_id for match
        if (strncmp(u_id,u_iter->next->u_id,MAX_NAME_LEN) == 0) { //u_id match
            sp_id_node* sp_iter = u_iter->next->sp_ids;
            if (strncmp(sp_id,sp_iter->sp_id,MAX_NAME_LEN) == 0) //check first sp_id
                return 2; //duplicate insert
            while(sp_iter->next != NULL) { //check all other sp_id for dups
                if (strncmp(sp_iter->next->sp_id,sp_id,MAX_NAME_LEN) == 0)
                    return 2;
                sp_iter = sp_iter->next;
            }
            //sp_iter @ end of list, insert new sp_id node
            sp_id_node* insert = malloc(sizeof(sp_id_node));
            strncpy(insert->sp_id,sp_id,MAX_NAME_LEN);
            sp_iter->next = insert;
            u_iter->next->num_sp_ids++;
            return 1;
        }
        u_iter = u_iter->next;
    }
    //no u_id match, create u_id node and insert
    user_node* u_insert = malloc(sizeof(user_node));
    sp_id_node* sp_insert = malloc(sizeof(sp_id_node));

    strncpy(u_insert->u_id,u_id,MAX_NAME_LEN);
    u_insert->num_sp_ids = 1;
    u_insert->sp_ids = sp_insert;

    strncpy(sp_insert->sp_id,sp_id,MAX_NAME_LEN);
    if (iter->num_users == 0)
        iter->users = u_insert;
    else
        u_iter->next = u_insert;

    iter->num_users++;

    return 1;
}

/*
 * (sp_id)
 */
int u_rm_user(char* sp_id){
    if (strlen(sp_id) == 0)
        return -1;
    if (num_rooms == 0)
        return 0;

    u_room_node* iter = users_head;
    u_room_node* iter_lag = users_head;    
    for (;iter != NULL;iter_lag = iter, iter = iter->next) {

        user_node* u_iter = iter->users;
        user_node* u_iter_lag = iter->users;
        for(; u_iter != NULL;u_iter_lag = u_iter, u_iter = u_iter->next) {

            sp_id_node* sp_iter = u_iter->sp_ids;
            sp_id_node* sp_iter_lag = u_iter->sp_ids;
            for(; sp_iter != NULL; sp_iter_lag = sp_iter, sp_iter = sp_iter->next) {
                if (strcmp(sp_iter->sp_id,sp_id) == 0) {
                    sp_iter_lag->next = sp_iter->next;
                    free(sp_iter);
                    u_iter->num_sp_ids--;
                    if (u_iter->num_sp_ids == 0)
                        break;
                    else
                        return 1; //can exit
                }
            } //must have found only sp_id, or none, check if no more sp_id
            if (u_iter->num_sp_ids == 0) {
                u_iter_lag->next = u_iter->next;
                free(u_iter);
                iter->num_users--;
                if (iter->num_users == 0)
                    break;
                else
                    return 1; //can exit
            }
        } //must have found only user, or none, check if no more users
        if (iter->num_users == 0) {
            iter_lag->next = iter->next;
            free(iter);
            num_rooms--;
            return 1;
        }
    } //got here means didn't find it
    return 0;
}
/*
 * (room_name)
 */
void u_print_room(char* room){
    u_room_node* iter = users_head;
    for (;iter != NULL; iter = iter->next) {
        if (strcmp(room,iter->name) == 0) {
            user_node* u_iter = iter->users;
            if (iter->users == NULL)
                printf(" <> ");
            printf("Users [%d]: ",iter->num_users);
            for (;u_iter != NULL; u_iter = u_iter->next){
                if (u_iter->next == NULL)
                    printf("%s \n",u_iter->u_id);
                else
                    printf("%s, ",u_iter->u_id);
            } 
        }
    }
}

u_room_node* u_get_head() {
    return users_head;
}
