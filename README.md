# serToMQTT

examples 

./serToMQTT -T /toStation/A2744 -m nmea0183 -H localhost -i /dev/ttyUSB2 -b B4800
./serToMQTT -m text -i /dev/ttyUSB1 -b B115200 -H localhost -T whatever


-T	mqtt topic	REQUIRED
-m	mode or protocol NMEA0183 or text	REQUIRED
-H	mqtt host	REQUIRED
-i	input port	REQUIRED
-b	input baud	REQUIRED
-v	verbose  OPTIONAL
-P	mqtt port   OPTIONAL
-M	mode special handling  OPTIONAL "stx=# etx=^M"   If more than one then must be grouped on
        command line and special characters must be escaped.   Each special is of the format
        var=value.  The specials are separated by wite space (' ',\n,\r,\t).   each mode
	can have its own special handling.  for -m text then -M can be "stx=# etx=^M".
-t	timeout packet after %d milliseconds since start  OPTIONAL
-s	Delaying startup for %d seconds OPTIONAL
-h	help OPTIONAL


