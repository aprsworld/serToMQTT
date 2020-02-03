CC=gcc
CFLAGS=-I. -Wunused-function  -Wunused-variable -g

serToMQTT: main.o text_engine.o nmea0183_engine.o
	$(CC) main.o  text_engine.o nmea0183_engine.o -o serToMQTT $(CFLAGS)  -lm -ljson-c


main.o: main.c
	$(CC)  -c main.c  $(CFLAGS) -I/usr/include/json-c/

text_engine.o: text_engine.c
	$(CC)  -c text_engine.c  $(CFLAGS) -I/usr/include/json-c/

nmea0183_engine.o: nmea0183_engine.c
	$(CC)  -c nmea0183_engine.c  $(CFLAGS) -I/usr/include/json-c/
