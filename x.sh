#!/bin/bash


./nmea.cmd "PAMTX" > /dev/ttyUSB3
sleep 2
./nmea.cmd "PAMTC,EN,RMC,1,20" > /dev/ttyUSB3
sleep 1
./nmea.cmd "PAMTX,1" > /dev/ttyUSB3
