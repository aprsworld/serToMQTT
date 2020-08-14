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
#include "serToMQTT.h"
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

/* STX is the start of the record.   ETX is the end of the paylocal   crlf is end of record */
/* checksum immediately follows EYX and is immediately followed by crlf */
#define STX ' '
#define ETX '\n'




int _YOST_FORMAT = 1;

static double _atof(const char *s ) {
	if ( '\0' == s[0] ) {
		return	NAN;
	}
	return	atof(s);
}
static struct json_object *_YOST( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	static int _Count;
	char *p,*q;
	double x,y,z;
	double dem;
	double angle;
	double pi = 3.14159265358979323846;	/* exceeds the precission we need */
	double rad2ang = 180.0 / pi;

	if (  0 == _Count ) {
		retainedFlag = 1;
	}

	_Count++;

	jobj = json_object_new_object();
	strncpy(buffer,s,sizeof(buffer));

	q = buffer;
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"Xaxis",
		json_division(x = _atof(p),"accelleration","G"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"Yaxis",
		json_division(y = _atof(p),"accelleration","G"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"Zaxis",
		json_division(z = _atof(p),"accelleration","G"));
	/* we are now computing the angles of inclination where 90 is vertial and 0 is zorizontal */
	dem = x*x + y*y + z * z;
	dem = sqrt(dem);
	angle = 90.0 - (acos(z/dem)*rad2ang);
	json_object_object_add(jobj,"angleOfInclination",
		json_division(angle ,"angle of inclination 90 == vertical","degrees"));
	parse_failed:
	return jobj;
}

	
static struct json_object *do_YOST_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();

	alarm(0);
	json_object_object_add(jobj,"YOST",_YOST(s));	
	return	jobj;
}

static int _yost_packet_processor(char *packet, int length, uint64_t microtime_start, int millisec_since_start) {
	int rc = 0;
	const char	*topic = mqtt_topic;

	/* quick sanity check */
	if ( length < 7 )
		return rc;

	/* null terminate so we can use string functions going forwards */
	packet[length]='\0';

	struct json_object *jobj;
	struct json_object *tmp;

	tmp = ( 0 != _YOST_FORMAT ) ?   do_YOST_FORMAT(packet) : 0;
	jobj = json_object_new_object();
	if ( 0 == retainedFlag ) {
		json_object_object_add(jobj,"date",json_object_new_dateTime());
		json_object_object_add(jobj,"milliSecondSinceStart",json_object_new_int(millisec_since_start));
		json_object_object_add(jobj,"rawData",json_object_new_string(packet));
	}

	if ( 0 != tmp ) {
		json_object_object_add(jobj,"formattedData", tmp);
		topic = new_topic("$YOST  ",( 0 == retainedFlag ) ? mqtt_topic : mqtt_meta_topic);
	}

	// fprintf(stderr,"# %s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));
	rc = serToMQTT_pub(json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY),topic);
	json_object_put(jobj);

	return	rc;
}

static int _yost_hertz = 10;

int next_hertz(struct timeval *real_time, struct timeval *trigger_time ) {
        static uint64_t hertz_interval ;

        if ( 0 == hertz_interval ) {
                hertz_interval =  ((uint64_t)1000000/_yost_hertz);
        }


        if ( 0  == trigger_time->tv_sec ) {
                trigger_time->tv_sec = real_time->tv_sec +1;    /* start at the next second */
                trigger_time->tv_usec = 0;
                return true;
        }

        uint64_t t1,t2;
        t1 = ((uint64_t)real_time->tv_sec * 1000000) + real_time->tv_usec;
        t2 = ((uint64_t)trigger_time->tv_sec * 1000000) + trigger_time->tv_usec;
        if ( t1 < t2 ) {
                return  true;
        }
        /*   we are triggered */
        if ( 1 == _yost_hertz ) {
                while ( t1 > t2 ) {
                        trigger_time->tv_sec++;
                        t2 = ((uint64_t)trigger_time->tv_sec * 1000000) + trigger_time->tv_usec;
                }
        }
        else {
                t2 += hertz_interval;
                trigger_time->tv_sec = t2 / 1000000;
                trigger_time->tv_usec = t2 % 1000000;
        }

        return  false;  /* we have been triggered. */

}

static void _do_poll(FILE *in_out ) {
	char buffer[64];
	double x,y,z;
	struct timeval time;
	static struct timeval hertz_tv;
	uint64_t microtime_now;
	int milliseconds_since_stx;
	char packet[128];
	uint64_t microtime_start=0;
	int rc;
	int bad_count = 0;



	do {
		gettimeofday(&time, NULL);

		memset(buffer,'\0',sizeof(buffer));
		alarm(0);	/* for debugging */
		while ( true == next_hertz(&time,&hertz_tv)) {
			usleep(1000);
			gettimeofday(&time, NULL);
			}
		fputs(":39\r\n",in_out);
		fflush(in_out);
		microtime_now=microtime();
		usleep(1000);
		fgets(buffer,sizeof(buffer),in_out);
		milliseconds_since_stx = (microtime() - microtime_now) / 1000;
		if ( 3 == sscanf(buffer,"%lf,%lf,%lf",&x,&y,&z) ) {
			snprintf(packet,sizeof(packet),"$YOST,%lf,%lf,%lf",x,y,z);
			rc = _yost_packet_processor(packet,strlen(packet),microtime_start,milliseconds_since_stx);
			bad_count = 0;
		} else {
			bad_count++;
		}
	} while (10 > bad_count );


}

static void _do_command( char * s)
{
	/* s contains one command of the type  $cmd=$parameter */
	char	buffer[256] = {};
	char	*p,*q;

	strncpy(buffer,s,sizeof(buffer) - 1);

	q= buffer;
	p = strsep(&q,"=");
	/* p is the command and q is the paramenter */
	if ( 0 == strcmp("hertz",p) ) {
		_yost_hertz = atoi(q);
		if ( 0 >= _yost_hertz ) {
			fprintf(stderr,"# bad special-hanlding %s\n",s);
			exit(1);
		}
	} else {
		fprintf(stderr,"# --special-handling option '%s' not supported.\n",s);
		exit(1);
	}
}
static void _overRide(char *s )
{
	/*  s can contain commands of the type  $cmd=$parameter with one or more commands sepearted by whitespace */
	char	buffer[256] = {};
	char	*p,*q;

	strncpy(buffer,s,sizeof(buffer) - 1);

	q= buffer;
	/* this will find zero or more commands */
	while ( (p = strsep(&q," \t\n\r")) && ('\0' != p[0]) )	{
		_do_command(p);
	}
}
void yost_engine(int serialfd,char *special_handling ) {

	_overRide(special_handling);

	set_blocking (serialfd, 1, 0);		// blocking wait for a character

	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	alarm(alarmSeconds);


	FILE *in_out;
	in_out = fdopen(serialfd,"r+");
	if ( 0 == in_out ) {
		fprintf(stderr,"# cannot open fdopen\n");
		exit(1);
	}
	fputs(":86\r\n",in_out);	/* turn off continuous feed because we are going to poll */
	_do_poll(in_out);
	fclose(in_out);
}
