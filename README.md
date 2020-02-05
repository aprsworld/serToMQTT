# serToMQTT

## Purpose
serToMQTT takes input from a serial or usb port,  packages with json and
publishes to mqtt on a topic

## Installation


`sudo  apt install gcc`
`stall mosquitto-dev`
`sudo apt-get install mosquitto-dev`
`sudo apt install pkg-config`
`sudo apt-get install libmodbus-dev`
`sudo apt-get install libjson-c-dev`
`sudo apt-get install xclip`
`sudo apt install git`
`sudo apt-get install libmosquittopp-dev`
`sudo apt-get install libssl1.0-dev`

##Protocols
protocol id|description
NMEA0183| NMEA0183 protocol
text|text that has a format stx payload ext

## Examples 

./serToMQTT -T /toStation/A2744 -m nmea0183 -H localhost -i /dev/ttyUSB2 -b B4800
./serToMQTT -m text -i /dev/ttyUSB1 -b B115200 -H localhost -T whatever

switch|Required/Optional|argument|description
---|---|---|---
-T|REQUIRED|topic|mqtt topic
-m|REQUIRED|see above|protocol id 
-H|REQUIRED|qualified host|mqtt host operating mqtt server
-i|REQUIRED|full path to device|input serial port
-b|OPTIONAL|number|baud rate of serial port default is 4800
-v|OPTIONAL|(none)|sets verbose mode
-P|OPTIONAL|number|default is 1883
-M|OPTIONAL|single arg|See below
-h|OPTIONAL|(none)|displays help and exits


-v	verbose  OPTIONAL
-P	mqtt port   OPTIONAL
-M	mode special handling  OPTIONAL "stx=# etx=^M"   If more than one then must be grouped on
        command line and special characters must be escaped.   Each special is of the format
        var=value.  The specials are separated by wite space (' ',\n,\r,\t).   each mode
	can have its own special handling.  for -m text then -M can be "stx=# etx=^M".
-t	timeout packet after %d milliseconds since start  OPTIONAL
-s	Delaying startup for %d seconds OPTIONAL
-h	help OPTIONAL


