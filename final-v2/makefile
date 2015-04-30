CC=gcc
LD=gcc
CFLAGS=-g -Wall 
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

all: client server

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

sp_user:  $(SP_LIBRARY_DIR)/libspread-core.a user.o
	$(LD) -o $@ user.o  $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

class_user:  $(SP_LIBRARY_DIR)/libspread-core.a class_user.o
	$(LD) -o $@ class_user.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

client:  libspread-core.a
	gcc -g -std=gnu99 -o client client.c room.c libspread-core.a -ldl -lm -lrt -lnsl libspread-util.a

server:  libspread-core.a
	gcc -g -std=gnu99 -o server server.c room.c libspread-core.a -ldl -lm -lrt -lnsl libspread-util.a

test:
	gcc -std=gnu99 -g room.c testroom.c -o testroom

clean:
	rm -f *.o client server testroom *.chat

