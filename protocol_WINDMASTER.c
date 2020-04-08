/* IT seems that the windMASTER needs to be polled inorder to get a suitable data rate.
   therefore we cannot use protocol_text.c */
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
#define STX 2
#define ETX 3

typedef struct station {
	struct station *left,*right;
	char *listenerID;	/* set at startup by user */
	char *pollCmd;	/* set by user */
	int milliSecondInterval;	/* set at startup by user */
	struct timeval	tv;
} ANEMOMETER;

static ANEMOMETER *rootAnemometer;

void installAnemometer( const ANEMOMETER *s ) {
	if ( 0 == rootAnemometer ) {
		rootAnemometer = calloc(1,sizeof(ANEMOMETER));
		*rootAnemometer = *s;
	}
	else {
		ANEMOMETER *p,*q;
		int ind;
		p = rootAnemometer;
		while ( 0 != p ) {
			ind = strcmp(s->listenerID,p->listenerID);
			if ( 0 == ind ) {
				fprintf(stderr,"# listener=%s already in the table.\n",s->listenerID);
				return;
			}
			q = p;
			if ( ind < 0 ) {
				p = q->left;
			}
			else {
				p = q->right;
			}
		}
		p = calloc(1,sizeof(ANEMOMETER));
		*p = *s;
		if ( ind < 0 ) {
			q->left = p;
		}
		else {
			q->right = p;
		}
	}	
}

int do_anemometer(const char *cmd ) {
	/*   cmd format will be "listener=01 talker=WI interval=1500" */
	int rc=0;
	char command[256] = {};
	ANEMOMETER station = {};
	char *p,*q;
	strncpy(command,cmd,sizeof(command));
	q = command;

	while ( 0 != (p = strsep(&q," "))) {
		if ( 0 == strncmp("listener",p,8)) {
			if ( 10 != strlen(p) ) {
				rc = 1;
			}
			station.listenerID = strsave(p+9);
		}
	}
	if ( 0 != rc || 0 == station.listenerID ) {
		fprintf(stderr,"# bad --anemometer\nEpecting \"listener=Q\"\n");
	}
	else {
		installAnemometer(&station);
	}
			
	return	rc;
}

int _WINDMASTER_FORMAT = 1;

#include "protocol_WINDMASTER.formatter.c"

static int _windMaster_packet_processor(char *packet, int length, uint64_t microtime_start, int millisec_since_start) {
	int rc = 0;
	int i;
	int lChecksum,rChecksum;
	const char	*topic = mqtt_topic;

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
	if ( 1 != sscanf(packet+length-2,"%X",&rChecksum) ) {
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
	struct json_object *tmp;

	tmp = ( 0 != _WINDMASTER_FORMAT ) ?   do_WINDMASTER_FORMAT(packet) : 0;
	jobj = json_object_new_object();
	if ( 0 == retainedFlag ) {
		json_object_object_add(jobj,"date",json_object_new_dateTime());
		json_object_object_add(jobj,"milliSecondSinceStart",json_object_new_int(millisec_since_start));
		json_object_object_add(jobj,"rawData",json_object_new_string(packet));
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
static int  WINDMASTER_serial_process(int serialfd) {
	int rc = 0;
	int i,n;
	uint64_t microtime_now;
	int milliseconds_since_stx;
	char buff[64] = {};


	static char packet[128];
	static int packet_pos=0;
	static uint64_t microtime_start=0;
	static int state=STATE_LOOKING_FOR_STX;

#if 1
	n = read (serialfd, buff, sizeof(buff));  // read next character if ready
#else
	strcpy(buff,"\002Q,,000.01,-000.01,M,00,\00331\r\n");
	n = strlen(buff);
#endif
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


	/* WINDMASTER packets:
		start with STX
		end with '\r' or '\n'
		get aborted on timeout
	*/

	/* copy byte to packet */
	for ( i=0 ; 0 == rc && i<n ; i++ ) {
		/* look for start character */
		if ( /* STATE_LOOKING_FOR_STX == state && */ STX ==  buff[i] ) {
			microtime_start=microtime_now;
			state=STATE_IN_PACKET;
			packet[0]='$';
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
				rc = _windMaster_packet_processor(packet,packet_pos,microtime_start,milliseconds_since_stx);
				memset(packet,'\0',packet_pos);
				microtime_start = 0;
				packet_pos = 0;
				continue;
			}

			if ( packet_pos < sizeof(packet)-1 ) {
				buff[i] = ( ETX == buff[i] ) ? '*' : buff[i];
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
static struct timeval next_tv;
static void _set_half_sleep(ANEMOMETER *p ) {
	if ( 0 == next_tv.tv_sec ) { 
		next_tv = p->tv;
	} else if ( (p->tv.tv_sec < next_tv.tv_sec) || ( p->tv.tv_sec == next_tv.tv_sec && p->tv.tv_usec <= next_tv.tv_usec)) {
		next_tv = p->tv;
	}
}
static void _set_the_new_timer(struct timeval *xv, ANEMOMETER *p ) {
	struct timeval tv;

	if ( 0 == p->tv.tv_sec ) {
		tv = *xv;
	} else {
		tv = p->tv;
	}
	tv.tv_usec += p->milliSecondInterval * 1000;
	while ( tv.tv_usec > 1000000 ) {
		tv.tv_sec++;
		tv.tv_usec -= 1000000;
	}
		

	p->tv.tv_sec = tv.tv_sec;
	p->tv.tv_usec = tv.tv_usec;

	_set_half_sleep(p);
}
void do_anemometerCmd( ANEMOMETER *p,int serialfd ) {

}
void check_anemometer_intervals(ANEMOMETER *p, int serialfd ) {
	if ( 0 != p ) {
		int lChecksum,length,i;
		char	buffer[64] = {};

		check_anemometer_intervals(p->left,serialfd);
		if ( 0 != p->pollCmd ) {
			do_anemometerCmd(p,serialfd);
		}
		struct timeval time;
		gettimeofday(&time, NULL); 
		if ( (time.tv_sec > p->tv.tv_sec  ) || (time.tv_sec == p->tv.tv_sec && time.tv_usec > p->tv.tv_usec)) {
			_set_the_new_timer(&time,p);
			snprintf(buffer,sizeof(buffer),"%2.2s,WV?",p->listenerID);
			length = strlen(buffer);
			/* calculate local checksum */
			lChecksum=0;
			for ( i=0 ; i<length ; i++ ) {
				lChecksum = lChecksum ^buffer[i];
			}

		snprintf(buffer,sizeof(buffer),"$%2.2s,WV?*%02X\r\n",p->listenerID,lChecksum);
		write(serialfd,buffer,strlen(buffer));
		if ( outputDebug ) {
			fprintf(stderr,"# %s",buffer);
			fflush(stderr);
		}

		check_anemometer_intervals(p->right,serialfd);
		}
	}
}

static int _do_startup( int serialfd ) {

#define PAUSE 2

	char	buffer[64] = {};
	int	n;
	do {
		write(serialfd, "*",1);
		usleep(500000);
		alarm(PAUSE);
		n = read(serialfd,buffer,sizeof(buffer));
		} while ( n > 0 &&  'C' != buffer[0] && '*' != buffer[0] );
		
	if ( 0 >= n ) {
		fprintf(stderr,"# sent \"*\" return form read %d\n",n);
		return -1;
	}
	if ( 0 != strncmp(buffer,"CONFIGURATION MODE",18) && 0 != strncmp(buffer,"*",1)) {
		fprintf(stderr,"# sent \"*\" expecting \"CONFIGURATION MODE\" or \"*\"\n# got \"%s\"\n",buffer);
		return	-2;
	}
	write(serialfd, "M1\r\n",4);
	usleep(500000);
	alarm(PAUSE);
	n = read(serialfd,buffer,sizeof(buffer));
	if ( 0 != strncmp(buffer,"M1",2) ) {
		return	-3;
	}
	write(serialfd, "P8\r\n",4);
	usleep(500000);
	alarm(PAUSE);
	n = read(serialfd,buffer,sizeof(buffer));
	if ( 0 != strncmp(buffer,"P8",2) ) {
		return	-3;
	}
	
	write(serialfd, "Q\r\n",3);
	usleep(500000);
	return	0;	
#undef PAUSE
}

void windMaster_engine(int serialfd,char *special_handling ) {
	int i;
	int rc = 0;

	if ( 0 != _do_startup(serialfd) ) {
		return;
	}
		

	set_blocking (serialfd, 1, 0);		// blocking wait for a character
	/* server socket */
	fd_set active_fd_set, read_fd_set;

	FD_ZERO( &active_fd_set);


	/* add serial fd to fd_set for select() */
	FD_SET(serialfd, &active_fd_set);

	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	alarm(alarmSeconds);


	if ( 0 == rootAnemometer ) {
		fprintf(stderr,"# --anemometer   one or more are required.\n");
		rc = 1;
	}
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
			rc = WINDMASTER_serial_process(serialfd);
		} 
	}

}
