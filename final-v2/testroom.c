#include "room.h"
#include "stdlib.h"

int main() {
    // test room insertion, deletion
    char* rooms[7] = {"room1", "room2", "room3", "room4", "room5", "room6", "room7"};
    for (int i = 0; i < 8; i++) {
        int op = rand() % 5;
        int name = rand() % 7;
        printf("Op %d room %d<%s>\n",op,name,rooms[name]);
        if (op == 0) 
            rm_room(rooms[name]);
        else
            add_room(rooms[name]);
    } 
    room_node* iter = get_head();
    printf("Rooms[%d]: ",get_num_rooms());
    for (int i = 0; i < get_num_rooms(); i++, iter = iter->next) {
        printf(" %s ",iter->name);
    }
    printf("\n\n\n");
    
    //test user insertion, removal
    char* users[5] = {"user1","user2","user3","user4","user5"};
    char* sp_ids[5] = {"s1","s2","s3","s4","s5"};
    for (int i = 0; i < 100; i++) {
        int op = rand() % 2;
        int user = rand() % 5;
        int room = rand() % 7;

        if (op == 0)
            add_user(users[user],sp_ids[user],rooms[room]);
        else
            rm_user(sp_ids[user]);
    }
    iter = get_head();

    printf("Rooms: %d \n",get_num_rooms());
    for (int i = 0; i < get_num_rooms(); i++, iter = iter->next) {
        printf("Room %s: ",iter->name);
        print_users(iter->users);
    }

    printf("\n\n\n\n");

    //test message insertion, likes
    char* message = "Message";
    for (int i = 1; i < 100; i++) {
        lamport* ts = malloc(sizeof(lamport));
        ts->server_id = 1;
        ts->index = i;

        int op = rand() % 5;
        int user = rand() % 5;
        int room = rand() % 7;
        add_msg(ts, users[user],rooms[room],message); 
    } 
    iter = get_head();
    printf("Rooms: %d \n",get_num_rooms());
    for (int i = 0; i < get_num_rooms(); i++, iter = iter->next) {
        printf(" Room %s:: \n",iter->name);
        print_users(iter->users);
        print_msgs(iter->msgs);
    }
}
