#include "packet.h"
#include "roomuserslist.h"
#include "roomslist.h"
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

        set_max_msgs(25);

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
                        if (ret < 1 )
                            printf("invalid username\n");
                        break;
                case 'c':
                       ret = sscanf( &command[2], "%d", &srv);
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
                        //check if already in room
                        if (strlen(chatroom) > 0) {
                            clear_room(chatroom);
                        }
                        else { 
                            ret = sscanf( &command[2], "%s", chatroom );
                            if( ret < 1 ) {
                                printf(" invalid chatroom name \n");
                                break;}

                            add_room(chatroom);
                        }
                        if (strlen(u_id) > 0 && isValidServer(server_num)) { //need valid username + server for chatroom
                            ret = sprintf ( group, "%s%d_%s","CR",server_num,chatroom);
                            if (ret < 1) {
                                printf("Error creating chatroom name <%s> from group <%s>\n",chatroom,group);
                                break;
                            }
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
/*
                case 'x':{
                        char ms[6];
                        add_room(chatroom);
                        set_max_msgs(25);
                        int maxi = 40;
                        for (int i = 1; i <= maxi;i++) {
                            struct lamport* ts = malloc(sizeof(lamport));
                            ts->server_id = 1;
                            ts->index = i;
                            sprintf(ms,"%s%d","asdfd",i);
                            add_message(ts,u_id,chatroom,ms);
                            if (i % 3 == 0 || i % 4 == 1 || i % 6 == 2) {
                                i++; maxi++;
                                struct lamport* ls = malloc(sizeof(lamport));
                                ls->server_id = 1;
                                ls->index = i; 
                                add_like(LIKE,ls,u_id,chatroom,ts);
                                if (i % 6 == 2){
                                    i++;maxi++;
                                    char newu[MAX_NAME_LEN];
                                    sprintf(newu,"%sx",u_id); 
                                    struct lamport* qs = malloc(sizeof(lamport));
                                    qs->server_id = 1;
                                    qs->index = i;
                                    add_like(LIKE,qs,newu,chatroom,ts);
                                }
                            }
                            if (i % 4 == 1) {
                                i++; maxi++;
                                struct lamport* ls = malloc(sizeof(lamport));
                                ls->server_id = 1;
                                ls->index = i; 
                                add_like(UNLIKE,ls,u_id,chatroom,ts);
                            }
                        }
                        

                        printf("== %s ==\n",chatroom);
                        print_room(chatroom); 
    
                        break;}
*/
		case 'a': {
                        char text[MAX_MESS_LEN];
			printf("enter message: ");
			if (fgets(text, MAX_MESS_LEN, stdin) == NULL)
				Bye();
                        if (strlen(chatroom) < 1){
                            printf("Please join a chatroom\n");
                            break;
                        }
                        message* dat = (message*)mess;
                        dat->type = TYPE_SEND_MSG;
                        dat->source = SOURCE_CLIENT;
                        strncpy(dat->u_id,u_id,MAX_NAME_LEN);
                        strncpy(dat->chatroom,chatroom,MAX_ROOM_LEN);
                        strncpy(dat->mess,text,MAX_MESS_LEN);
			ret= SP_multicast( Mbox, AGREED_MESS, server_priv_group, 1, sizeof(message), mess );
			if( ret < 0 ) 
		        {
				SP_error( ret );
				Bye();
                        }
			break;
                }
		case 'l': 
                {
                        if (strlen(chatroom) < 1){
                            printf("Please join a chatroom\n");
                            break;}

                        int linenum;        
                        ret = sscanf( &command[2], "%d", &linenum );
                        printf("Liking line %d.....\n",linenum);
			if( ret < 1 ) {
				printf(" invalid linenum \n");
				break;}
			lamport* stamp = get_msg_stamp(linenum);
                        printf("%d. index %d?\n",linenum,stamp->index);
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
                        ret = sscanf( &command[2], "%d", linenum );
			if( ret < 1 ) {
				printf(" invalid linenum \n");
				break;}
			lamport* stamp = get_msg_stamp(linenum);
                        printf("%d. index %d?\n",linenum,stamp->index);
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
                ret = add_user(dat->u_id,dat->sp_id,dat->chatroom);
            }
            else if (type->type == TYPE_LEAVE_CHATROOM) {
                leave_chatroom* dat = (leave_chatroom*)mess;
                rm_user(dat->sp_id);
            }
            else if (type->type == TYPE_SEND_MSG) {
                message* dat = (message*)mess;
                if (strlen(chatroom) == 0)
                    add_room(chatroom);   
                lamport* ts = malloc(sizeof(lamport));
                ts->server_id = dat->timestamp.server_id;
                ts->index = dat->timestamp.index;
                printf("[%d,%d]\n",ts->server_id,ts->index);
                add_message(ts, dat->u_id, dat->chatroom, dat->mess);             
            }
            else if (type->type == TYPE_LIKE_MSG) {
                like* dat = (like*)mess;
                lamport* lts = malloc(sizeof(lamport));
                lamport* mts = malloc(sizeof(lamport));
                lts->server_id = dat->timestamp.server_id;
                lts->index = dat->timestamp.index;
                mts->server_id = dat->timestamp.server_id;
                mts->index = dat->timestamp.index;

                ret = add_like(dat->like_state, lts, dat->u_id, chatroom, mts); 
                printf("~~~~~~~~ %d:[%d,%d | %d,%d] %d %s %s \n",ret, lts->server_id,lts->index,mts->server_id,mts->index,dat->like_state,dat->u_id,chatroom);
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
			printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
				sender, num_groups, mess_type );
			for( i=0; i < num_groups; i++ )
				printf("\t%s\n", &target_groups[i][0] );
			printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

			if( Is_caused_join_mess( service_type ) )
			{
				printf("Due to the JOIN of %s\n", memb_info.changed_member );
			}else if( Is_caused_leave_mess( service_type ) ){
				printf("Due to the LEAVE of %s\n", memb_info.changed_member );
			}else if( Is_caused_disconnect_mess( service_type ) ){
				printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
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
			printf("received membership message that left group %s\n", sender );
		}else printf("received incorrecty membership message of type 0x%x\n", service_type );
        } else if ( Is_reject_mess( service_type ) )
        {
		printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
			sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
	}else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);


	printf("\n");
	fflush(stdout);
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
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Chatroom: %s \n",chatroom);
    print_users(chatroom);
    print_room(chatroom);
    printf(" \n > ");
    fflush(stdout);
}
