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

typedef struct station {
	struct station *left,*right;
	char *listenerID;	/* set at startup by user */
	char *talkerID;	/* set at startup by user */
	char *startupCmd;	/* optionally set by user */
	int milliSecondInterval;	/* set at startup by user */
	struct timeval	tv;
} STATION;

static STATION *rootStation;

void installStation( const STATION *s ) {
	if ( 0 == rootStation ) {
		rootStation = calloc(1,sizeof(STATION));
		*rootStation = *s;
	}
	else {
		STATION *p,*q;
		int ind;
		p = rootStation;
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
		p = calloc(1,sizeof(STATION));
		*p = *s;
		if ( ind < 0 ) {
			q->left = p;
		}
		else {
			q->right = p;
		}
	}	
}

int do_station(const char *cmd ) {
	/*   cmd format will be "listener=01 talker=WI interval=1500" */
	int rc=0;
	char command[256] = {};
	STATION station = {};
	char *p,*q;
	strncpy(command,cmd,sizeof(command));
	q = command;

	while ( 0 != (p = strsep(&q," "))) {
		if ( 0 == strncmp("listener",p,8)) {
			if ( 11 != strlen(p) ) {
				rc = 1;
			}
			station.listenerID = strsave(p+9);
		} else if ( 0 == strncmp("talker",p,6)) {
			if ( 9 != strlen(p) ) {
				rc = 2;;
			}
			station.talkerID = strsave(p+7);
		} else if ( 0 == strncmp("startup",p,7)) {
			station.startupCmd = strsave(p+8);
		} else if ( 0 == strncmp("interval",p,8)) {
			station.milliSecondInterval=atoi(p+9);
		}
	}
	if ( 0 != rc || 0 == station.milliSecondInterval || 0 == station.listenerID || 0 == station.talkerID ) {
		fprintf(stderr,"# bad --station\nEpecting \"listener=01 talker=WI interval=1500\"\n");
	}
	else {
		installStation(&station);
	}
			
	return	rc;
}

int _FL702LT_FORMAT = 1;

#include "protocol_FL702LT.formatter.c"

static int _fl702lt_packet_processor(char *packet, int length, uint64_t microtime_start, int millisec_since_start) {
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
	struct json_object *tmp;

	tmp = ( 0 != _FL702LT_FORMAT ) ?   do_FL702LT_FORMAT(packet) : 0;
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
static int  _serial_process(int serialfd) {
	int rc = 0;
	int i,n;
	uint64_t microtime_now;
	int milliseconds_since_stx;
	char buff[32];


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


	/* FL702LT packets:
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
	
			if (  '\r' == buff[i] ||   '\n' == buff[i] ) {
				state=STATE_LOOKING_FOR_STX;
				alarm(0);
				/* process packet */
				rc = _fl702lt_packet_processor(packet,packet_pos,microtime_start,milliseconds_since_stx);
				microtime_start = 0;
				break;
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
void set_the_new_timer(struct timeval *xv, STATION *p ) {
	struct timeval tv = *xv;
	tv.tv_usec += p->milliSecondInterval * 1000;
	while ( tv.tv_usec > 1000000 ) {
		tv.tv_sec++;
		tv.tv_usec -= 1000000;
	}
		

	p->tv.tv_sec = tv.tv_sec;
	p->tv.tv_usec = tv.tv_usec;
}
void do_startupCmd( STATION *p,int serialfd ) {
	int lChecksum,length,i;
	char	buffer[128] = {};
	snprintf(buffer,sizeof(buffer),"%2.2s,%s",p->listenerID,p->startupCmd);
	length = strlen(buffer);
	/* calculate local checksum */
	lChecksum=0;
	for ( i=0 ; i<length ; i++ ) {
		lChecksum = lChecksum ^buffer[i];
	}

	snprintf(buffer,sizeof(buffer),"$%2.2s,%s*%02X\r\n",p->listenerID,p->startupCmd,lChecksum);
	write(serialfd,buffer,strlen(buffer));


	free(p->startupCmd);
	p->startupCmd = 0;
	
}
void check_station_intervals(STATION *p, int serialfd ) {
	if ( 0 != p ) {
		int lChecksum,length,i;
		char	buffer[64] = {};

		check_station_intervals(p->left,serialfd);
		if ( 0 != p->startupCmd ) {
			do_startupCmd(p,serialfd);
		}
		struct timeval time;
		gettimeofday(&time, NULL); 
		if ( (time.tv_sec > p->tv.tv_sec  ) || (time.tv_sec == p->tv.tv_sec && time.tv_usec > p->tv.tv_usec)) {
			set_the_new_timer(&time,p);
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

		check_station_intervals(p->right,serialfd);
		}
	}
}


void fl702lt_engine(int serialfd,char *special_handling ) {
	int i;
	int rc = 0;


	set_blocking (serialfd, 1, 0);		// blocking wait for a character
	/* server socket */
	fd_set active_fd_set, read_fd_set, write_fd_set;

	FD_ZERO( &active_fd_set);


	/* add serial fd to fd_set for select() */
	FD_SET(serialfd, &active_fd_set);

	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	//alarm(alarmSeconds);


	if ( 0 == rootStation ) {
		fprintf(stderr,"# --station   one or more are required.\n");
		rc = 1;
	}
	for ( ; 0 == rc ; ) {
		/* Block until input arrives on one or more active sockets. */
		write_fd_set = read_fd_set = active_fd_set;

		i=select(FD_SETSIZE, &read_fd_set, &write_fd_set, NULL, NULL);
		if ( EBADF == i ) {
			fprintf(stderr,"# select() EBADF error. Aborting.\n");
			exit(1);
		} else if ( ENOMEM == i ) {
			fprintf(stderr,"# select() ENOMEM error. Aborting.\n");
			exit(1);
		} 
		
		if ( FD_ISSET(serialfd, &write_fd_set) ) {
			check_station_intervals(rootStation,serialfd);
		} 

		if ( FD_ISSET(serialfd, &read_fd_set) ) {
			rc = _serial_process(serialfd);
		} 
	}

}
