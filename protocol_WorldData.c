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
#include <stdint.h>


static int local_stx = '#';
static uint8_t _PACKET_TYPE;
static uint8_t _PACKET_LENGTH;




extern char *optarg;
extern int optind, opterr, optopt;

extern int outputDebug;
extern int milliseconds_timeout;
extern int alarmSeconds;




#include <sys/time.h>
#include "protocol_WorldData.formatter.c"

static uint16_t _crc_chk(uint8_t *data, uint8_t length) {
	uint8_t j;
	uint16_t reg_crc=0xFFFF;

	while ( length-- ) {
		reg_crc ^= *data++;

		for ( j=0 ; j<8 ; j++ ) {
			if ( reg_crc & 0x01 ) {
				reg_crc=(reg_crc>>1) ^ 0xA001;
			} else {
				reg_crc=reg_crc>>1;
			}
		}	
	}
	
	return reg_crc;
}



int16_t crc_chk(int8_t *data, int8_t length) {
	int8_t j;
	int16_t reg_crc=0xFFFF;

	while ( length-- ) {
		reg_crc ^= *data++;

		for ( j=0 ; j<8 ; j++ ) {
			if ( reg_crc & 0x01 ) {
				reg_crc=(reg_crc>>1) ^ 0xA001;
			} else {
				reg_crc=reg_crc>>1;
			}
		}	
	}
	
	return reg_crc;
}

#if 0
void debug_crc(uint8_t *packet, int length) {

	fprintf(stderr,"0x%04x\n",_crc_chk(packet+1,length - 3));

	int rChecksum = packet[length -2 ];
	rChecksum <<= 8;
	rChecksum += packet[length -1 ];
	fprintf(stderr,"0x%04x\n",rChecksum);

}
#endif

int WorldData_packet_processor(uint8_t *packet, int length, uint64_t microtime_start, int millisec_since_start) {
	int rc = 0;
	uint16_t lChecksum,rChecksum;
	const char	*topic = mqtt_topic;

	/* quick sanity check */
	if ( length < 9 )
		return rc;

	/* null terminate so we can use string functions going forwards */
	packet[length]='\0';

	//debug_crc(packet,length);
	
	/* calculate local checksum */
	lChecksum = _crc_chk(packet+1,length - 3);

	/* compute remote checksum */
	rChecksum = packet[length -2 ];
	rChecksum <<= 8;
	rChecksum += packet[length -1 ];


	/* compare local and remote checksums */
	if ( lChecksum != rChecksum ) {
		if ( outputDebug ) {
			printf("(remote and local checksum do not match!)\n");
		}
		return rc;
	}

	struct json_object *jobj,*tmp = 0;

	tmp = do_WorldData_FORMAT(packet);
	jobj = json_object_new_object();
	if ( 0 == retainedFlag ) {
		json_object_object_add(jobj,"date",json_object_new_dateTime());
		json_object_object_add(jobj,"milliSecondSinceStart",json_object_new_int(millisec_since_start));
		// json_object_object_add(jobj,"rawData",json_object_new_string(packet));
	}

	if ( 0 != tmp ) {
		json_object_object_add(jobj,"formattedData", tmp);
		topic = new_topic(packet,( 0 == retainedFlag ) ? mqtt_topic : mqtt_meta_topic);
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


	static uint8_t packet[256];
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
	if ( '#' == buff[0] ) {
		fprintf(stderr," # %c\n",buff[0]);
	}

	/* cancel pending alarm */
	alarm(0);
	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	// alarm(alarmSeconds);

	milliseconds_since_stx=(int) ((microtime_now-microtime_start)/1000.0);



	/* copy byte to packet */
	for ( i=0 ; 0 == rc && i<n ; i++ ) {
		if ( STATE_IN_PACKET == state ) {
			if ( packet_pos < sizeof(packet)-1 ) {
				packet[packet_pos]=buff[i];
				packet_pos++;
			} else {
				if ( outputDebug ) {
					printf("(packet length exceeded length of buffer!)\n");
				}
			}
		}
		/* look for start character */
		else if ( STATE_LOOKING_FOR_STX == state && local_stx  ==  buff[i] ) {
			packet_pos=1;
			microtime_start=microtime_now;
			state=STATE_IN_PACKET;
			packet[0]=local_stx;
			continue;
		}
#if 0
		if ( 5 < packet_pos  &&  _PACKET_TYPE != packet[5] ){
			packet_pos=0;
			state=STATE_LOOKING_FOR_STX;
			alarm(0);
			continue;
		}
#endif
		if ( packet_pos  == _PACKET_LENGTH ){
			state=STATE_LOOKING_FOR_STX;
			alarm(0);
			/* process packet */
			rc = WorldData_packet_processor(packet,packet_pos,microtime_start,milliseconds_since_stx);
			microtime_start=0;
			break;
		}

#if 0
			if ( milliseconds_since_stx > milliseconds_timeout ) {
				packet_pos=0;
				state=STATE_LOOKING_FOR_STX;

				if ( outputDebug ) {
					printf("(timeout while reading NMEA sentence)\n");
				}
				continue;
			}
#endif
	



	}

return	rc;
}
static void _do_format(char *s ) {
	if ( 0 == strcmp(s,"XRW2G" )) {
		_PACKET_TYPE = 14;
		_PACKET_LENGTH = 98;	
	} else {
		fprintf(stderr,"# Bad format %s\n",s);
		fprintf(stderr,"# format=[XRW2G]\n");
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
	} else if ( 0 == strcmp(p,"format")) {
		_do_format(q);
	} else {
	fprintf(stderr,"# --special-handling option '%s' not supported.\n",s);
	fprintf(stderr,"# [stx, format ,]\n");
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

void WorldData_engine(int serialfd,char *special_handling ) {
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
