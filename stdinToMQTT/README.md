# stdinToMQTT

## Purpose
stdinToMQTT takes input from stdin,  packages with json and
publishes to mqtt on a topic.   The stdin is rawData in the json output and date is the timestamp.
Each line of stdin is published as an additional record.

## Installation


`sudo apt-get install mosquitto-dev`

`sudo apt-get install libjson-c-dev`

`sudo apt-get install libmosquittopp-dev`

`sudo apt-get install libssl1.0-dev`

## Build

`make`

## Command line switches

switch|Required/Optional|argument|description
---|---|---|---
--mqtt-topic|REQUIRED|topic|mqtt topic
--mqtt-host|REQUIRED|qualified host|mqtt host operating mqtt server
--mqtt-port|OPTIONAL|number|default is 1883
--mqtt-user-name|OPTIONAL|maybe required on some systems
--mqtt-password|OPTIONAL|maybe required on some systems
--disable-mqtt|OPTIONAL|(none)|prevents mqtt publishing
--quiet|OPTIONAL|(none)|turns off json packets sent to stdout
--verbose|OPTIONAL|(none)|turns on debugging to stderr
--help|OPTIONAL|(none)|displays help and exits

## Example

`df -h | grep /dev/mmcblk0p2 | ./stdinToMQTT --mqtt-host 192.168.10.221  --mqtt-topic diskSpace --quiet`

checks for disk space and publishes to diskSpace.

```
{
  "date":"2020-10-12 11:35:01.779",
  "rawData":"\/dev\/mmcblk0p2   57G   14G   40G  26% \/\n"
}
```
