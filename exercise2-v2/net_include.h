#include <stdio.h>

#include <stdlib.h>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h> 
#include <netdb.h>

#include <errno.h>

#define PORT	     10160
#define IPADDR	     225 << 24 | 1 << 16 | 2 << 8 | 116 /* (225.1.2.116) */
	

