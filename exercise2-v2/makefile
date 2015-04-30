all: main start 

main:
	gcc -std=c99 -g -D_GNU_SOURCE circlearray.c mcast.c recv_dbg.c -o mcast

start:
	gcc -std=c99 -g -D_GNU_SOURCE start_mcast.c -o start_mcast

test:
	gcc -std=gnu99 -g circlearray.c testarray.c -o testarray 

clean:
	rm -f testarray mcast start_mcast *.out
