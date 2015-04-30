all: main start 

main:
	gcc -std=c99 -g -D_GNU_SOURCE mcast.c recv_dbg.c -o mcast

start:
	gcc -std=c99 -D_GNU_SOURCE start_mcast.c -o start_mcast

clean:
	rm -f mcast start_mcast *.out
