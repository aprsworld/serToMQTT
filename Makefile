CC=gcc
CFLAGS=-I. -Wunused-function  -Wunused-variable -g

serToMQTT: main.o protocol_text.o protocol_NMEA0183.o
	$(CC) main.o  protocol_text.o protocol_NMEA0183.o -o serToMQTT $(CFLAGS)  -lm -ljson-c -lmosquitto 


main.o: main.c
	$(CC)  -c main.c  $(CFLAGS) -I/usr/include/json-c/

protocol_text.o: protocol_text.c
	$(CC)  -c protocol_text.c  $(CFLAGS) -I/usr/include/json-c/

protocol_NMEA0183.o: protocol_NMEA0183.c
	$(CC)  -c protocol_NMEA0183.c  $(CFLAGS) -I/usr/include/json-c/
