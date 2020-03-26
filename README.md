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

`./serToMQTT --mqtt-topic  /toStation/A2744 --protocol nmea0183 --mqtt-host localhost --input-port /dev/ttyUSB2 --input-speed 4800`

`./serToMQTT --protocol text --input-port /dev/ttyUSB1 --input-speed 230400 --mqtt-host localhost --mqtt-topic whatever -M format=TRI`

 `./serToMQTT --mqtt-topic /toStation/A2744 --protocol text -mqtt-host localhost --input-port /dev/ttyUSB0 --input-speed 57600 --special-handling "stx=X etx=0x0d format=XQ" -t 1024`

## Command line switches

switch|Required/Optional|argument|description
---|---|---|---
--mqtt-topic|REQUIRED|topic|mqtt topic
--mqtt-neta-topic|OPTIONAL|topic|mqtt meta-topic
--mqtt-protocol|REQUIRED|see above|protocol id 
--mqtt-host|REQUIRED|qualified host|mqtt host operating mqtt server
--input-port|REQUIRED|full path to device|input serial port
--alarm-no-data-after-start|OPTIONAL|seconds|Terminate after seconds without data
--timeout|OPTIONAL|milliseconds|Timeout packet after milliseconds since start
--sleep-before-startup|OPTIONAL|seconds|startup delay
--input-speed|OPTIONAL|number|baud rate of serial port default is 4800
--verbose|OPTIONAL|(none)|sets verbose mode
--mqtt-port|OPTIONAL|number|default is 1883
--special-handling|OPTIONAL|single arg|See below
--disable-mqtt|OPTIONAL|(none)|prevents mqtt publishing
--no-meta|OPTIONAL|(none)|turns off output of meta-data
--quiet|OPTIONAL|(none)|turns off json packets sent to stdout
--help|OPTIONAL|(none)|displays help and exits

## Mode or protocol special handling

### NMEA0183

#### --special-handling (options)

arg|type |description
---|---|---
stx|start of text|default='$'
etx|end of text|default=newline
format|format name|"NMEA"
setTimeStartupCount|integer|number of time to set systime to GPS
setTimeIntervalCount|integer|resync after integer packets RMC

#### Example

--special-handling "stx=$ etx=^m"

Visually this looks correct but special characters must be escaped to prevent the shell from
mishandling.   In bash $ should be enterd stx=`'$'`   and newline should be entered as `control-M <enter>`.



### text

#### --special-handling (options)

arg|type |description
---|---|---
stx|start of text|default='S'
etx|end of text|default=newline

#### Example

`--special-handling "stx=S etx=^m"`

       or

`--special-handling "stx=X etx=0x0d"`

Visually this looks correct but special characters must be escaped to prevent the shell from
mishandling.   In bash $ should be enterd stx=`'$'`   and newline should be entered as `control-M <enter>`.

As an easier alternative you can specify both stx and etx in hexidecimal.   Where newline =0x0a   and cariage return = 0x0d


## InterMet iMet-XQ2

Command line in the lab looks like:

` ./serToMQTT --mqtt-topic /toStation/A2744 --protocol text --mqtt-host localhost --input-port /dev/ttyUSB0 --input-speed 57600 --special-handling "stx=X etx=0x0d format=XQ " --timeout 1280`

The critcal difference is specifying the stx and etx using the -M option and specifying the packet time out.   The time out by 
default is 500 mSeconds.   This device seems to be sending packets once per second so the need for 1280 mSeconds.   This
was determined experimetally.

## TriSonica Mini

Command line in the lab looks like:

`./serToMQTT --protocol text --input-port /dev/ttyUSB1 --input-speed 115200 --mqtt-host localhost --mqtt-topic /toStation/A2744 --special-handling format=TRI` 

The defaults of stx='S' and etx=0x0a are used, and need not be specified.

## NMEA0183 instruments

Command line in the lab looks like:

`./serToMQTT --mqtt-topic /toStation/A2744 --protocol nmea0183 --mqtt-host localhost --input-port /dev/ttyUSB2 --input-speed 4800 --special-handling format=NMEA`

The defaults of stx='$' and etx=newline are used, and need not be specified.


# nmea.cmd

## purpose

The nmea.cmd purose is to format the NMEA0183 command to be sent to a device.   This can be difficult to do in
a Linux system as terminals and other programs end records with newline rather CrLf.   Also devices receiving the
the commands are expecting the checksum.

## use

`./nmea.cmd "PAMTC,EN,VWT,1,20"` 

formats the PAMTC commmand correctly, puts the '$' prefix on the command and
computes the checksum.   Then it applies the CrLf to the end.

In use in a shell script:

`
./nmea.cmd "PAMTX" > /dev/ttyUSB3

sleep 2

./nmea.cmd "PAMTC,EN,VWT,1,20" > /dev/ttyUSB3

sleep 1

./nmea.cmd "PAMTX,1" > /dev/ttyUSB3
`

This turns off sending from the device, enables the VWT command, and then turns on sending from the device.


