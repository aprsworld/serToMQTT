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

/* STX is the start of the record.   ETX is the end of the paylocal   crlf is end of record */
/* checksum immediately follows EYX and is immediately followed by crlf */
#define STX ' '
#define ETX '\n'




int _LOADSTAR_FORMAT = 1;
static int loadStarSamplesPerSecond;
static char loadStarUNIT[16];

#include "protocol_LOADSTAR.formatter.c"

static int _loadStar_packet_processor(char *packet, int length, uint64_t microtime_start, int millisec_since_start) {
	int rc = 0;
	const char	*topic = mqtt_topic;

	/* quick sanity check */
	if ( length < 7 )
		return rc;

	/* null terminate so we can use string functions going forwards */
	packet[length]='\0';

	struct json_object *jobj;
	struct json_object *tmp;

	tmp = ( 0 != _LOADSTAR_FORMAT ) ?   do_LOADSTAR_FORMAT(packet) : 0;
	jobj = json_object_new_object();
	if ( 0 == retainedFlag ) {
		json_object_object_add(jobj,"date",json_object_new_dateTime());
		json_object_object_add(jobj,"milliSecondSinceStart",json_object_new_int(millisec_since_start));
		json_object_object_add(jobj,"rawData",json_object_new_string(packet));
	}

	if ( 0 != tmp ) {
		json_object_object_add(jobj,"formattedData", tmp);
		topic = new_topic("$WC    ",( 0 == retainedFlag ) ? mqtt_topic : mqtt_meta_topic);
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
static int  LOADSTAR_serial_process(int serialfd) {
	int rc = 0;
	int i,n;
	uint64_t microtime_now;
	int milliseconds_since_stx;
	char buff[64] = {};


	static char packet[128];
	static int packet_pos=0;
	static uint64_t microtime_start=0;
	static int state=STATE_LOOKING_FOR_STX;

	//n = read (serialfd, buff, sizeof(buff));  // read next character if ready
	n = read (serialfd, buff, 5);  // read next character if ready
	microtime_now=microtime();
	


	/* non-blocking, so we will get here if there was no data available */
	/* read timeout */
	if ( 0 == n ) {
		if ( outputDebug ) {
			fprintf(stderr,"# (read returned 0 bytes)\n");
		}
		return rc;
	}
	else {
		if ( outputDebug ) {
			buff[n] = '\0';
			fprintf(stderr,"0# %s  \n",buff);
		}
	}	
	

	/* cancel pending alarm */
	alarm(0);
	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	// alarm(alarmSeconds);

	milliseconds_since_stx=(int) ((microtime_now-microtime_start)/1000.0);


	/* LOADSTAR packets:
		start with STX
		end with '\r' or '\n'
		get aborted on timeout
	*/

	/* copy byte to packet */
	for ( i=0 ; 0 == rc && i<n ; i++ ) {
		/* look for start character */
		if (  STATE_LOOKING_FOR_STX == state &&  '\r' !=  buff[i] && '\n' != buff[i] ) {
			microtime_start=microtime_now;
			state=STATE_IN_PACKET;
			packet[0]=buff[i];
			packet_pos=1;
			continue;
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
	
			if (  '\r' == buff[i] ||   '\n' == buff[i] ) {
				state=STATE_LOOKING_FOR_STX;
				alarm(0);
				/* process packet */
				rc = _loadStar_packet_processor(packet,packet_pos,microtime_start,milliseconds_since_stx);
				memset(packet,'\0',sizeof(packet));
				packet_pos = 0;
				microtime_start = 0;
				microtime_now=microtime();
				milliseconds_since_stx=(int) ((microtime_now-microtime_start)/1000.0);
				continue;
			}

			if ( packet_pos < sizeof(packet)-1 ) {
				packet[packet_pos]=buff[i];
				packet_pos++;
			} else {
				if ( outputDebug ) {
					printf("(packet length exceeded length of buffer!)\n");
				}
			}
			if ( outputDebug ) {
				fprintf(stderr,"# %s\n",packet);
			}

		}

	}

return	rc;
}

static int _get_sampling_rate( int fd ) {
	char	buffer[256] = {};
	int rd;
	int	rc = 0;

	write(fd,"\r\n\r\n\r\n",6);	/* send 3 crlf to get it to sop continous */
	usleep(100000);

	alarm(0);
	while ( 0 != (rd = read(fd,buffer,sizeof(buffer)))) {	/* let the input stream drain */
		usleep(100000);
	}
	alarm(0);
	write(fd,"SPS\r\n",5);	/* Samples Per Second */
	usleep(100000);
	alarm(0);
	if ( 0 == (rd = read(fd,buffer,sizeof(buffer)))) {	/* wait for input */
		rc = 1;
		fprintf(stderr,"# no reponse from SPS\n");
	} else {
		char *p;
		alarm(0);
		p = buffer;
		for ( ; 0 != p[0] && ' ' > p[0] ;	p++ ) {
			}
		loadStarSamplesPerSecond = atoi(p);
	}

	return rc;

}
static int _get_units( int fd ) {
	char	buffer[256] = {};
	int rd;
	int	rc = 0;


	alarm(0);
	write(fd,"UNIT\r\n",6);	/* unit of measure */
	usleep(100000);
	alarm(0);
	if ( 0 == (rd = read(fd,buffer,sizeof(buffer)))) {	/* wait for input */
		rc = 1;
		fprintf(stderr,"# no reponse from UNIT\n");
	} else {
		char *p;
		alarm(0);
		p = buffer;
		for ( ; 0 != p[0] && ' ' > p[0] ;	p++ ) {
			}
		p = strpbrk("LKN",p);
		switch ( p[0] ) {
			case	'L':
				strcpy(loadStarUNIT,"LBS");	
				break;
			case	'K':
				strcpy(loadStarUNIT,"Kilograms");
				break;
			case	'N':
				strcpy(loadStarUNIT,"Newtons");
				break;
			default:
				rc  = 1;
				strcpy(loadStarUNIT,"Unknown");
		}
	}
	fprintf(stderr,"# %s units\n",loadStarUNIT);
	return	rc;
}
static void _set_continuous( int fd ) {
	write(fd,"WC\r\n",4);	/* weigh continuous */
	usleep(100000);
}
void loadStar_engine(int serialfd,char *special_handling ) {
	int i;
	int rc = 0;

	if ( _get_sampling_rate(serialfd)) {
		exit(1);
	}
	if ( _get_units(serialfd)) {
		exit(1);
	}
	_set_continuous(serialfd);


	set_blocking (serialfd, 1, 0);		// blocking wait for a character
	/* server socket */
	fd_set active_fd_set, read_fd_set;

	FD_ZERO( &active_fd_set);


	/* add serial fd to fd_set for select() */
	FD_SET(serialfd, &active_fd_set);

	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	alarm(alarmSeconds);
	alarm(0);

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
			rc = LOADSTAR_serial_process(serialfd);
		} 
	}

}
