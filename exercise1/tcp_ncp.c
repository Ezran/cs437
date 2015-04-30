#include "net_include.h"
#include <sys/time.h>

int main(int argc, char* argv[])
{
    struct sockaddr_in host;
    struct hostent     h_ent, *p_h_ent;

    char               host_name[80];
    char               *c;

    int                s;
    int                ret;
    int                mess_len;
    char               mess_buf[MAX_MESS_LEN];
    char               *neto_mess_ptr = &mess_buf[sizeof(mess_len)]; 
	FILE*		fp;
	struct timeval	start, interm, end;
	long long	seconds;
	int		mb_sent,interm_mb;

    s = socket(AF_INET, SOCK_STREAM, 0); /* Create a socket (TCP) */
    if (s<0) {
        perror("Net_client: socket error");
        exit(1);
    }

	if (argc != 3) {
		printf("client <hostname> <file>\n");
		exit(0);
	}

    host.sin_family = AF_INET;
    host.sin_port   = htons(PORT);

    p_h_ent = gethostbyname(argv[1]);
    if ( p_h_ent == NULL ) {
        printf("net_client: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent) );
    memcpy( &host.sin_addr, h_ent.h_addr_list[0],  sizeof(host.sin_addr) );

	fp = fopen(argv[2], "r");

    ret = connect(s, (struct sockaddr *)&host, sizeof(host) ); /* Connect! */
    if( ret < 0)
    {
        perror( "Net_client: could not connect to server"); 
        exit(1);
    }
	gettimeofday(&start,NULL);   //start the timer
	interm = start;
	interm_mb = 0;
    for(;;)
    {
       mess_len = fread(mess_buf, 1, MAX_MESS_LEN, fp); 
	mb_sent = mb_sent + mess_len;
	interm_mb = interm_mb + mess_len;
        ret = send( s, mess_buf, mess_len, 0);
	if (interm_mb >= 50000000) { //every 50MB print speed
		gettimeofday(&end, NULL);
		seconds = (end.tv_sec * (unsigned int)1e6 + end.tv_usec) - (interm.tv_sec * (unsigned int)1e6 + interm.tv_usec);
		interm = end;
		printf("[%lld] %d MB sent @ %d mb/s\n",seconds,(int)(mb_sent/1000000),8*(int)(interm_mb/seconds));
		interm_mb = 0;
	}
        if(ret != mess_len) 
        {
            perror( "Net_client: error in writing");
            exit(1);
        }
	else if (feof(fp)){
		gettimeofday(&end, NULL);
		seconds = (end.tv_sec * (unsigned int)1e6 + end.tv_usec) - (start.tv_sec * (unsigned int)1e6 + start.tv_usec);
		printf("File send completed in %.2f seconds. Average transfer rate for %d MB was %d mb/s. \n",(double)(seconds/1e6),(int)(mb_sent/1000000),8*(int)(mb_sent/seconds));
		exit(0);
	}
    }

    return 0;

}

