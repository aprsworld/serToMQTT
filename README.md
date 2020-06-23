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
NMEA0183|NMEA0183 protocol
text|text that has a format stx payload ext
fl702lt|one or more wx stations on a single port,
windmaster|Gil Windmaster
loadstar|LoadStar load cell.   Just outputs a series of numbers
yost|polls a Yost 3-space sensor and report angle of inclination
worlddata|interface to any device that uses a port with WorldData formats.


## Examples 

`./serToMQTT --mqtt-topic  /toStation/A2744 --protocol nmea0183 --mqtt-host localhost --input-port /dev/ttyUSB2 --input-speed 4800`

`./serToMQTT --protocol text --input-port /dev/ttyUSB1 --input-speed 230400 --mqtt-host localhost --mqtt-topic whatever -M format=TRI`

 `./serToMQTT --mqtt-topic /toStation/A2744 --protocol text -mqtt-host localhost --input-port /dev/ttyUSB0 --input-speed 57600 --special-handling "stx=X etx=0x0d format=XQ" -t 1024`

`./serToMQTT --protocol loadstar --mqtt-host 192.168.10.80 --mqtt-topic left --input-port /dev/ttyUSB2 --input-speed 230400 --quiet --mqtt-meta-topic meta-left`

`./serToMQTT --mqtt-host 192.168.10.221 --mqtt-topic World_Data --protocol worlddata --input-port /dev/ttyUSB3 --input-speed 9600 --special-handling format=XRW2G --mqtt-meta-topic Meta_World_Data --quiet`

## Command line switches

switch|Required/Optional|argument|description
---|---|---|---
--mqtt-topic|REQUIRED|topic|mqtt topic
--mqtt-meta-topic|OPTIONAL|topic|mqtt meta-topic
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

## FL702LT instruments

Command line in the lab looks like:

`./serToMQTT --mqtt-topic /toStation/A2744 --protocol FL702LT --mqtt-host localhost --input-port /dev/ttyUSB2 --input-speed 9600 --station "talk=WI listener=01 interval=1000"`

The defaults of stx='$' and etx=newline are used, and need not be specified.

--station must be used once but can be used more than once if there are multiple instruments on the same port.   Each talker and listener must be unique.   The interval is in milliseconds and intervals can duplicate.   Optionally there is 'startup=' inside of each --station.   To use NMEA format `--station "talker=WI listener=01 interval=1000 startup=DFN"`.  To use polar format `--station "talker=WI listener=01 interval=1000 startup=DFP"`.  Each station does not have to use the same format.  Remember that you must use something in the shell or script to group all of the station parameters.


## WINDMASTER instruments

Command line in the lab looks like:

 ./serToMQTT --protocol windmaster --mqtt-host 192.168.10.80 --mqtt-topic data/006 --input-port=/dev/ttyUSB0 --input-speed=19200 --mqtt-meta-topic meta/006 --quiet

The defaults of stx='\002' and etx='\003' are used, and need not be specified.

### WINDMASTER setup

These devices need to be setup once or when their parameters need to be changed.   To get the device into setup mode:

1.  Make sure that there is only one device attached to the serial bus.

2.  Start up minicom and using `^AZ` enter the setup menu.   Set the port to the desired port.  You may need to use the dmesg command
to figure this out.   Set the parameters to n81 and the default baud of 19200.   

3.  Press `*` and one of two things will happen.  The words `CONFIGURATION MODE` will apppear or `*` will appear.    You are now in configuration mode.

4.  Enter `M1` and press enter.   `M1` should echo back.   This sets the device so it will return the `M1` message type.

5.  Enter `P8` and press enter.  `P8` should echo back.  This set the sampling rate to 20Hz.

6.  When in running mode if you will have more than one device on the serial bus then you should assign each device a unique ID
if you want to be able to tell which device the reading came from.    To check the current ID enter `N` and press enter.  It will 
echo `NQ` or the second letter maybe any letter in the alphabet.   To change the ID to `R` then enter `NR` and press enter.

7.  If #6 above is true then you may need to up the baud rate.  `B6` will set the baud to 57600.   You must then change the setting
in minicom to 57600 also.   Then Enter `B` and press enter.   All devices on the serial bus need to be at the same baud and parameters.

8.  Enter `Q` and press enter.   A configuration will display and then the device will spew data.

## LOADSTAR instruments

Command line in the lab looks like:

./serToMQTT --protocol loadstar --mqtt-host 192.168.10.80 --mqtt-topic left --input-port /dev/ttyUSB2 --input-speed 230400 --quiet --mqtt-meta-topic meta-left

STX and ETX are built in and if specified on the command line will be ignored.

### LOADSTAR setup

These devices need to be setup once or when their parameters need to be changed.   To get the device into setup mode:

1.  Start up minicom and using `^AZ` enter the setup menu.   Set the port to the desired port.  You may need to use the dmesg command
to figure this out.   Set the parameters to n81 and the default baud of 230400.   

2.  Press enter.  An 'A' or 'E' will appear.  By default the device is not in continuous mode.

3.  Enter `UNIT LB` for punds, or `UNIT KG` for Kilograms, or `UNIT N` for Newtons.   All inputs/outputs after this will be in the 
unit selected.

4.  To check your work enter `UNIT`.

5.  To calibrate you must read the fine manual.

6.  To set the sampling rate enter `SPS`.   This will report back the current sampling rate.  Slower rates produce more accuracy.
  Sampling rates are 7.5, 15, 30, 60, 120, 240, 480, 960, 1920, 3840. Factory default is 120.  These are SamplesPerSecond or Hz.
The current version that we have cannot adjust the sampling rate.   It will appear to work but when you turn on WC it still goes
fast.   This tends to saturate the network.   You can use --special-handling hertz=4.  You can use any positive number but the
upper limit is what the exquipment puts out.   Choosing super big numbers is meaningless. 

7.  To see the current value on the device enter `W`.   If this is not correct then you must read the fine manual.  

8.  To see the stream of data at the sampling rate enter `WC`.   Data will be written to the screen at the rate selected.

9.  To exit minicom enter `^AQ`.

10.  When all else fails read the fine manual.

## YOST

Command line in the lab looks like:

`./serToMQTT --mqtt-host localhost --mqtt-topic Yost --protocol yost --input-port /dev/ttyACM0 --input-speed 115200 --special-handling hertz=20`

You do not need to know how to program a yost.  As long as you have the --input-port and --input-speed then this software makes
sure the device is in polling mode and polls the device for the corrected 3-axis accellerometer.  The defaulti polling rate is 
10 Hertz.  Higher or lower rates can be set by using --special-handling hertz=40.   

## WorldData

`./serToMQTT --mqtt-host 192.168.10.221 --mqtt-topic World_Data --protocol worlddata --input-port /dev/ttyUSB3 --input-speed 9600 --special-handling format=XRW2G --mqtt-meta-topic Meta_World_Data --quiet`


WorldData requires the use of `--special-handling` because there are several different formats for WorldData.

format|description
---|---
XRW2G|The XBee, RS-232, and USB versions of the XRW2G module.

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


