#include "sp.h"
#include "room.h"
#include <stdio.h>

#include "packet.h"


#define MAX_TRANSFER_LEN 1400
#define MAX_VSSETS       10
#define MAX_MEMBERS      100
#define GROUP_NAME       "ServerGroup"

static mailbox          mbox;
static unsigned int     previous_len;
static int              num_messages;
static int              process_index;
static int              num_processes;
static char             private_group[MAX_GROUP_NAME];

static int              stamp_index;
static char             public_client_group[14];
static char             private_server_group[8];
static int              entropy[5] = {0};

static int              my_id;
static FILE*            fp;
char                    filename[6];

static void             read_message();

int main ( int argc, char* argv[]) {
    
    int         ret;
                stamp_index = 0;
    int         mver, miver, pver;

    // process arguments
    if (argc != 2) {
        printf("Useage: server <process_index>\n");
        return 0;
    }
    else {
        my_id = *argv[1] - '0';
    }
    sprintf(filename,"%d.chat",my_id);

    sprintf(public_client_group, "client_group%d\0",my_id);
    sprintf(private_server_group, "server%d\0",my_id);

    char asdf[1];
    sprintf(asdf,"%d",my_id);
    ret = SP_connect( SPREAD_NAME, asdf, 0, 1, &mbox, private_group);
    if (ret != ACCEPT_SESSION) {
        printf("initial connect error\n");
        SP_error(ret);
        SP_disconnect(mbox);
        return 0;
    }
    
    fp = fopen(filename,"r");
    if (fp != NULL) {
        char * line = NULL;
        size_t len = 0;
        ssize_t read;
        while ((read = getline(&line, &len, fp)) != -1) {
            typecheck* type = (typecheck*)line;
            if (type->type == TYPE_SEND_MSG){
                message* dat = (message*)line;
                lamport* ts = malloc(sizeof(lamport));
                ts->server_id = dat->timestamp.server_id;
                ts->index = dat->timestamp.index;
                add_msg(ts, dat->u_id, dat->chatroom, dat->mess);
            }
            else if (type->type == TYPE_LIKE_MSG){
                like* dat = (like*)line;
                lamport* ts = malloc(sizeof(lamport));
                lamport* mts = malloc(sizeof(lamport));
                ts->server_id = dat->timestamp.server_id;
                ts->index = dat->timestamp.index;
                mts->server_id = dat->msg_timestamp.server_id;
                mts->index = dat->msg_timestamp.index;
                
                add_like(dat->like_state, dat->u_id, ts,find_msg(dat->chatroom,mts));  
                free(mts);
            }
        }
        fclose(fp);
    }


    printf("Connected to <%s> with private group <%s>\n",SPREAD_NAME, private_group);

    ret = SP_join( mbox, GROUP_NAME);
    if (ret < 0) {
        printf("Error joining server update group -- %s ",GROUP_NAME);
        SP_error(ret);
        SP_disconnect(mbox);
        return 0;
    }
    ret = SP_join( mbox, private_server_group);
    if (ret < 0) {
        printf("Error joining private server group -- %s ",private_server_group);
        SP_error(ret);
        SP_disconnect(mbox);
        return 0;
    }
    ret = SP_join( mbox, public_client_group);
    if (ret < 0) {
        printf("Error joining public client group -- %s ",public_client_group);
        SP_error(ret);
        SP_disconnect(mbox);
        return 0;
    }

    E_init();
    E_attach_fd(mbox, READ_FD, read_message, 0, NULL, HIGH_PRIORITY);
    E_handle_events();

    return 0;
}

/*
 *      Receive message event handler
 */
static void read_message() {
    static char         mess[MAX_TRANSFER_LEN];
    char                group[MAX_ROOM_LEN];
    char                sender[MAX_GROUP_NAME];
    char                target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
    membership_info     memb_info;
    vs_set_info         vssets[MAX_VSSETS];
    unsigned int        my_vsset_index;
    int                 num_vs_sets;
    char                members[MAX_MEMBERS][MAX_GROUP_NAME];
    int                 num_groups;
    int		        service_type;
    int16		mess_type;
    int		        endian_mismatch;
    int		        i,j;
    int		        ret;

    service_type = 0;

    ret = SP_receive(mbox, &service_type, sender, 100, &num_groups, target_groups, 
                        &mess_type, &endian_mismatch, sizeof(mess), mess);

/* ==== Begin Text Feedback ==== */
	if( ret < 0 ) 
	{
                if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
                        service_type = DROP_RECV;
                        printf("\n========Buffers or Groups too Short=======\n");
                        printf("Endian mismatch: %d, num groups: %d\n",endian_mismatch, num_groups);
                        ret = SP_receive( mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups, 
                                          &mess_type, &endian_mismatch, sizeof(mess), mess );
                }
        }
        if (ret < 0 )
        {
		if( 1 )
		{
			SP_error( ret );
			printf("\n============================\n");
			printf("\nBye.\n");
		}
		exit( 0 );
	}
	if( Is_regular_mess( service_type ) )
	{
	    //sender, mess_type, endian_mismatch, num_groups, ret, mess
	    typecheck* type = (typecheck*)mess;

            if (type->type == TYPE_JOIN_CHATROOM) {
                join_chatroom* dat = (join_chatroom*)mess;

                if (type->source == SOURCE_CLIENT) {
                    // update local data structures
                    add_user(dat->u_id,sender,dat->chatroom);

                    // send notification to other servers that client joined
                    dat->source = SOURCE_SERVER;
                    strncpy(dat->sp_id,sender,MAX_NAME_LEN);
                    
		    ret = SP_multicast( mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(join_chatroom), mess );
                    if( ret < 0 ) {
                        SP_error( ret );
                        exit(0);}

                    // send local chatroom membership changed
                    sprintf(group, "CR%d_%s",my_id,dat->chatroom);
		    ret = SP_multicast( mbox, AGREED_MESS, group, 1, sizeof(join_chatroom), mess );
                    if( ret < 0 ) {
                        SP_error( ret );
                        exit(0);}

                    // serve client all relevent updates to that chatroom
                        //send user other participants in chatroom
                    room_node* riter = find_room(dat->chatroom);
                    if (riter == NULL){
                        printf("Error finding users in room %s\n",dat->chatroom);
                        exit(0);}
                    printf("Number of users in %s: %d \n",dat->chatroom,riter->num_users);
                    user_node* u_iter = riter->users;
                    while(u_iter != NULL) {
                        strncpy(dat->u_id,u_iter->u_id,MAX_NAME_LEN);
                        ret = SP_multicast( mbox, AGREED_MESS, sender, 1, sizeof(join_chatroom), mess );
                        if( ret < 0 ) {
                            SP_error( ret );
                            exit(0);}
                        u_iter = u_iter->next;
                    }

                        //send user last 25 messages + likes
                    msg_node* msg_iter = riter->msgs;
                    like_node* like_iter;
                    printf("num messages: %d in room...\n",riter->num_msgs);
                    if (riter->num_msgs > 25) {  //move iterator to position
                        for (int i = 1; i < riter->num_msgs - 25; i++)
                            msg_iter = msg_iter->next; 
                    } 
                    while(msg_iter != NULL) { //smash out updates from last 25 msgs:
                        message* out = (message*)mess;
                        out->type = TYPE_SEND_MSG;
                        out->timestamp.server_id = msg_iter->timestamp->server_id; 
                        out->timestamp.index = msg_iter->timestamp->index; 
                        strncpy(out->u_id,msg_iter->u_id,MAX_NAME_LEN);
                        strncpy(out->mess,msg_iter->mess,MAX_MESS_LEN); 
                        strncpy(out->chatroom,riter->name,MAX_ROOM_LEN);

                        ret = SP_multicast( mbox, AGREED_MESS, sender, 1, sizeof(message), mess );
                        if( ret < 0 ) {
                            SP_error( ret );
                            exit(0);}

                        //send likes 
                        for (like_iter = msg_iter->likes; like_iter != NULL; like_iter = like_iter->next) {
                            like* lout = (like*)mess;
                            lout->type = TYPE_LIKE_MSG;
                            lout->timestamp.server_id = like_iter->timestamp->server_id;
                            lout->timestamp.index = like_iter->timestamp->index;
                            strncpy(lout->u_id,like_iter->u_id,MAX_NAME_LEN);
                            lout->msg_timestamp.server_id = msg_iter->timestamp->server_id;
                            lout->msg_timestamp.index = msg_iter->timestamp->index;
                            lout->like_state = like_iter->state;

                            ret = SP_multicast( mbox, AGREED_MESS, sender, 1, sizeof(like), mess );
                            if( ret < 0 ) {
                                SP_error( ret );
                                exit(0);}
                        }
                        msg_iter = msg_iter->next;
                    }
                    //end user join chatroom update sending smash
                }
                else if (type->source == SOURCE_SERVER) {
                    add_user(dat->u_id,dat->sp_id,dat->chatroom);

                    //send local chatroom membership changed
                    sprintf(group, "CR%d_%s",my_id,dat->chatroom);
		    ret = SP_multicast( mbox, AGREED_MESS, group, 1, sizeof(join_chatroom), mess );
                    if( ret < 0 ) {
                        SP_error( ret );
                        exit(0);}
                }
            }
            else if (type->type == TYPE_LEAVE_CHATROOM) {
                leave_chatroom* dat = (leave_chatroom*)mess;
                if (type->source == SOURCE_CLIENT){
                    // send update to other servers if necessary 
                    type->source = SOURCE_SERVER;
                    SP_multicast( mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(leave_chatroom),mess);
                    //rm from local struct
                    rm_user(dat->sp_id);
                    //update local clients
                    sprintf(group, "CR%d_%s",my_id,dat->room);
                    ret = SP_multicast( mbox, AGREED_MESS, group, 1, sizeof(leave_chatroom), mess );
                    if( ret < 0 ) {
                        SP_error( ret );
                        exit(0);}
                }
                else if (type->source == SOURCE_SERVER && strcmp(sender,private_group) != 0){
                    //update local list
                    rm_user(dat->sp_id);
                    //update local clients 
                    sprintf(group, "CR%d_%s",my_id,dat->room);
                    printf("server\n");
                    ret = SP_multicast( mbox, AGREED_MESS, group, 1, sizeof(leave_chatroom), mess );
                    if( ret < 0 ) {
                        SP_error( ret );
                        exit(0);}
                }
            }
            else if (type->type == TYPE_SEND_MSG) {
                message* dat = (message*)mess;
                if (type->source == SOURCE_CLIENT) {
                    //stamp the update
                    dat->source = SOURCE_SERVER;
                    dat->timestamp.server_id = my_id;
                    dat->timestamp.index = ++stamp_index;
                    //write update to file
                    fp = fopen(filename,"a");
                    fprintf(fp,"%s\n",mess);
                    fclose(fp);
                    //update anti-entropy vector
                    //
                    //send to other servers
                    ret = SP_multicast( mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(message), mess);
                    if (ret < 0) { SP_error(ret);exit(0);}
                    //send to local CR
                    sprintf(group,"CR%d_%s",my_id,dat->chatroom);
                    printf("Sending message %s back to chatroom %s...\n",dat->mess,group);
                    ret = SP_multicast( mbox, AGREED_MESS, group, 1, sizeof(message), mess);
                    if (ret < 0) {SP_error(ret);exit(0);}
                    //update data struct                 
                    lamport* ts = malloc(sizeof(lamport));
                    ts->server_id = my_id;
                    ts->index = stamp_index;
                    add_msg(ts, dat->u_id, dat->chatroom, dat->mess);
                }
                else if (type->source == SOURCE_SERVER && dat->timestamp.server_id != my_id) {
                    fp = fopen(filename,"a");
                    fprintf(fp,"%s\n",mess);
                    fclose(fp);
                    //update vector
                    //
                    //increment local next msg counter if higher
                    if (dat->timestamp.index > stamp_index)
                        stamp_index = dat->timestamp.index;
                    //send to local CR
                    sprintf(group,"CR%d_%s",my_id,dat->chatroom);
                    ret = SP_multicast( mbox, AGREED_MESS, group, 1, sizeof(message), mess);
                    if (ret < 0) {SP_error(ret);exit(0);}
                    //update data struct
                    lamport* ts = malloc(sizeof(lamport));
                    ts->server_id = dat->timestamp.server_id;
                    ts->index = dat->timestamp.index;
                    add_msg(ts, dat->u_id, dat->chatroom, dat->mess);
                }
            }
            else if (type->type == TYPE_LIKE_MSG) {
                like* dat = (like*)mess;
                if (type->source == SOURCE_CLIENT) {
                    //stamp
                    dat->source = SOURCE_SERVER;
                    dat->timestamp.server_id = my_id;
                    dat->timestamp.index = ++stamp_index;
                    //write to file
                    fp = fopen(filename,"a");
                    fprintf(fp,"%s\n",mess);
                    fclose(fp);
                    //update vector
                    //
                    //send to other servers
                    ret = SP_multicast( mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(like), mess);
                    if (ret < 0) { SP_error(ret);exit(0);}
                    //send to local CR
                    printf("Sending like <[%d %d] %d,%s> down to local chatroom...\n",dat->timestamp.server_id, dat->timestamp.index, dat->like_state,dat->u_id);
                    sprintf(group,"CR%d_%s",my_id,dat->chatroom);
                    ret = SP_multicast( mbox, AGREED_MESS, group, 1, sizeof(like), mess);
                    if (ret < 0) {SP_error(ret);exit(0);}
                    //update data struct
                    lamport* lts = malloc(sizeof(lamport));
                    lamport* mts = malloc(sizeof(lamport));
                    lts->server_id = my_id;
                    lts->index = stamp_index;
                    mts->server_id = dat->msg_timestamp.server_id;
                    mts->index = dat->msg_timestamp.index;
                    
                    add_like (dat->like_state,dat->u_id, lts, find_msg(dat->chatroom,mts));
                }
                else if (type->source == SOURCE_SERVER && dat->timestamp.server_id !=my_id) {
                    //write to file
                    fp = fopen(filename,"a");
                    fprintf(fp,"%s\n",mess);
                    fclose(fp);
                    //update vector
                    //
                    //increment local next msg counter if higher
                    if (dat->timestamp.index > stamp_index)
                        stamp_index = dat->timestamp.index;
                    //send to local CR
                    sprintf(group,"CR%d_%s",my_id,dat->chatroom);
                    ret = SP_multicast( mbox, AGREED_MESS, group, 1, sizeof(like), mess);
                    //update data struct
                    lamport* lts = malloc(sizeof(lamport));
                    lamport* mts = malloc(sizeof(lamport));
                    lts->server_id = dat->timestamp.server_id;
                    lts->index = dat->timestamp.index;
                    mts->server_id = dat->msg_timestamp.server_id;
                    mts->index = dat->msg_timestamp.index;
                    
                    add_like (dat->like_state, dat->u_id, lts, find_msg(dat->chatroom, mts));
                }
            }
            else if (type->type == TYPE_PARTITION_STATE) {
                //get list of servers in partition
                //send to client that requested it
            }
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
			}else if( Is_caused_leave_mess( service_type ) || Is_caused_disconnect_mess( service_type) ){
                            printf(" Got leave or disconnect \n"); 
                            if (strcmp(sender,public_client_group) == 0) {
                                printf("Client %s left or disconnected from group %s \n",memb_info.changed_member,sender);
                                //update local list
                                rm_user(memb_info.changed_member);
                                //update local clients 
                                leave_chatroom* send = malloc(sizeof(leave_chatroom));
                                send->type = TYPE_LEAVE_CHATROOM;
                                send->source = SOURCE_SERVER;
                                strcpy(send->sp_id,memb_info.changed_member);
                                strcpy(send->room,strchr(sender,'_')+1);
                                
                                printf("Sending membership change...\n");
                                SP_multicast( mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(leave_chatroom), (char*)send); 
                                ret = SP_multicast( mbox, AGREED_MESS, sender, 1, sizeof(leave_chatroom), (char*)send );
                                if( ret < 0 ) {
                                    SP_error( ret );
                                    exit(0);
                                }
                            }
                            else if (strcmp(sender,GROUP_NAME) == 0) {
                                printf("A server joined or left the server group\n");
                            }
			}else if( Is_caused_disconnect_mess( service_type ) ){
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
/* ==== End Feedback Messages ==== */

    

/* ==== Begin Application Code ==== */
#if 0    
    if ( memb_info.gid.id[2] == num_processes ) {
        // start timer
        if (time_started == 0) {
            printf("Begin test...\n");
            gettimeofday(&starttime,NULL);
            time_started = 1;
        }

        // read mess -- save data
        dat = (packet*)mess; 
        if (dat->type != 2) {
           //put data in array
            if(dat->type == 1) {
                dat_array[0][dat_array_len] = dat->process_index;
                dat_array[1][dat_array_len] = dat->message_index;
                dat_array[2][dat_array_len] = dat->random_data;
                dat_array_len++;
            }
           //generate new packet to send
            if(num_sent < num_messages && dat->process_index == process_index) 
                counter++;
            //check to see if we can send a new burst of packets
            if ( counter >= counter_max ) {
                //calculate if we have less than BURST_SIZE packets left to send
                if (num_messages - num_sent < BURST_SIZE)
                    minval = num_messages-num_sent;
                else
                    minval = BURST_SIZE;
                for (i = 0; i < minval; i++) {    
                    dat->type = 1;
                    dat->process_index = process_index;
                    dat->message_index = num_sent++;
                    dat->random_data = rand() % 1000000 + 1;
                    
                        
                    ret = SP_multicast( mbox, AGREED_MESS, GROUP_NAME, 2, sizeof(packet), mess );
                    if( ret < 0 ) 
                    {
                            SP_error( ret );
                            SP_disconnect(mbox);
                            exit(0);
                    }
                }
                //send a done_sending packet if we are finished
                if (num_sent == num_messages) {
                    done_sending* ds = (done_sending*)mess;
                    ds->type = 2;
                    ret = SP_multicast( mbox, AGREED_MESS, GROUP_NAME, 2, sizeof(done_sending), mess );
                    if( ret < 0 ) 
                    {
                            SP_error( ret );
                            SP_disconnect(mbox);
                            exit(0);
                    }     
                }
                counter = 0;
                counter_max = BURST_SIZE;
            }
            // write array to file if full
            if (dat_array_len == ARRAY_SIZE) {
                for (i = 0; i < dat_array_len; i++)
                    fprintf( fp, "%2d, %8d, %8d\n", dat_array[0][i], dat_array[1][i], dat_array[2][i] );
                dat_array_len = 0;
            }
        }
        else if (dat->type == 2) {
            num_finished++;
            if (num_finished == num_processes) { //everyone is done
                for (i = 0; i < dat_array_len; i++)
                    fprintf( fp, "%2d, %8d, %8d\n", dat_array[0][i], dat_array[1][i], dat_array[2][i] );
                gettimeofday(&endtime,NULL);
                fclose(fp);
                SP_disconnect(mbox);
                printf("Done.\n\n");
                
                //print time results
                int diffsec = endtime.tv_sec - starttime.tv_sec;
                int diffusec = endtime.tv_usec - starttime.tv_usec;
                double totesec = (double)diffsec + (((double)diffusec)/1000000);                

                printf("Time taken was %.3f sec.\n",totesec);

                exit(0);
            } 
        }
    }
#endif
}

