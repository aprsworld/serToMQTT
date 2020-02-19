CC=gcc
CFLAGS=-I. -Wunused-function  -Wunused-variable -g

serToMQTT: main.o protocol_text.o protocol_NMEA0183.o
	$(CC) main.o  protocol_text.o protocol_NMEA0183.o -o serToMQTT $(CFLAGS)  -lm -ljson-c -lmosquitto 


main.o: main.c serToMQTT.h
	$(CC)  -c main.c  $(CFLAGS) -I/usr/include/json-c/

protocol_text.o: protocol_text.c serToMQTT.h \
	protocol_text_iMet_XQ2.c protocol_text_TriSoncica_Mini.c
	$(CC)  -c protocol_text.c  $(CFLAGS) -I/usr/include/json-c/

protocol_NMEA0183.o: protocol_NMEA0183.c serToMQTT.h \
	 protocol_NMEA0183.formatter.c
	$(CC)  -c protocol_NMEA0183.c  $(CFLAGS) -I/usr/include/json-c/
