#include "sp.h"
#include "packet.h"

#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE      1000
#define GROUP_NAME      "fforell1@ugrad"
#define SPREAD_NAME     "4803"
#define USERNAME        "fforell1"
#define MAX_MESS_LEN    1400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100
#define BURST_SIZE      100
#define BURST_PAD       99

static mailbox          mbox;
static int              num_sent;
static unsigned int     previous_len;
static int              num_messages;
static int              process_index;
static int              num_processes;
static char             private_group[MAX_GROUP_NAME];
static int              counter;
static int              counter_max;
static int              dat_array[3][ARRAY_SIZE];
static int              dat_array_len;
static FILE*            fp;
static int              num_finished;

static struct timeval   starttime;
static struct timeval   endtime;
static int              time_started;


static void             read_message();

int main ( int argc, char* argv[]) {
    
    int         ret;
    int         mver, miver, pver;
    num_sent = 0;
    counter = BURST_SIZE - BURST_PAD;
    counter_max = BURST_SIZE - BURST_PAD;
    dat_array_len = 0;
    num_finished = 0;
    time_started = 0;

    // process arguments
    if (argc != 4) {
        printf("Useage: mcast <num_messages> <process_index> <num_processes>\n");
        return 0;
    }
    else {
        num_messages = atoi(argv[1]);
        process_index = atoi(argv[2]);
        num_processes = atoi(argv[3]);
    }
    char* tmp = malloc(sizeof(char)*strlen(argv[2])+sizeof(char)*4);
    sprintf(tmp,"%s%s",argv[2],".out");
    fp = fopen(tmp,"w");
    printf("writing to %s\n",tmp);
    free(tmp);

    ret = SP_connect( SPREAD_NAME, USERNAME, 0, 1, &mbox, private_group);
    if (ret != ACCEPT_SESSION) {
        SP_error(ret);
        SP_disconnect(mbox);
        return 0;
    }

    printf("User: connected to <%s> with private group <%s>\n",SPREAD_NAME, private_group);

    ret = SP_join( mbox, GROUP_NAME);
    if (ret < 0) {
        printf("Error joining group -- ");
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
    static char         mess[MAX_MESS_LEN];
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
    int                 minval;
    packet*             dat;

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
		/*mess[ret] = 0;
		if     ( Is_unreliable_mess( service_type ) ) printf("received UNRELIABLE ");
		else if( Is_reliable_mess(   service_type ) ) printf("received RELIABLE ");
		else if( Is_fifo_mess(       service_type ) ) printf("received FIFO ");
		else if( Is_causal_mess(     service_type ) ) printf("received CAUSAL ");
		else if( Is_agreed_mess(     service_type ) ) printf("received AGREED ");
		else if( Is_safe_mess(       service_type ) ) printf("received SAFE ");
		printf("message from %s, of type %d, (endian %d) to %d groups \n(%d bytes): %s\n",
			sender, mess_type, endian_mismatch, num_groups, ret, mess );
        */
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
/* ==== End Feedback Messages ==== */

/* ==== Begin Application Code ==== */    
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
}

