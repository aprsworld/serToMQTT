CC=gcc
CFLAGS=-I./json-c-0.14 -I/usr/include/ -I. -Wunused-function  -Wunused-variable -g
LDFLAGS=-Ljson-c-build

serToMQTT: longsize.h serToMQTT.o protocol_text.o protocol_NMEA0183.o setDateTimeFromGPS.o protocol_FL702LT.o protocol_WINDMASTER.o \
	protocol_LOADSTAR.o protocol_YOST.o protocol_WorldData.o json_division.o
	$(CC) serToMQTT.o  protocol_text.o protocol_NMEA0183.o setDateTimeFromGPS.o \
	protocol_FL702LT.o protocol_WINDMASTER.o protocol_LOADSTAR.o protocol_YOST.o \
	protocol_WorldData.o json_division.o \
	-o serToMQTT $(CFLAGS) $(LDFLAGS)  -lm -ljson-c -lmosquitto 

longsize.h: longsize.c
	$(CC) longsize.c -o longsize
	echo "#define LONGSIZE `./longsize`" > longsize.h

serToMQTT.o: serToMQTT.c serToMQTT.h
	$(CC)  -c serToMQTT.c  $(CFLAGS) -I/usr/include/json-c/ 

json_division.o: json_division.c 
	$(CC)  -c json_division.c  $(CFLAGS) -I/usr/include/json-c/ 

protocol_text.o: protocol_text.c serToMQTT.h \
	protocol_text_iMet_XQ2.c protocol_text_TriSoncica_Mini.c protocol_text_TriSoncica_Sphere.c
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

protocol_WorldData.o: protocol_WorldData.c serToMQTT.h  protocol_WorldData.formatter.c longsize.h
	$(CC)  -c protocol_WorldData.c  $(CFLAGS) $(LDFLAGS) -I/usr/include/json-c/

setDateTimeFromGPS.o: setDateTimeFromGPS.c 
	$(CC)  -c setDateTimeFromGPS.c  $(CFLAGS) -I/usr/include/json-c/

backToJson: backToJson.c protocol_NMEA0183.formatter.c
	$(CC) backToJson.c  -o backToJson $(CFLAGS)  -lm -ljson-c -lmosquitto -I/usr/include/json-c/

nmea.cmd: nmea.cmd.c
	$(CC) nmea.cmd.c  -o nmea.cmd 

test_jsonToAPCALC: test_jsonToAPCALC.c jsonToAPCALC.c json_division.o
	$(CC) test_jsonToAPCALC.c jsonToAPCALC.c -o test_jsonToAPCALC  json_division.o $(CFLAGS) $(LDFLAGS) -ljson-c -I/usr/include/json-c/

GPSsetSYSTIME: GPSsetSYSTIME.c setDateTimeFromGPS.o
	$(CC) GPSsetSYSTIME.c -o GPSsetSYSTIME  $(CFLAGS) $(LDFLAGS) setDateTimeFromGPS.o

clean:
	rm -f *.o 
