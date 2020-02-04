
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

extern int serToMQTT_pub(const char *message );

static int local_stx = 'S';
static int local_etx = '\n';



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




static void text_packet_processor(char *packet, int length, uint64_t microtime_start, int millisec_since_start) {

	/* quick sanity check */
	if ( length < 9 )
		return;

	/* null terminate so we can use string functions going forwards */
	packet[length]='\0';

// fprintf(stderr,"#  %s\n",packet);
	
char	buffer[32] = {};
struct json_object *jobj;

jobj = json_object_new_object();
snprintf(buffer,sizeof(buffer),"%lu",microtime_start);
json_object_object_add(jobj,"epochMicroseconds",json_object_new_string(buffer));
json_object_object_add(jobj,"milliSecondSinceStart",json_object_new_int(millisec_since_start));
json_object_object_add(jobj,"rawData",json_object_new_string(packet));

// fprintf(stderr,"# %s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));
// fprintf(stdout,"%s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));	fflush(stdout);
(void) serToMQTT_pub(json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));
json_object_put(jobj);
}




enum states {
STATE_LOOKING_FOR_STX,
STATE_IN_PACKET,
};

extern uint64_t microtime(); 

static void serial_process(int serialfd) {
	int i,n;
	uint64_t microtime_now;
	int milliseconds_since_stx;
	char buff[1];


	static char packet[128];
	static int packet_pos=0;
	static uint64_t microtime_start=0;
	static int state=STATE_LOOKING_FOR_STX;

	n = read (serialfd, buff, sizeof(buff));  // read next character if ready
/*   stub stub stub 
	if ( 0 < n ) write(1,buff,n);	return; */
	
	microtime_now=microtime();

//`	printf("# read buff[0]=%c\n",buff[0]);

	/* non-blocking, so we will get here if there was no data available */
	/* read timeout */
	if ( 0 == n ) {
		if ( outputDebug ) {
			printf("(read returned 0 bytes)\n");
		}
		return;
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
	for ( i=0 ; i<n ; i++ ) {
		/* look for start character */
		if ( STATE_LOOKING_FOR_STX == state && local_stx ==  buff[i] ) {
			packet_pos=0;
			microtime_start=microtime_now;
			state=STATE_IN_PACKET;
			packet[0]='$';
		}

		if ( STATE_IN_PACKET == state ) {
//			printf("---> milliseconds_since_stx = %d\n",milliseconds_since_stx);
			if ( milliseconds_since_stx > milliseconds_timeout ) {
				packet_pos=0;
				state=STATE_LOOKING_FOR_STX;

				if ( outputDebug ) {
					printf("(timeout while reading NMEA sentence)\n");
				}
				continue;
			}
	
//			if ( '\r' == buff[i] || '\n' == buff[i] ) {
			if ( local_etx == buff[i] ) {
				state=STATE_LOOKING_FOR_STX;

				/* process packet */
				text_packet_processor(packet,packet_pos,microtime_start,milliseconds_since_stx);
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
if ( 0 == strcmp(p,"stx"))
	local_stx = q[0];
else if ( 0 == strcmp(p,"etx"))
	local_etx = q[0];
else
	{
	fprintf(stderr,"# -M option '%s' not supported.\n",s);
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

void text_engine(int serialfd,char *special_handling ) {
	int i;

	_overRide(special_handling);

	/* server socket */
	fd_set active_fd_set, read_fd_set;



	/* add serial fd to fd_set for select() */
	FD_SET(serialfd, &active_fd_set);

	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	//alarm(alarmSeconds);


	for ( ; ; ) {
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

		/* Service all the sockets with input pending. */
		for ( i=0 ; i < FD_SETSIZE ; ++i ) {
			if ( FD_ISSET(i, &read_fd_set) ) {
				if ( serialfd  == i ) {
					/* serial port has something to do */
					serial_process(serialfd);
				}
			}
		}
	}

}
