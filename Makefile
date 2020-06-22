CC=gcc
CFLAGS=-I/usr/include/-I. -Wunused-function  -Wunused-variable -g

serToMQTT: serToMQTT.o protocol_text.o protocol_NMEA0183.o setDateTimeFromGPS.o protocol_FL702LT.o protocol_WINDMASTER.o \
	protocol_LOADSTAR.o protocol_YOST.o protocol_WorldData.o
	$(CC) serToMQTT.o  protocol_text.o protocol_NMEA0183.o setDateTimeFromGPS.o \
	protocol_FL702LT.o protocol_WINDMASTER.o protocol_LOADSTAR.o protocol_YOST.o \
	protocol_WorldData.o \
	-o serToMQTT $(CFLAGS)  -lm -ljson-c -lmosquitto 

serToMQTT.o: serToMQTT.c serToMQTT.h
	$(CC)  -c serToMQTT.c  $(CFLAGS) -I/usr/include/json-c/ 

protocol_text.o: protocol_text.c serToMQTT.h \
	protocol_text_iMet_XQ2.c protocol_text_TriSoncica_Mini.c
	$(CC)  -c protocol_text.c  $(CFLAGS) -I/usr/include/json-c/

protocol_NMEA0183.o: protocol_NMEA0183.c serToMQTT.h \
	 protocol_NMEA0183.formatter.c
	$(CC)  -c protocol_NMEA0183.c  $(CFLAGS) -I/usr/include/json-c/

protocol_FL702LT.o: protocol_FL702LT.c serToMQTT.h protocol_FL702LT.formatter.c
	$(CC)  -c protocol_FL702LT.c  $(CFLAGS) -I/usr/include/json-c/

protocol_WINDMASTER.o: protocol_WINDMASTER.c serToMQTT.h  protocol_WINDMASTER.formatter.c
	$(CC)  -c protocol_WINDMASTER.c  $(CFLAGS) -I/usr/include/json-c/

protocol_LOADSTAR.o: protocol_LOADSTAR.c serToMQTT.h  protocol_LOADSTAR.formatter.c
	$(CC)  -c protocol_LOADSTAR.c  $(CFLAGS) -I/usr/include/json-c/

protocol_YOST.o: protocol_YOST.c serToMQTT.h  
	$(CC)  -c protocol_YOST.c  $(CFLAGS) -I/usr/include/json-c/

protocol_WorldData.o: protocol_WorldData.c serToMQTT.h  protocol_WorldData.formatter.c
	$(CC)  -c protocol_WorldData.c  $(CFLAGS) -I/usr/include/json-c/

setDateTimeFromGPS.o: setDateTimeFromGPS.c 
	$(CC)  -c setDateTimeFromGPS.c  $(CFLAGS) -I/usr/include/json-c/

backToJson: backToJson.c protocol_NMEA0183.formatter.c
	$(CC) backToJson.c  -o backToJson $(CFLAGS)  -lm -ljson-c -lmosquitto -I/usr/include/json-c/

nmea.cmd: nmea.cmd.c
	$(CC) nmea.cmd.c  -o nmea.cmd 

clean:
	rm -f *.o 
