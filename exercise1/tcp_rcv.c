#include "net_include.h"
#include "queue.h"

int main()
{
    struct sockaddr_in name;
    int                s;
    int                w;
    fd_set             mask;
    int                recv_s[10];
    int                valid[10];  
    fd_set             dummy_mask,temp_mask;
    int                i,j,num;
    int                mess_len;
    int                neto_len;
    long int           total_written;
    int 	       last_mb_written;
    char               mess_buf[MAX_MESS_LEN];
    long               on=1;
    FILE *	 	fp;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s<0) {
        perror("Net_server: socket");
        exit(1);
    }

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        perror("Net_server: setsockopt error \n");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( s, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Net_server: bind");
        exit(1);
    }
 
    if (listen(s, 4) < 0) {
        perror("Net_server: listen");
        exit(1);
    }

    i = 0;
    FD_ZERO(&mask);
    FD_ZERO(&dummy_mask);
    FD_SET(s,&mask);

	fp = fopen("received","w");
	last_mb_written = 0;
	total_written = 0;

    for(;;)
    {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
        if (num > 0) {
            if ( FD_ISSET(s,&temp_mask) ) {
                recv_s[i] = accept(s, 0, 0) ;
                FD_SET(recv_s[i], &mask);
                valid[i] = 1;
                i++;
            }
            for(j=0; j<i ; j++)
            {   if (valid[j])    
                if ( FD_ISSET(recv_s[j],&temp_mask) ) {
			
		    mess_len = recv(recv_s[j],mess_buf,MAX_MESS_LEN,0);

                    if( mess_len > 0) 
			push(mess_buf, mess_len);
                    else
                    {
                        printf("closing %d \n",j);
			while (get_size() > 0){
				int tmp_len;
				fwrite(pop(&tmp_len),1,tmp_len,fp);
			}
			fclose(fp);
                        FD_CLR(recv_s[j], &mask);
                        close(recv_s[j]);
                        valid[j] = 0;  
                    }
                }
            }
        }
    }

    return 0;

}

