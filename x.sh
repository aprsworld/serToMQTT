#!/bin/bash

PROG="./serToMQTT"
HOST="192.168.10.221"
TOPIC="World_Data"
PROTOCOL="worlddata"
PORT="/dev/ttyUSB0"
SPEED="9600"
SPECIAL='format=XRW2G xrw2g_pulseTimeAnemometer(0, 0.5, 1.0, "Reference Anemometer", "m/s") xrw2g_pulseTimeAnemometer(1, 0.7, 4.0, "Imperial Anemometer", "m/h") xrw2g_pulseCountAnemometer(0, 0.5, 1.0, "Reference Anemometer", "m/s")'
QUIET="--quiet"


/bin/echo r --mqtt-host $HOST  --mqtt-topic $TOPIC --protocol $PROTOCOL --input-port $PORT --input-speed $SPEED \
	--special-handling \'$SPECIAL\'  $QUIET

/usr/bin/gdb $PROG


