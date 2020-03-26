CC=gcc
CFLAGS=-I. -Wunused-function  -Wunused-variable -g

serToMQTT: serToMQTT.o protocol_text.o protocol_NMEA0183.o setDateTimeFromGPS.o
	$(CC) serToMQTT.o  protocol_text.o protocol_NMEA0183.o setDateTimeFromGPS.o -o serToMQTT $(CFLAGS)  -lm -ljson-c -lmosquitto 


serToMQTT.o: serToMQTT.c serToMQTT.h
	$(CC)  -c serToMQTT.c  $(CFLAGS) -I/usr/include/json-c/

protocol_text.o: protocol_text.c serToMQTT.h \
	protocol_text_iMet_XQ2.c protocol_text_TriSoncica_Mini.c
	$(CC)  -c protocol_text.c  $(CFLAGS) -I/usr/include/json-c/

protocol_NMEA0183.o: protocol_NMEA0183.c serToMQTT.h \
	 protocol_NMEA0183.formatter.c
	$(CC)  -c protocol_NMEA0183.c  $(CFLAGS) -I/usr/include/json-c/

setDateTimeFromGPS.o: setDateTimeFromGPS.c 
	$(CC)  -c setDateTimeFromGPS.c  $(CFLAGS) -I/usr/include/json-c/

backToJson: backToJson.c protocol_NMEA0183.formatter.c
	$(CC) backToJson.c  -o backToJson $(CFLAGS)  -lm -ljson-c -lmosquitto -I/usr/include/json-c/

nmea.cmd: nmea.cmd.c
	$(CC) nmea.cmd.c  -o nmea.cmd 

clean:
	rm -f *.o 
