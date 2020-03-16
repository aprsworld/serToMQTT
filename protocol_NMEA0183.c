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


static int local_stx = '$';
static int local_etx = '\n';
static int _NMEA_FORMAT;
static int setTimeStartupCount;
static int setTimeIntervalCount;



extern char *optarg;
extern int optind, opterr, optopt;

extern int outputDebug;
extern int milliseconds_timeout;
extern int alarmSeconds;


#define DATA_BLOCK_N 32 /* maximum number of NMEA sentences */

typedef struct {
	uint64_t microtime_start;	/* time when STX was received*/
	uint8_t sentence[128];		/* full sentence ('$' to second character of checksum), null terminated */
} struct_data_block;
#define DUMP_CURRENT_JSON_SUGGESTED_SIZE (DATA_BLOCK_N * 2 * sizeof(data_block[0].sentence))

/* array of data_blocks to put Magnum network data into */
struct_data_block data_block[DATA_BLOCK_N];




#include <sys/time.h>
#include "protocol_NMEA0183.formatter.c"


int nmea_packet_processor(char *packet, int length, uint64_t microtime_start, int millisec_since_start) {
	int rc = 0;
	int i;
	int lChecksum,rChecksum;
	const char	*topic;

	/* quick sanity check */
	if ( length < 9 )
		return rc;

	/* null terminate so we can use string functions going forwards */
	packet[length]='\0';

	

	/* calculate local checksum */
	lChecksum=0;
	for ( i=1 ; i<length-3 ; i++ ) {
		lChecksum = lChecksum ^ packet[i];
	}

	/* read remote checksum */
	if ( 1 != sscanf(packet+length-2,"%x",&rChecksum) ) {
		if ( outputDebug ) {
			printf("(error scanning remote checksum hex)\n");
		}
		return rc;
	}

	/* compare local and remote checksums */
	if ( lChecksum != rChecksum ) {
		if ( outputDebug ) {
			printf("(remote and local checksum do not match!)\n");
		}
		return rc;
	}

	struct json_object *jobj;

	jobj = json_object_new_object();
	json_object_object_add(jobj,"date",json_object_new_dateTime());

	json_object_object_add(jobj,"milliSecondSinceStart",json_object_new_int(millisec_since_start));
	json_object_object_add(jobj,"rawData",json_object_new_string(packet));

	if ( 0 != _NMEA_FORMAT ) {
		json_object_object_add(jobj,"formattedData", do_NMEA0183_FORMAT(packet));
		topic = new_topic(packet,mqtt_topic);
	}

	// fprintf(stderr,"# %s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));
	rc = serToMQTT_pub(json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY),topic);
	json_object_put(jobj);

	return	rc;
}

enum states {
	STATE_LOOKING_FOR_STX,
	STATE_IN_PACKET,
};


extern uint64_t microtime(); 

static int  _serial_process(int serialfd) {
	int rc = 0;
	int i,n;
	uint64_t microtime_now;
	int milliseconds_since_stx;
	char buff[1];


	static char packet[128];
	static int packet_pos=0;
	static uint64_t microtime_start=0;
	static int state=STATE_LOOKING_FOR_STX;

	n = read (serialfd, buff, sizeof(buff));  // read next character if ready
	microtime_now=microtime();


	/* non-blocking, so we will get here if there was no data available */
	/* read timeout */
	if ( 0 == n ) {
		if ( outputDebug ) {
			fprintf(stderr,"# (read returned 0 bytes)\n");
		}
		return rc;
	}

	/* cancel pending alarm */
	alarm(0);
	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	alarm(alarmSeconds);

	milliseconds_since_stx=(int) ((microtime_now-microtime_start)/1000.0);


	/* NMEA packets:
		start with '$'
		end with '\r' or '\n'
		get aborted on timeout
	*/

	/* copy byte to packet */
	for ( i=0 ; 0 == rc && i<n ; i++ ) {
		/* look for start character */
		if ( STATE_LOOKING_FOR_STX == state && '$' ==  buff[i] ) {
			packet_pos=0;
			microtime_start=microtime_now;
			state=STATE_IN_PACKET;
			packet[0]='$';
		}

		if ( STATE_IN_PACKET == state ) {
			if ( milliseconds_since_stx > milliseconds_timeout ) {
				packet_pos=0;
				state=STATE_LOOKING_FOR_STX;

				if ( outputDebug ) {
					printf("(timeout while reading NMEA sentence)\n");
				}
				continue;
			}
	
			if ( '\r' == buff[i] || '\n' == buff[i] ) {
				state=STATE_LOOKING_FOR_STX;
				alarm(0);
				/* process packet */
				rc = nmea_packet_processor(packet,packet_pos,microtime_start,milliseconds_since_stx);
			}

			if ( packet_pos < sizeof(packet)-1 ) {
				packet[packet_pos]=buff[i];
				packet_pos++;
			} else {
				if ( outputDebug ) {
					printf("(packet length exceeded length of buffer!)\n");
				}
			}

		}

	}

return	rc;
}
static void _do_format(char *s ) {
	if ( 0 == strcmp(s,"NMEA" )) {
		_NMEA_FORMAT = 1;
	} else {
		fprintf(stderr,"# Bad format %s\n",s);
		fprintf(stderr,"# NMEA0183_FORMAT=NMEA\n");
		exit(1);
	}


}
static void _do_command( char * s) {
	/* s contains one command of the type  $cmd=$parameter */
	char	buffer[256] = {};
	char	*p,*q;

	strncpy(buffer,s,sizeof(buffer) - 1);

	q= buffer;
	p = strsep(&q,"=");
	/* p is the command and q is the paramenter */
	if ( 0 == strcmp(p,"stx")) {
		local_stx = q[0];
	}
	else if ( 0 == strcmp(p,"etx")) {
		local_etx = q[0];
	} else if ( 0 == strcmp(p,"format")) {
		_do_format(q);
	} else if ( 0 == strcmp(p,"setTimeStartupCount")) {
		setTimeStartupCount = atoi(q);
	} else if ( 0 == strcmp(p,"setTimeIntervalCount")) {
		setTimeIntervalCount = atoi(q);
	} else {
	fprintf(stderr,"# -M option '%s' not supported.\n",s);
	fprintf(stderr,"# [stx, ext ,format ,setTimeStartupCount ,setTimeIntervalCount ]\n");
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
while ( (p = strsep(&q," \t\n\r")) && ('\0' != p[0]) )	// this will find zero or more commands
	_do_command(p);
}

void nmea0183_engine(int serialfd,char *special_handling ) {
	int i;
	int rc = 0;

	_overRide(special_handling);

	set_blocking (serialfd, 1, 0);		// blocking wait for a character
	/* server socket */
	fd_set active_fd_set, read_fd_set;

	FD_ZERO( &active_fd_set);


	/* add serial fd to fd_set for select() */
	FD_SET(serialfd, &active_fd_set);

	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	//alarm(alarmSeconds);


	for ( ; 0 == rc ; ) {
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;


		i=select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);
		if ( EBADF == i ) {
			fprintf(stderr,"# select() EBADF error. Aborting.\n");
			exit(1);
		} else if ( ENOMEM == i ) {
			fprintf(stderr,"# select() ENOMEM error. Aborting.\n");
			exit(1);
		} 

		if ( FD_ISSET(serialfd, &read_fd_set) ) {
			rc = _serial_process(serialfd);
		} 
	}

}
