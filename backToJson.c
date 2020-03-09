
#if 0
{
  "date":"2020-02-19 13:04:18.174",
  "milliSecondSinceStart":40,
  "rawData":"$GNVTG,323.60,T,,M,1.883,N,3.487,K,A*2D"
}

{
  "date":"2020-02-19 13:04:18.253",
  "milliSecondSinceStart":73,
  "rawData":"$GNGGA,130418.00,3636.06535,N,09729.25372,W,1,09,1.13,308.3,M,-26.0,M,,*75"
}

{
  "date":"2020-02-19 13:04:18.311",
  "milliSecondSinceStart":55,
  "rawData":"$GNGSA,A,3,10,11,12,32,24,31,25,,,,,,1.77,1.13,1.36*1A"
}

{
  "date":"2020-02-19 13:04:18.359",
  "milliSecondSinceStart":44,
  "rawData":"$GNGSA,A,3,69,79,,,,,,,,,,,1.77,1.13,1.36*1B"
}

{
  "date":"2020-02-19 13:04:18.432",
  "milliSecondSinceStart":70,
  "rawData":"$GPGSV,4,1,13,01,17,320,10,08,05,261,10,10,62,089,25,11,26,287,24*7C"
}

{
  "date":"2020-02-19 13:04:18.505",
  "milliSecondSinceStart":69,
  "rawData":"$GPGSV,4,2,13,12,11,073,24,20,33,117,08,21,04,164,19,22,05,304,21*7C"
}
#endif
#if 0

{
  "date":"2020-02-19 18:24:40.369",
  "milliSecondSinceStart":67,
  "rawData":"$GPGSV,3,2,11,09,47,314,08,16,63,053,,22,15,192,08,23,77,313,10*7F"
}

{
  "date":"2020-02-19 18:24:40.426",
  "milliSecondSinceStart":53,
  "rawData":"$GPGSV,3,3,11,26,35,047,,27,31,128,08,31,08,078,08*41"
}

{
  "date":"2020-02-19 18:24:40.490",
  "milliSecondSinceStart":61,
  "rawData":"$GLGSV,3,1,09,65,49,315,,71,24,167,,72,70,211,,73,21,044,*60"
}

{
  "date":"2020-02-19 18:24:40.555",
  "milliSecondSinceStart":61,
  "rawData":"$GLGSV,3,2,09,74,55,010,,75,44,269,,76,02,244,,83,09,045,*62"
}

{
  "date":"2020-02-19 18:24:40.585",
  "milliSecondSinceStart":26,
  "rawData":"$GLGSV,3,3,09,84,07,099,*57"
}

{
  "date":"2020-02-19 18:24:40.615",
  "milliSecondSinceStart":26,
  "rawData":"$GNGLL,,,,,182440.00,V,N*5F"
}

#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <json.h>
#include <json_tokener.h>
#include "serToMQTT.h"

/* turn this pseudo json back into a read json array */
int setTimeStartupCount;
int setTimeIntervalCount;

int setDateTimeFromGPS(char *s, char *s1 ) {
	return	0;
}


struct json_object *json_division(double value,char *description, char *units) {
	struct json_object *jobj = json_object_new_object();
	json_object_object_add(jobj,"value",json_object_new_double(value));
	json_object_object_add(jobj,"description",json_object_new_string(description));
	json_object_object_add(jobj,"units",json_object_new_string(units));
	return	jobj;
}


#include "protocol_NMEA0183.formatter.c"
struct json_object * _process( const char *s ) {
#if 0
	char *s_end = strrchr(s,'"');
	s = strchr(s,':');
	s = strchr(s,'"');
	s++;
	s_end[0] = '\0';
#endif
	char	buffer[256];
	strncpy(buffer,s,sizeof(buffer));
	struct json_object * jobj = do_NMEA0183_FORMAT(buffer);
	return	jobj;

}
json_object *parse_a_string(char *string ) {

	json_object *jobj = NULL;
	json_tokener *tok = json_tokener_new();
	const char *mystring = string;
	int stringlen = 0;
	enum json_tokener_error jerr;
	do {
		stringlen = strlen(mystring);
		jobj = json_tokener_parse_ex(tok, mystring, stringlen);
	} while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
	if (jerr != json_tokener_success) {
		fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
		// Handle errors, as appropriate for your application.
	}
	if (tok->char_offset < stringlen) {
		// Handle extra characters after parsed object as desired.
		// e.g. issue an error, parse another object from that point, etc...
	}
	return	jobj;
}

char	buffer[256];
char	input_string[512];
int main ( int argc, char **argv ) {

	char *p;
	while ( fgets(buffer,sizeof(buffer),stdin)) {
		int len;
		len = strlen(buffer);
		if ( 0 != ( p = strchr(buffer,'{')) ){
			strncpy(input_string,p,sizeof(input_string));
		} else  {
			strcat(input_string,buffer);
		}
		if (  0 != ( p = strchr(buffer,'}')) ){ 
			json_object *jobj = parse_a_string(input_string);
			if ( 0 != jobj ) {
				const char	*string = json_object_get_string(json_object_object_get(jobj,"rawData"));
				json_object * formatted = _process(string);
				if ( 0 != formatted ) {
					json_object_object_add(jobj,"formattedData",formatted);
				}
				fputs(json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY),stdout);
				json_object_put(jobj);
				fputc('\n',stdout);
				fputc('\n',stdout);
			}
		}
	}
				
	return	0;
}


