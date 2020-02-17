# serToMQTT

## Purpose
serToMQTT takes input from a serial or usb port,  packages with json and
publishes to mqtt on a topic

## Installation


`sudo apt-get install mosquitto-dev`

`sudo apt-get install libjson-c-dev`

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

`./serToMQTT -T /toStation/A2744 -m nmea0183 -H localhost -i /dev/ttyUSB2 -b 4800`

`./serToMQTT -m text -i /dev/ttyUSB1 -b 230400 -H localhost -T whatever -M format=TRI`

 `./serToMQTT -T /toStation/A2744 -m text -H localhost -i /dev/ttyUSB0 -b 57600 -M "stx=X etx=0x0d format=XQ" -t 1024`

## Command line switches

switch|Required/Optional|argument|description
---|---|---|---
-T|REQUIRED|topic|mqtt topic
-m|REQUIRED|see above|protocol id 
-H|REQUIRED|qualified host|mqtt host operating mqtt server
-i|REQUIRED|full path to device|input serial port
-a|OPTIONAL|seconds|Terminate after seconds without data
-t|OPTIONAL|milliseconds|Timeout packet after milliseconds since start
-s|OPTIONAL|seconds|startup delay
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

Visually this looks correct but special characters must be escaped to prevent the shell from
mishandling.   In bash $ should be enterd stx=`'$'`   and newline should be entered as `control-M <enter>`.



### text

#### -M (options)

arg|type |description
---|---|---
stx|start of text|default='S'
etx|end of text|default=newline

#### Example

`-M "stx=S etx=^m"`

       or

`-M "stx=X etx=0x0d"`

Visually this looks correct but special characters must be escaped to prevent the shell from
mishandling.   In bash $ should be enterd stx=`'$'`   and newline should be entered as `control-M <enter>`.

As an easier alternative you can specify both stx and etx in hexidecimal.   Where newline =0x0a   and cariage return = 0x0d


## InterMet iMet-XQ2

Command line in the lab looks like:

` ./serToMQTT -T /toStation/A2744 -m text -H localhost -i /dev/ttyUSB0 -b 57600 -M "stx=X etx=0x0d format=XQ " -t 1280`

The critcal difference is specifying the stx and etx using the -M option and specifying the packet time out.   The time out by 
default is 500 mSeconds.   This device seems to be sending packets once per second so the need for 1280 mSeconds.   This
was determined experimetally.

## TriSonica Mini

Command line in the lab looks like:

`./serToMQTT -m text -i /dev/ttyUSB1 -b 115200 -H localhost -T /toStation/A2744 -M format=TRI` 

The defaults of stx='S' and etx=0x0a are used, and need not be specified.

## NMEA0183 instruments

Command line in the lab looks like:

`./serToMQTT -T /toStation/A2744 -m nmea0183 -H localhost -i /dev/ttyUSB2 -b 4800`

The defaults of stx='$' and etx=newline are used, and need not be specified.


