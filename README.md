# serToMQTT

## Purpose
serToMQTT takes input from a serial or usb port,  packages with json and
publishes to mqtt on a topic

## Installation


`sudo apt install gcc`

`sudo apt stall mosquitto-dev`

`sudo apt-get install mosquitto-dev`

`sudo apt install pkg-config`

`sudo apt-get install libmodbus-dev`

`sudo apt-get install libjson-c-dev`

`sudo apt install git`

`sudo apt-get install libmosquittopp-dev`

`sudo apt-get install libssl1.0-dev`

## Build

`make`

## Protocols

protocol id|description
---|---
NMEA0183| NMEA0183 protocol
text|text that has a format stx payload ext

## Examples 

./serToMQTT -T /toStation/A2744 -m nmea0183 -H localhost -i /dev/ttyUSB2 -b B4800

./serToMQTT -m text -i /dev/ttyUSB1 -b B115200 -H localhost -T whatever

## Command line switches

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


## Mode or protocol special handling

### NMEA0183

#### -M (options)

arg|type |description
---|---|---
stx|start of text|default='$'
etx|end of text|default=newline

#### Example

-M "stx=$ etx=^m"



### text

#### -M (options)

arg|type |description
---|---|---
stx|start of text|default='S'
etx|end of text|default=newline

#### Example

-M "stx=S etx=^m"


