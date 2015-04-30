#include "packet.h"
#include "room.h"
#include "sp.h"


#define int32u unsigned int

static  char    u_id[MAX_NAME_LEN];   //given by user to identify their messages in chatrooms
static  int     server_num; //number of server client is connected to
static  char    server_priv_group[7];
static	char	chatroom[MAX_ROOM_LEN];  //name of chatroom joined
static  char    Spread_name[80];

static  char    Private_group[MAX_GROUP_NAME];
static  mailbox Mbox;
static	int	    Num_sent;
static	unsigned int	Previous_len;

static  int     To_exit = 0;

#define MAX_VSSETS      10
#define MAX_MEMBERS     100

static	void	Print_menu();
static  void    master_print();
static	void	User_command();
static	void	Read_message();
static  void	Bye();

static  int     isValidServer(int);

int main( int argc, char *argv[] )
{
        int	    ret;
        int     mver, miver, pver;
        sp_time test_timeout;

        test_timeout.sec = 5;
        test_timeout.usec = 0;
        server_num = 0;

        if (!SP_version( &mver, &miver, &pver)) 
        {
	          printf("main: Illegal variables passed to SP_version()\n");
	          Bye();
	    }
	    printf("Spread library version is %d.%d.%d\n", mver, miver, pver);

	    ret = SP_connect_timeout( SPREAD_NAME, NULL, 0, 1, &Mbox, Private_group, test_timeout );
	    if( ret != ACCEPT_SESSION ) 
	    {
		    SP_error( ret );
		    Bye();
	    }
	    printf("Client: connected to %s with private group %s\n", SPREAD_NAME, Private_group );

	    E_init();

	    E_attach_fd( 0, READ_FD, User_command, 0, NULL, LOW_PRIORITY );

	    E_attach_fd( Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY );

	    Print_menu();

	    printf("\n > ");
	    fflush(stdout);

	    E_handle_events();

	    return( 0 );
}

static	void	User_command()
{
	char	command[130];
        char    tmp[MAX_ROOM_LEN];
        char    client_group[13];
	char	mess[SPREAD_MESS_LEN];
        char    group[MAX_ROOM_LEN - strlen("CR#_")];
	char	groups[10][MAX_GROUP_NAME];
	int	num_groups;
	unsigned int	mess_len;
	int	ret;
        int     srv;
	int	i;
	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, 130, stdin ) == NULL ) 
            Bye();

	switch( command[0] )
	{
                case 'u':
                        ret = sscanf( &command[2], "%s", u_id);
                        command[ret] = '\0';
                        if (ret < 1 )
                            printf("invalid username\n");
                        break;
                case 'c':
                        ret = sscanf( &command[2], "%d", &srv);
                        command[ret] = '\0';
                        if ( ret < 1 || isValidServer(srv) != 1) { //isValid sets server_num
                            printf("invalid server index\n");
                            break;
                        }
                        ret = sprintf( client_group, "client_group%d",server_num);
                        if (ret < 1) {
                            printf("Error connecting to server-client group\n");
                            break;
                        }
                        ret = SP_join(Mbox, client_group);
                        if (ret < 0) SP_error( ret );
                        sprintf( server_priv_group, "server%d", server_num);
                        break;  
		case 'j':
                        //get new chatroom
                        ret = sscanf( &command[2], "%s", tmp );
                        if( ret < 1 ) {
                            printf(" invalid chatroom name \n");
                            break;}
                        if (strcmp(chatroom,tmp) == 0){
                            printf("Already in chatroom\n");
                            break;
                        }
 
                        //check if already in room
                        if (strlen(chatroom) > 0) {
                            rm_room(chatroom);
                            leave_chatroom* dat = (leave_chatroom*)mess;
                            dat->type = TYPE_LEAVE_CHATROOM;
                            dat->source = SOURCE_CLIENT;
                            strcpy(dat->sp_id,Private_group);
                            strcpy(dat->room, chatroom);

			    SP_multicast( Mbox, AGREED_MESS, server_priv_group, 1, sizeof(leave_chatroom), mess );
                            sprintf(group, "CR%d_%s",server_num,chatroom);
                            SP_leave( Mbox, group);
                        } 
                        strcpy(chatroom,tmp);
                        
                        if (strlen(u_id) > 0 && isValidServer(server_num)) { //need valid username + server for chatroom

                            add_room(chatroom);
                            ret = sprintf ( group, "CR%d_%s",server_num,chatroom);
                            ret = SP_join( Mbox, group );  //join CR#_<group> spread group
                            if( ret < 0 ) SP_error( ret );
                        }
                        else {
                            printf("Invalid username or server number, cannot join chatroom\n"); break;}
                        join_chatroom* send = (join_chatroom*)mess;
                        send->type = TYPE_JOIN_CHATROOM;
                        send->source = SOURCE_CLIENT;
                        strncpy(send->u_id,u_id,MAX_NAME_LEN);
                        strncpy(send->chatroom,chatroom,MAX_ROOM_LEN); //send <group> as chatroom spread name
			ret = SP_multicast( Mbox, AGREED_MESS, server_priv_group, 1, sizeof(join_chatroom), mess );
			if( ret < 0 ) 
			{
				SP_error( ret );
				Bye();
			}
			break;
		case 'a': {
                        char text[MAX_MESS_LEN];
			printf("enter message: ");
			if (fgets(text, MAX_MESS_LEN, stdin) == NULL)
				Bye();
                        text[strlen(text)-1] = '\0';
                        if (strlen(chatroom) < 1){
                            printf("Please join a chatroom\n");
                            break;
                        }
                        message* dat = (message*)mess;
                        dat->type = TYPE_SEND_MSG;
                        dat->source = SOURCE_CLIENT;
                        strcpy(dat->u_id,u_id);
                        strcpy(dat->chatroom,chatroom);
                        strcpy(dat->mess,text);

			ret= SP_multicast( Mbox, AGREED_MESS, server_priv_group, 1, sizeof(message), mess );
			if( ret < 0 )  {
                            SP_error( ret );
                            Bye();}
            
                        print_room(chatroom);
			break;
                }
		case 'l': 
                {
                        if (strlen(chatroom) < 1){
                            printf("Please join a chatroom\n");
                            break;}

                        int linenum;        
                        ret = sscanf( &command[2], "%d", &linenum );
			if( ret < 1 ) {
				printf(" invalid linenum \n");
				break;}
			lamport* stamp = get_timestamp_from_index(find_room(chatroom),linenum);
                        like* dat = (like*)mess;
                        dat->type = TYPE_LIKE_MSG;
                        dat->source = SOURCE_CLIENT;
                        dat->msg_timestamp.server_id = stamp->server_id;
                        dat->msg_timestamp.index = stamp->index;
                        strncpy(dat->u_id,u_id,MAX_NAME_LEN);
                        strncpy(dat->chatroom,chatroom,MAX_ROOM_LEN);
                        dat->like_state = LIKE;                      

                        ret = SP_multicast( Mbox, AGREED_MESS, server_priv_group, 1, sizeof(like), mess);
                        if (ret < 0) {
                            SP_error(ret); Bye; }

			break;
                }
                case 'r':
                {
                        if (strlen(chatroom) < 1){
                            printf("Please join a chatroom\n");
                            break;}

                        int linenum;        
                        ret = sscanf( &command[2], "%d", &linenum );
			if( ret < 1 ) {
				printf(" invalid linenum \n");
				break;}
			lamport* stamp = get_timestamp_from_index(find_room(chatroom),linenum);
                        like* dat = (like*)mess;
                        dat->type = TYPE_LIKE_MSG;
                        dat->source = SOURCE_CLIENT;
                        dat->msg_timestamp.server_id = stamp->server_id;
                        dat->msg_timestamp.index = stamp->index;
                        strncpy(dat->u_id,u_id,MAX_NAME_LEN);
                        strncpy(dat->chatroom,chatroom,MAX_ROOM_LEN);
                        dat->like_state = UNLIKE;                      

                        ret = SP_multicast( Mbox, AGREED_MESS, server_priv_group, 1, sizeof(like), mess);
                        if (ret < 0) {
                            SP_error(ret); Bye; }

			break;
                }
                case 'h':
                        printf("history not implemented\n");
                        break;
                case 'v':
                        printf("network view not implemented\n");
                        break;
		case 'q':
			Bye();
			break;

		default:
			printf("\nUnknown commnad\n");
			Print_menu();

			break;
	}

    printf(" \n > ");
    fflush(stdout);
}

static	void	Print_menu()
{
	printf("\n");
	printf("==============\n");
	printf("Commands Menu:\n");
	printf("--------------\n");
	printf("\n");
	printf("\tu <name> -- set username\n");
	printf("\tc <server> -- connect to server\n");
	printf("\tj <chatroom> -- join a chatroom\n");
	printf("\n");
	printf("\ta  -- send a message to chatroom\n");
	printf("\n");
	printf("\tl <linenum> -- like a message  \n");
	printf("\tr <linenum> -- unlike a message \n");
	printf("\n");
	printf("\th -- view chatroom history\n");
	printf("\tv -- view current network partition \n");
	printf("\n");
	printf("\tq -- quit\n");
	fflush(stdout);
}

static	void	Read_message()
{

static	char		 mess[SPREAD_MESS_LEN];
        char		 sender[MAX_GROUP_NAME];
        char		 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
        membership_info  memb_info;
        vs_set_info      vssets[MAX_VSSETS];
        unsigned int     my_vsset_index;
        int              num_vs_sets;
        char             members[MAX_MEMBERS][MAX_GROUP_NAME];
        int		 num_groups;
        int		 service_type;
        int16		 mess_type;
        int		 endian_mismatch;
        int		 i,j;
        int		 ret;

        service_type = 0;

	ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups, 
		&mess_type, &endian_mismatch, sizeof(mess), mess );
	if( ret < 0 ) 
	{
                if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
                        service_type = DROP_RECV;
                        printf("\n========Buffers or Groups too Short=======\n");
                        ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups, 
                                          &mess_type, &endian_mismatch, sizeof(mess), mess );
                }
        }
        if (ret < 0 )
        {
		if( ! To_exit )
		{
			SP_error( ret );
			printf("\n============================\n");
			printf("\nBye.\n");
		}
		exit( 0 );
	}
	if( Is_regular_mess( service_type ) )
	{
		//printf("message from %s, of type %d, (endian %d) to %d groups \n(%d bytes): %s\n", sender, mess_type, endian_mismatch, num_groups, ret, mess );
	    
            typecheck* type = (typecheck*)mess;
            if (type->type == TYPE_JOIN_CHATROOM) {
                join_chatroom* dat = (join_chatroom*)mess;
                add_user(dat->u_id,dat->sp_id,dat->chatroom);
            }
            else if (type->type == TYPE_LEAVE_CHATROOM) {
                leave_chatroom* dat = (leave_chatroom*)mess;
                rm_user(dat->sp_id);
            }
            else if (type->type == TYPE_SEND_MSG) {
                room_node* root = find_room(chatroom);  //make space if maxxed              
                if (root->num_msgs > 25) {
                    while(root->msgs->likes != NULL){
                        like_node* rm = root->msgs->likes;
                        root->msgs->likes = rm->next;
                        free(rm->timestamp);
                        free(rm);
                    }
                    msg_node* mrm = root->msgs;
                    root->msgs = mrm->next;
                    free(mrm);
                    root->num_msgs--;
                }

                message* dat = (message*)mess;
                lamport* ts = malloc(sizeof(lamport));
                ts->server_id = dat->timestamp.server_id;
                ts->index = dat->timestamp.index;
                add_msg(ts, dat->u_id, dat->chatroom, dat->mess);             
            }
            else if (type->type == TYPE_LIKE_MSG) {
                like* dat = (like*)mess;
                lamport* lts = malloc(sizeof(lamport));
                lamport* mts = malloc(sizeof(lamport));
                lts->server_id = dat->timestamp.server_id;
                lts->index = dat->timestamp.index;
                mts->server_id = dat->msg_timestamp.server_id;
                mts->index = dat->msg_timestamp.index;

                add_like(dat->like_state, dat->u_id, lts, find_msg(chatroom,mts)); 
            } 

            master_print();
	}else if( Is_membership_mess( service_type ) )
        {
                ret = SP_get_memb_info( mess, service_type, &memb_info );
                if (ret < 0) {
                        printf("BUG: membership message does not have valid body\n");
                        SP_error( ret );
                        exit( 1 );
                }
		if     ( Is_reg_memb_mess( service_type ) )
		{
			//printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
			//	sender, num_groups, mess_type );
			//for( i=0; i < num_groups; i++ )
			//	printf("\t%s\n", &target_groups[i][0] );
			//printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

			if( Is_caused_join_mess( service_type ) )
			{
		//		printf("Due to the JOIN of %s\n", memb_info.changed_member );
			}else if( Is_caused_leave_mess( service_type ) ){
	//			printf("Due to the LEAVE of %s\n", memb_info.changed_member );
			}else if( Is_caused_disconnect_mess( service_type ) ){	
                            char * ptr = strchr(memb_info.changed_member,'#')+1;
                            if (atoi(ptr) == server_num) {
                                printf("The server has disconnected. Please choose a new server. \n > ");
                                fflush(stdout);
                                rm_room(chatroom);
                                strcpy(chatroom,"\0");
                                char group[MAX_ROOM_LEN];
                                sprintf(group,"CR%d_%s",server_num,chatroom);
                                SP_leave(Mbox, group);
                                server_num = -1;

                            }
			}else if( Is_caused_network_mess( service_type ) ){
				printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
                                num_vs_sets = SP_get_vs_sets_info( mess, &vssets[0], MAX_VSSETS, &my_vsset_index );
                                if (num_vs_sets < 0) {
                                        printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
                                        SP_error( num_vs_sets );
                                        exit( 1 );
                                }
                                for( i = 0; i < num_vs_sets; i++ )
                                {
                                        printf("%s VS set %d has %u members:\n",
                                               (i  == my_vsset_index) ?
                                               ("LOCAL") : ("OTHER"), i, vssets[i].num_members );
                                        ret = SP_get_vs_set_members(mess, &vssets[i], members, MAX_MEMBERS);
                                        if (ret < 0) {
                                                printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
                                                SP_error( ret );
                                                exit( 1 );
                                        }
                                        for( j = 0; j < vssets[i].num_members; j++ )
                                                printf("\t%s\n", members[j] );
                                }
			}
		}else if( Is_transition_mess(   service_type ) ) {
			printf("received TRANSITIONAL membership for group %s\n", sender );
		}else if( Is_caused_leave_mess( service_type ) ){
			//printf("received membership message that left group %s\n", sender );
		}else printf("received incorrecty membership message of type 0x%x\n", service_type );
        } else if ( Is_reject_mess( service_type ) )
        {
		printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
			sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
	}else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);


}

static  void	Bye()
{
	To_exit = 1;

	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}

static int isValidServer(int in) {
    if (in > 0 && in < 6) {
        server_num = in;
        return 1;
    }
    return 0;
}

static void master_print() {
    fflush(stdout);
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    print_room(chatroom);
    printf(" \n > ");
    fflush(stdout);
}
