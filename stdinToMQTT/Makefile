CC=gcc
CFLAGS=-I./json-c-0.14 -I/usr/include/ -I. -Wunused-function  -Wunused-variable -g
LDFLAGS=-Ljson-c-build

stdinToMQTT: stdinToMQTT.o 
	$(CC) stdinToMQTT.o  \
	-o stdinToMQTT $(CFLAGS) $(LDFLAGS)  -lm -ljson-c -lmosquitto 

stdinToMQTT.o: stdinToMQTT.c 
	$(CC)  -c stdinToMQTT.c  $(CFLAGS) -I/usr/include/json-c/ 


clean:
	rm -f *.o 
