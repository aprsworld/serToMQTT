# GPSsetSYSTIME

## Purpose

To use the gps in a PI to set the system time when the PI is not on a network that has a timeserver.


## Installation

Not libraries that I know of beyond the the basic c development system are needed.


## Build

`make`

## Command line switches

switch|Required/Optional|argument|description
---|---|---|---
--input-port|REQUIRED|full path to device|input serial port
--input-speed|OPTIONAL|number|baud rate of serial port default is 9600
--verbose|OPTIONAL|(none)|sets verbose mode
--help|OPTIONAL|(none)|displays help and exits

## Privleges

You must have permssion to set the system time.   Use sudo su when in doubt.


## Example usage.

`GPSsetSYSTIME --input-port /dev/tthS0 --input-speed 9600 --verbose`

## Discussion

The PI GPS does not report its result with high precission.  Therefore systime will dither arond
the correct value.   I do not think it is useful to run this program in an endless loop but running at boot or
as a crontab task say once an hour are probably just fine.

## Disabling network timesync


`sudo systemctl stop systemd-timesyncd`

`sudo systemctl disable systemd-timesyncd` 
