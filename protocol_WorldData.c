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
static int _pollat=-1;	/* the number of milliseconds after the top of the second */	




extern char *optarg;
extern int optind, opterr, optopt;

extern int outputDebug;
extern int milliseconds_timeout;
extern int alarmSeconds;

typedef struct ll_xrw2g_pulseTimeAnemometer {
	struct ll_xrw2g_pulseTimeAnemometer *next;
	int pulse_channel;
	double M;
	double B;
	char *Title;
	char *Units;
	char *channelName;
	int flag;   /* 0 == pulseTime, 1 = pulseMinTime */
} LL_xrw2g_pulseTimeAnemometer;

LL_xrw2g_pulseTimeAnemometer *xrw2g_pulseTimeAnemometers_root;

typedef struct ll_xrw2g_pulseCountAnemometer {
	struct ll_xrw2g_pulseCountAnemometer *next;
	int pulse_channel;
	double M;
	double B;
	char *Title;
	char *Units;
	char *channelName;
} LL_xrw2g_pulseCountAnemometer;

LL_xrw2g_pulseCountAnemometer *xrw2g_pulseCountAnemometers_root;

typedef struct ll_xrw2g_linear {
	struct ll_xrw2g_linear *next;
	int analog_channel;
	double M;
	double B;
	char *Title;
	char *Units;
	char *channelName;
} LL_xrw2g_linear;

LL_xrw2g_linear *xrw2g_linears_root;

typedef struct ll_xrw2g_thermistorNTC {
	struct ll_xrw2g_thermistorNTC *next;
	int analog_channel;
	double Beta;
	double Beta25;
	double RSource;
	double VSource;
	char *Title;
	char *Units;
	char *channelName;
} LL_xrw2g_thermistorNTC;

LL_xrw2g_thermistorNTC *xrw2g_thermistorNTCs_root;

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


static char *_get_quoted_string(char *s ) {
	char *p,*q;
	char *string;
	p = strchr(s,'"');
	if ( 0 == p ) {
		string = "";
	} else {
		q = strchr(p+1,'"');
		if ( 0 == q ) {
			string = "";
		} else {
			q[0] = '\0';	
			string = p+1;
		}
	}
	return strsave(string);

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

	fprintf(stderr,"length=%d\n",length);
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

	// debug_crc(packet,length);
	
	/* calculate local checksum */
	lChecksum = _crc_chk(packet+1,length - 3);

	/* compute remote checksum */
	rChecksum = packet[length -2 ];
	rChecksum <<= 8;
	rChecksum += packet[length -1 ];


	/* compare local and remote checksums */
	if ( lChecksum != rChecksum ) {
		if ( outputDebug ) {
			fprintf(stderr,"(remote %0x and local %xd checksum do not match!)\n",rChecksum,lChecksum);
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

	/* cancel pending alarm */
	alarm(0);
	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	alarm(alarmSeconds);

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
		if ( STATE_IN_PACKET == state && milliseconds_since_stx > milliseconds_timeout ) {
			packet_pos=0;
			state=STATE_LOOKING_FOR_STX;

			if ( outputDebug ) {
				printf("(timeout while reading WorldData)\n");
			}
			continue;
		}
		if ( 6 == packet_pos  &&  _PACKET_TYPE != packet[5] ){
			packet_pos=0;
			state=STATE_LOOKING_FOR_STX;
			alarm(0);
			if ( outputDebug ) {
				fprintf(stderr,"# wrong PACKET_TYPE %02x\n",packet[5]);
			}
			continue;
		}
		if ( packet_pos  == _PACKET_LENGTH ){
			state=STATE_LOOKING_FOR_STX;
			alarm(0);
			/* process packet */
			rc = WorldData_packet_processor(packet,packet_pos,microtime_start,milliseconds_since_stx);
			microtime_start=0;
			break;
		}

	



	}

return	rc;
}
static void _do_format(char *s ) {
	if ( 0 == strcmp(s,"XRW2G" )) {
		_PACKET_TYPE = 23;
		_PACKET_LENGTH = 98;	
		fprintf(stderr,"# _PACKET_TYPE = %02x\n",_PACKET_TYPE);
		fprintf(stderr,"# _PACKET_LENGTH = %02x\n",_PACKET_LENGTH);
	} else {
		fprintf(stderr,"# Bad format %s\n",s);
		fprintf(stderr,"# format=[XRW2G]\n");
		exit(1);
	}


}
static void _do_pollat(char *s ) {
	_pollat = atoi(s);
	if ( 999 < _pollat ) {
		fprintf(stderr,"# pollat %d > 999.  Must be 0 - 999\n",_pollat);
		exit(1);
	}
}
static void _do_command( char * s) {
	/* s contains one command of the type  $cmd=$parameter */
	char	buffer[256] = {};
	char	*p,*q;

	strncpy(buffer,s,sizeof(buffer) - 1);

	q= buffer;
	p = strsep(&q,"=(");
	/* p is the command and q is the paramenter */
	if ( 0 == strcmp(p,"stx")) {
		local_stx = q[0];
	} else if ( 0 == strcmp(p,"format")) {
		_do_format(q);
	} else if ( 0 == strcmp(p,"pollat")) {
		_do_pollat(q);
	} else {
	fprintf(stderr,"# --special-handling option '%s' not supported.\n",s);
	fprintf(stderr,"# [stx, format , pollat]\n");
	exit(1);
	}
}
static char * get_xrw2g_pulseTimeAnemometer(char ** qP ) {
	char *s = *qP;
	char *p,*q;
	static char buffer[1024];

	if ( 0 == ( p = strstr(s,"xrw2g_pulseTimeAnemometer(") )) {
		return	0;
	}
	if ( 0 == (q = strchr(p+1,')'))) {
		return	0;
	}
	/* okay there is the function and the closing ')' */
	*qP = q;
	s = buffer;
	for ( ; p <= q; p++,s++ ) {
		s[0] = p[0];
	}
	s[0] = '\0';
	return	buffer;
}

static void ll_add_xrw2g_pulseTimeAnemometer(int pulse_channel, char *channelName, double M, double B, char *Title, char *Units , int flag) {
	if ( 0 == xrw2g_pulseTimeAnemometers_root ) {
		xrw2g_pulseTimeAnemometers_root = calloc(1,sizeof(LL_xrw2g_pulseTimeAnemometer));
		xrw2g_pulseTimeAnemometers_root->pulse_channel = pulse_channel;
		xrw2g_pulseTimeAnemometers_root->M = M;
		xrw2g_pulseTimeAnemometers_root->B = B;
		xrw2g_pulseTimeAnemometers_root->Title = Title;
		xrw2g_pulseTimeAnemometers_root->Units = Units;
		xrw2g_pulseTimeAnemometers_root->channelName = channelName;
		xrw2g_pulseTimeAnemometers_root->flag = flag;
		return;
	}
	LL_xrw2g_pulseTimeAnemometer *p = xrw2g_pulseTimeAnemometers_root;
	while ( 0 != p->next ) {
		p = p->next;
	}
	/* okay we are at the end */
	p->next =  calloc(1,sizeof(LL_xrw2g_pulseTimeAnemometer));
	p->next->pulse_channel = pulse_channel;
	p->next->M = M;
	p->next->B = B;
	p->next->Title = Title;
	p->next->Units = Units;
	p->next->channelName = channelName;
	p->next->flag = flag;

}
static  int parse_xrw2g_pulseTimeAnemometer( char *s ) {
	char *p,*q;
	q = s;

	p = strsep(&q,"(");
	if ( 0 == p || 0 != strcmp(p,"xrw2g_pulseTimeAnemometer")) {
		fprintf(stderr,"Parse error xrw2g_pulseTimeAnemometer\n");
		return	-1;
	}
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"Parse error xrw2g_pulseTimeAnemometer\n");
		return	-1;
	}
	int pulse_channel = atoi(p);

	if ( 0 > pulse_channel || 2 < pulse_channel ) {
		fprintf(stderr,"pulse channel out of range. xrw2g_pulseTimeAnemometer\n");
		return	-1;
	}

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing channelName  xrw2g_pulseTimeAnemometer\n");
		return	-1;
	}
	char *channelName  = _get_quoted_string(p);
	
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing M  xrw2g_pulseTimeAnemometer\n");
		return	-1;
	}

	double M = atof(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing B  xrw2g_pulseTimeAnemometer\n");
		return	-1;
	}

	double B = atof(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing Title  xrw2g_pulseTimeAnemometer\n");
		return	-1;
	}

	char *Title = _get_quoted_string(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing Units  xrw2g_pulseTimeAnemometer\n");
		return	-1;
	}

	char *Units = _get_quoted_string(p);

	p = strsep(&q,")");	
	if ( 0 == p ) {
		fprintf(stderr,"missing flag  xrw2g_pulseTimeAnemometer\n");
		return	-1;
	}
	int flag = atoi(p);

	ll_add_xrw2g_pulseTimeAnemometer(pulse_channel,channelName,M,B,Title,Units,flag);

	return	0;

}
static void _xrw2g_pulseTimeAnemometer( char *s ) {
	char	buffer[4096] = {};
	char	*p,*q;
	strncpy(buffer,s,sizeof(buffer) - 1);

	/* we are looking for something like
		xrw2g_pulseTimeAnemometer( 0,0.5, 1.0, "Reference Anemometer", "m/s" )  */

	q = buffer;

	while ( p = get_xrw2g_pulseTimeAnemometer(&q)) {
		fprintf(stderr,"# %s\n",p);
		if ( 0 != parse_xrw2g_pulseTimeAnemometer(p)) {
			exit(1);
		}
	}

}


static char * get_xrw2g_pulseCountAnemometer(char ** qP ) {
	char *s = *qP;
	char *p,*q;
	static char buffer[1024];

	if ( 0 == ( p = strstr(s,"xrw2g_pulseCountAnemometer(") )) {
		return	0;
	}
	if ( 0 == (q = strchr(p+1,')'))) {
		return	0;
	}
	/* okay there is the function and the closing ')' */
	*qP = q;
	s = buffer;
	for ( ; p <= q; p++,s++ ) {
		s[0] = p[0];
	}
	s[0] = '\0';
	return	buffer;
}

static void ll_add_xrw2g_pulseCountAnemometer(int pulse_channel, char *channelName, double M, double B, char *Title, char *Units ) {
	if ( 0 == xrw2g_pulseCountAnemometers_root ) {
		xrw2g_pulseCountAnemometers_root = calloc(1,sizeof(LL_xrw2g_pulseCountAnemometer));
		xrw2g_pulseCountAnemometers_root->pulse_channel = pulse_channel;
		xrw2g_pulseCountAnemometers_root->M = M;
		xrw2g_pulseCountAnemometers_root->B = B;
		xrw2g_pulseCountAnemometers_root->Title = Title;
		xrw2g_pulseCountAnemometers_root->Units = Units;
		xrw2g_pulseCountAnemometers_root->channelName = channelName;
		return;
	}
	LL_xrw2g_pulseCountAnemometer *p = xrw2g_pulseCountAnemometers_root;
	while ( 0 != p->next ) {
		p = p->next;
	}
	/* okay we are at the end */
	p->next =  calloc(1,sizeof(LL_xrw2g_pulseCountAnemometer));
	p->next->pulse_channel = pulse_channel;
	p->next->M = M;
	p->next->B = B;
	p->next->Title = Title;
	p->next->Units = Units;
	p->next->channelName = channelName;

}
static  int parse_xrw2g_pulseCountAnemometer( char *s ) {
	char *p,*q;
	q = s;

	p = strsep(&q,"(");
	if ( 0 == p || 0 != strcmp(p,"xrw2g_pulseCountAnemometer")) {
		fprintf(stderr,"Parse error xrw2g_pulseCountAnemometer\n");
		return	-1;
	}
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"Parse error xrw2g_pulseCountAnemometer\n");
		return	-1;
	}
	int pulse_channel = atoi(p);

	if ( 0 > pulse_channel || 2 < pulse_channel ) {
		fprintf(stderr,"pulse channel out of range. xrw2g_pulseCountAnemometer\n");
		return	-1;
	}
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing channelName  xrw2g_pulseCountAnemometer\n");
		return	-1;
	}
	char *channelName  = _get_quoted_string(p);
	
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing M  xrw2g_pulseCountAnemometer\n");
		return	-1;
	}

	double M = atof(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing B  xrw2g_pulseCountAnemometer\n");
		return	-1;
	}

	double B = atof(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing Title  xrw2g_pulseCountAnemometer\n");
		return	-1;
	}

	char *Title = _get_quoted_string(p);

	p = strsep(&q,")");	
	if ( 0 == p ) {
		fprintf(stderr,"missing Units  xrw2g_pulseCountAnemometer\n");
		return	-1;
	}

	char *Units = _get_quoted_string(p);

	ll_add_xrw2g_pulseCountAnemometer(pulse_channel,channelName,M,B,Title,Units);

	return	0;

}
static void _xrw2g_pulseCountAnemometer( char *s ) {
	char	buffer[4096] = {};
	char	*p,*q;
	strncpy(buffer,s,sizeof(buffer) - 1);

	/* we are looking for something like
		xrw2g_pulseCountAnemometer( 0,0.5, 1.0, "Reference Anemometer", "m/s" )  */

	q = buffer;

	while ( p = get_xrw2g_pulseCountAnemometer(&q)) {
		fprintf(stderr,"# %s\n",p);
		if ( 0 != parse_xrw2g_pulseCountAnemometer(p)) {
			exit(1);
		}
	}

}


static char * get_xrw2g_linear(char ** qP ) {
	char *s = *qP;
	char *p,*q;
	static char buffer[1024];

	if ( 0 == ( p = strstr(s,"xrw2g_linear(") )) {
		return	0;
	}
	if ( 0 == (q = strchr(p+1,')'))) {
		return	0;
	}
	/* okay there is the function and the closing ')' */
	*qP = q;
	s = buffer;
	for ( ; p <= q; p++,s++ ) {
		s[0] = p[0];
	}
	s[0] = '\0';
	return	buffer;
}

static void ll_add_xrw2g_linear(int analog_channel, char *channelName, double M, double B, char *Title, char *Units ) {
	if ( 0 == xrw2g_linears_root ) {
		xrw2g_linears_root = calloc(1,sizeof(LL_xrw2g_linear));
		xrw2g_linears_root->analog_channel = analog_channel;
		xrw2g_linears_root->M = M;
		xrw2g_linears_root->B = B;
		xrw2g_linears_root->Title = Title;
		xrw2g_linears_root->Units = Units;
		xrw2g_linears_root->channelName = channelName;
		return;
	}
	LL_xrw2g_linear *p = xrw2g_linears_root;
	while ( 0 != p->next ) {
		p = p->next;
	}
	/* okay we are at the end */
	p->next =  calloc(1,sizeof(LL_xrw2g_linear));
	p->next->analog_channel = analog_channel;
	p->next->M = M;
	p->next->B = B;
	p->next->Title = Title;
	p->next->Units = Units;
	p->next->channelName = channelName;

}
static  int parse_xrw2g_linear( char *s ) {
	char *p,*q;
	q = s;

	p = strsep(&q,"(");
	if ( 0 == p || 0 != strcmp(p,"xrw2g_linear")) {
		fprintf(stderr,"Parse error xrw2g_linear\n");
		return	-1;
	}
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"Parse error xrw2g_linear\n");
		return	-1;
	}
	int analog_channel = atoi(p);

	if ( 0 > analog_channel || 7 < analog_channel ) {
		fprintf(stderr,"pulse channel out of range. xrw2g_linear\n");
		return	-1;
	}
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"Parse error xrw2g_linear\n");
		return	-1;
	}
	char *channelName = _get_quoted_string(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing M  xrw2g_linear\n");
		return	-1;
	}

	double M = atof(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing B  xrw2g_linear\n");
		return	-1;
	}

	double B = atof(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"Parse error xrw2g_linear\n");
		return	-1;
	}
	char *Title = _get_quoted_string(p);

	p = strsep(&q,")");	
	if ( 0 == p ) {
		fprintf(stderr,"Parse error xrw2g_linear\n");
		return	-1;
	}
	char *Units = _get_quoted_string(p);


	ll_add_xrw2g_linear(analog_channel,channelName,M,B,Title,Units);

	return	0;

}
static void _xrw2g_linear( char *s ) {
	char	buffer[4096] = {};
	char	*p,*q;
	strncpy(buffer,s,sizeof(buffer) - 1);

	/* we are looking for something like
		xrw2g_linear( 0,0.5, 1.0, "Reference Anemometer", "m/s" )  */

	q = buffer;

	while ( p = get_xrw2g_linear(&q)) {
		fprintf(stderr,"# %s\n",p);
		if ( 0 != parse_xrw2g_linear(p)) {
			exit(1);
		}
	}

}

static char * get_xrw2g_thermistorNTC(char ** qP ) {
	char *s = *qP;
	char *p,*q;
	static char buffer[1024];

	if ( 0 == ( p = strstr(s,"xrw2g_thermistorNTC(") )) {
		return	0;
	}
	if ( 0 == (q = strchr(p+1,')'))) {
		return	0;
	}
	/* okay there is the function and the closing ')' */
	*qP = q;
	s = buffer;
	for ( ; p <= q; p++,s++ ) {
		s[0] = p[0];
	}
	s[0] = '\0';
	return	buffer;
}
static void ll_add_xrw2g_thermistorNTC(int analog_channel, char *channelName, double Beta, double Beta25, 
	double RSource, double VSource, char *Title, char *Units ) {
	if ( 0 == xrw2g_thermistorNTCs_root ) {
		xrw2g_thermistorNTCs_root = calloc(1,sizeof(LL_xrw2g_thermistorNTC));
		xrw2g_thermistorNTCs_root->analog_channel = analog_channel;
		xrw2g_thermistorNTCs_root->Beta = Beta;
		xrw2g_thermistorNTCs_root->Beta25 = Beta25;
		xrw2g_thermistorNTCs_root->RSource = RSource;
		xrw2g_thermistorNTCs_root->VSource = VSource;
		xrw2g_thermistorNTCs_root->channelName = channelName;
		xrw2g_thermistorNTCs_root->Title = Title;
		xrw2g_thermistorNTCs_root->Units = Units;
		return;
	}
	LL_xrw2g_thermistorNTC *p = xrw2g_thermistorNTCs_root;
	while ( 0 != p->next ) {
		p = p->next;
	}
	/* okay we are at the end */
	p->next =  calloc(1,sizeof(LL_xrw2g_thermistorNTC));
	p->next->analog_channel = analog_channel;
	p->next->Beta = Beta;
	p->next->Beta = Beta;
	p->next->RSource = RSource;
	p->next->VSource = VSource;
	p->next->channelName = channelName;
	p->next->Title = Title;
	p->next->Units = Units;

}
static  int parse_xrw2g_thermistorNTC( char *s ) {
	char *p,*q;
	q = s;

	p = strsep(&q,"(");
	if ( 0 == p || 0 != strcmp(p,"xrw2g_thermistorNTC")) {
		fprintf(stderr,"Parse error xrw2g_thermistorNTC\n");
		return	-1;
	}
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"Parse error xrw2g_thermistorNTC\n");
		return	-1;
	}
	int analog_channel = atoi(p);

	if ( 0 > analog_channel || 7 < analog_channel ) {
		fprintf(stderr,"pulse channel out of range. xrw2g_thermistorNTC\n");
		return	-1;
	}
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing channelName  xrw2g_thermistorNTC\n");
		return	-1;
	}

	char *channelName = _get_quoted_string(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing Beta  xrw2g_thermistorNTC\n");
		return	-1;
	}

	double Beta = atof(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing Beta25 xrw2g_thermistorNTC\n");
		return	-1;
	}

	double Beta25 = atof(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing RSource xrw2g_thermistorNTC\n");
		return	-1;
	}
	

	double RSource = atof(p);
	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing DSource xrw2g_thermistorNTC\n");
		return	-1;
	}
	double DSource = atof(p);

	p = strsep(&q,",");	
	if ( 0 == p ) {
		fprintf(stderr,"missing Title xrw2g_thermistorNTC\n");
		return	-1;
	}
	char *Title = _get_quoted_string(p);


	p = strsep(&q,")");	
	if ( 0 == p ) {
		fprintf(stderr,"missing Units xrw2g_thermistorNTC\n");
		return	-1;
	}
	char *Units = _get_quoted_string(p);


	ll_add_xrw2g_thermistorNTC(analog_channel,channelName, Beta,Beta25,RSource,DSource,Title, Units);

	return	0;

}
static void _xrw2g_thermistorNTC( char *s ) {
	char	buffer[4096] = {};
	char	*p,*q;
	strncpy(buffer,s,sizeof(buffer) - 1);

	/* we are looking for something like
		xrw2g_thermistorNTC( 0,0.5, 1.0, "Reference Anemometer", "m/s" )  */

	q = buffer;

	while ( p = get_xrw2g_thermistorNTC(&q)) {
		fprintf(stderr,"# %s\n",p);
		if ( 0 != parse_xrw2g_thermistorNTC(p)) {
			exit(1);
		}
	}

}
static void _overRide(char *s ) {
	/*  s can contain commands of the type  $cmd=$parameter with one or more commands sepearted by whitespace */
	char	buffer[4096] = {};
	char	*p,*q;

	strncpy(buffer,s,sizeof(buffer) - 1);

	q= buffer;
	while ( (p = strsep(&q," \t\r\n")) && ('\0' != p[0]) ) {	// this will find zero or more commands
		if ( 0 != strchr(p,'=') ) {
			_do_command(p);
		}
	}
}
static void _poll(int serialfd ) {
	static char buffer[6] = { 0x23, 0x41, 0x00, 0x01, };
	struct timeval time;
	gettimeofday(&time, NULL); 
	/* three condition  -1 = before  0 = during and  1= after */
	int now = time.tv_usec / 1000;
	int flag;
	static int already_done;

	if ( _pollat > now ) {
		flag = -1;
	} if ( _pollat < (now + 50 ) ) {
		flag = 0;
	} else {
		flag = 1;
	}

	if ( 0 != flag ) {
		already_done = 0;
	} else if ( 0 == already_done ) {
		already_done = 1;
		write(serialfd,buffer,4);
	}
}

void WorldData_engine(int serialfd,char *special_handling ) {
	int i;
	int rc = 0;
	struct timeval poll_interval;

	_xrw2g_pulseCountAnemometer(special_handling);
	_xrw2g_pulseTimeAnemometer(special_handling);
	_xrw2g_linear(special_handling);
	_xrw2g_thermistorNTC(special_handling);
	_overRide(special_handling);

	set_blocking (serialfd, 1, 0);		// blocking wait for a character
	/* server socket */
	fd_set active_fd_set, read_fd_set;

	FD_ZERO( &active_fd_set);


	/* add serial fd to fd_set for select() */
	FD_SET(serialfd, &active_fd_set);

	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	//alarm(alarmSeconds);

	if ( -1 != _pollat ) {
		poll_interval.tv_sec = 0;
		poll_interval.tv_usec = 10000;	/* 10 msec */
	}

	for ( ; 0 == rc ; ) {
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;


		i=select(FD_SETSIZE, &read_fd_set, NULL, NULL, ( -1 == _pollat ) ? NULL: &poll_interval );
		if ( -1 != _pollat ) {
			_poll(serialfd);
		}


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
