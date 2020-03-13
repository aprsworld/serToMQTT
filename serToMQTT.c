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
#include <mosquitto.h>
#include <time.h>
#include "serToMQTT.h"

static int mqtt_port=1883;
static char mqtt_host[256];
char mqtt_topic[256];
static char *mqtt_user_name,*mqtt_passwd;
static struct mosquitto *mosq;
static int disable_mqtt_output;
static int quiet_flag;

static void _mosquitto_shutdown(void);



extern char *optarg;
extern int optind, opterr, optopt;

int outputDebug=0;
int milliseconds_timeout=500;
int alarmSeconds=5;
int no_meta;

typedef struct {
	char *label;
	void (*packet_processor_func)(int serialfd,char *special_handling );
	}	MODES;

uint64_t microtime() {
	struct timeval time;
	gettimeofday(&time, NULL); 
	return ((uint64_t)time.tv_sec * 1000000) + time.tv_usec;
}

struct json_object *json_division(double value,char *description, char *units) {
	struct json_object *jobj = json_object_new_object();
	json_object_object_add(jobj,"value",json_object_new_double(value));
	json_object_object_add(jobj,"description",json_object_new_string(description));
	json_object_object_add(jobj,"units",json_object_new_string(units));
	return	jobj;
}


json_object *json_object_new_dateTime(void) {
	struct tm *now;
	struct timeval time;
	char timestamp[32];
        gettimeofday(&time, NULL);
	now = localtime(&time.tv_sec);
	if ( 0 == now ) {
		fprintf(stderr,"# error calling localtime() %s",strerror(errno));
		exit(1);
	}

	snprintf(timestamp,sizeof(timestamp),"%04d-%02d-%02d %02d:%02d:%02d.%03ld",
		1900 + now->tm_year,1 + now->tm_mon, now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec,time.tv_usec/1000);

	
	return	json_object_new_string(timestamp);

}
void connect_callback(struct mosquitto *mosq, void *obj, int result) {
	if ( 5 == result ) {
		fprintf(stderr,"# --mqtt-user-name and --mqtt-passwd required at this site.\n");
	}
	fprintf(stderr,"# connect_callback, rc=%d\n", result);
}

static void signal_handler(int signum) {


	if ( SIGALRM == signum ) {
		fprintf(stderr,"\n# Timeout while waiting for NMEA data.\n");
		fprintf(stderr,"# Terminating.\n");
		_mosquitto_shutdown();
		exit(100);
	} else if ( SIGPIPE == signum ) {
		fprintf(stderr,"\n# Broken pipe.\n");
		fprintf(stderr,"# Terminating.\n");
		_mosquitto_shutdown();
		exit(101);
	} else if ( SIGUSR1 == signum ) {
		/* clear signal */
		signal(SIGUSR1, SIG_IGN);

		fprintf(stderr,"# SIGUSR1 triggered data_block dump:\n");
		
		/* re-install alarm handler */
		signal(SIGUSR1, signal_handler);
	} else {
		fprintf(stderr,"\n# Caught unexpected signal %d.\n",signum);
		fprintf(stderr,"# Terminating.\n");
		_mosquitto_shutdown();
		exit(102);
	}

}
#include <sys/time.h>

static int set_interface_attribs (int fd, int speed, int parity) {
	struct termios tty;

	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) {
//		error_message ("error %d from tcgetattr", errno);
		return -1;
	}

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;	// disable break processing
	tty.c_lflag = 0;	// no signaling chars, no echo,
				// no canonical processing
	tty.c_oflag = 0;	// no remapping, no delays
	tty.c_cc[VMIN]  = 0;	// read doesn't block
	tty.c_cc[VTIME] = 5;	// 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
					// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0) {
//		error_message ("error %d from tcsetattr", errno);
		return -1;
	}
	return 0;
}

void set_blocking (int fd, int vmin, int vtime) {
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) {
		fprintf(stderr,"# set_blocking unable to load tty attributes\n");
		return;
	}

	tty.c_cc[VMIN]  = vmin;	// minimum  number of characters for noncanonical read
	tty.c_cc[VTIME] = vtime; // timeout in deciseconds for noncanonical read

	if ( outputDebug ) {
		fprintf(stderr,"# set_blocking tty.c_cc[VMIN]=%d\n",tty.c_cc[VMIN]);
		fprintf(stderr,"# set_blocking tty.c_cc[VTIME]=%d\n",tty.c_cc[VTIME]);
	}

	if (tcsetattr (fd, TCSANOW, &tty) != 0) {
		printf("error %d setting term attributes", errno);
	}

}


static void _report_reason_for_serialfd_failure(char *s )
{
struct stat	buf;

fprintf(stderr,"# %s - %s\n",s,strerror(errno));
if ( 0 != stat(s,&buf))
	{
	fprintf(stderr,"# %s does not exisit!\n",s);
	}
else
	{
	fprintf(stderr,"# %s does exisit!\n",s);
	fprintf(stderr,"# %s permissions are %03o\n",s,buf.st_mode % 2048);
	fprintf(stderr,"# %s owner=%d group=%d\n",s,buf.st_uid,buf.st_gid);
	}
}
void text_packet_processor(int serialfd,char *special_handling ) {
	extern void text_engine(int serialfd,char *special_handling );

	text_engine(serialfd,special_handling);
	/* this is a wrapper for the actual function in protocol_text.c */
}
void nmea0183_packet_processor(int serialfd,char *special_handling ) {
	extern void nmea0183_engine(int serialfd,char *special_handling );
	nmea0183_engine(serialfd,special_handling);
	/* this is a wrapper for the actual function in protocol_NMEA0183.c */
}
static MODES modes[] = {
	{"text",text_packet_processor},
	{"nmea0183",nmea0183_packet_processor},
	{},	/* sentinnel */
};

static void _do_speed( int *speed,char *s ) {
	char	buffer[32];
	if ( 'B' != s[0] )	{	snprintf(buffer,sizeof(buffer),"B%s",s);	s = buffer;	}
	if ( 0 == strcmp(s,"B0"))	*speed = B0;
	else if ( 0 == strcmp(s,"B50"))	*speed = B50;
	else if ( 0 == strcmp(s,"B75"))	*speed = B75;
	else if ( 0 == strcmp(s,"B110"))	*speed = B110;
	else if ( 0 == strcmp(s,"B134"))	*speed = B134;
	else if ( 0 == strcmp(s,"B150"))	*speed = B150;
	else if ( 0 == strcmp(s,"B200"))	*speed = B200;
	else if ( 0 == strcmp(s,"B300"))	*speed = B300;
	else if ( 0 == strcmp(s,"B600"))	*speed = B600;
	else if ( 0 == strcmp(s,"B1200"))	*speed = B1200;
	else if ( 0 == strcmp(s,"B1800"))	*speed = B1800;
	else if ( 0 == strcmp(s,"B2400"))	*speed = B2400;
	else if ( 0 == strcmp(s,"B4800"))	*speed = B4800;
	else if ( 0 == strcmp(s,"B9600"))	*speed = B9600;
	else if ( 0 == strcmp(s,"B19200"))	*speed = B19200;
	else if ( 0 == strcmp(s,"B38400"))	*speed = B38400;
	else if ( 0 == strcmp(s,"B57600"))	*speed = B57600;
	else if ( 0 == strcmp(s,"B115200"))	*speed = B115200;
	else if ( 0 == strcmp(s,"B230400"))	*speed = B230400;
	else {
		fprintf(stderr,"# BAD -b %s\n" "# try -b 115200\n",s);
		exit(1);
	}
}
char	*strsave(char *s )
{
	char	*ret_val = 0;

	ret_val = malloc(strlen(s)+1);
	if ( 0 != ret_val) {
	       	strcpy(ret_val,s); 
	}
	return ret_val;	
}
	
static void set_mode(MODES **modep, char *s ) {
	MODES *p = modes;
	for ( ; 0 != p->label && 0 != strcmp(s,p->label) ; p++ ) {
		;
	}

	if ( 0 == p->label ) {
		fputs("# < -m ",stderr);
		p = modes;
		for ( ; 0 != p->label; p++ ) {
			fprintf(stderr,"%s ",p->label);
		}
		fputs(">\n",stderr);	
		exit(1);
	}
		
	*modep = p;
}
static struct mosquitto * _mosquitto_startup(void) {
	char clientid[24];
	int rc = 0;


	fprintf(stderr,"# initializing mosquitto MQTT library\n");
	mosquitto_lib_init();

	memset(clientid, 0, 24);
	snprintf(clientid, 23, "mqtt-send-example_%d", getpid());
	mosq = mosquitto_new(clientid, true, 0);

	if (mosq) {
		if ( 0 != mosq,mqtt_user_name && 0 != mqtt_passwd ) {
			mosquitto_username_pw_set(mosq,mqtt_user_name,mqtt_passwd);
		}
		mosquitto_connect_callback_set(mosq, connect_callback);

		fprintf(stderr,"# connecting to MQTT server %s:%d\n",mqtt_host,mqtt_port);
		rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);

		/* start mosquitto network handling loop */
		mosquitto_loop_start(mosq);
		}

return	mosq;
}

static void _mosquitto_shutdown(void) {

	if ( mosq ) {
		
		/* disconnect mosquitto so we can be done */
		mosquitto_disconnect(mosq);
		/* stop mosquitto network handling loop */
		mosquitto_loop_stop(mosq,0);


		mosquitto_destroy(mosq);
		}

	fprintf(stderr,"# mosquitto_lib_cleanup()\n");
	mosquitto_lib_cleanup();
}
int serToMQTT_pub(const char *message, const char *topic  ) {
	int rc = 0;
	if ( 0 == quiet_flag ) {
		fputs(message,stdout);
		fflush(stdout);
	}
	
	if ( 0 == disable_mqtt_output ) {

		static int messageID;
		/* instance, message ID pointer, topic, data length, data, qos, retain */
		rc = mosquitto_publish(mosq, &messageID, topic, strlen(message), message, 0, 0); 

		if (0 != outputDebug) fprintf(stderr,"# mosquitto_publish provided messageID=%d and return code=%d\n",messageID,rc);

		/* check return status of mosquitto_publish */ 
		/* this really just checks if mosquitto library accepted the message. Not that it was actually send on the network */
		if ( MOSQ_ERR_SUCCESS == rc ) {
			/* successful send */
		} else if ( MOSQ_ERR_INVAL == rc ) {
			fprintf(stderr,"# mosquitto error invalid parameters\n");
		} else if ( MOSQ_ERR_NOMEM == rc ) {
			fprintf(stderr,"# mosquitto error out of memory\n");
		} else if ( MOSQ_ERR_NO_CONN == rc ) {
			fprintf(stderr,"# mosquitto error no connection\n");
		} else if ( MOSQ_ERR_PROTOCOL == rc ) {
			fprintf(stderr,"# mosquitto error protocol\n");
		} else if ( MOSQ_ERR_PAYLOAD_SIZE == rc ) {
			fprintf(stderr,"# mosquitto error payload too large\n");
		} else if ( MOSQ_ERR_MALFORMED_UTF8 == rc ) {
			fprintf(stderr,"# mosquitto error topic is not valid UTF-8\n");
		} else {
			fprintf(stderr,"# mosquitto unknown error = %d\n",rc);
		}
	}


	return	rc;
}
enum arguments {
	A_protocol = 512,
	A_input_port,
	A_input_speed,
	A_special_handling,
	A_disable_mqtt,
	A_mqtt_host,
	A_mqtt_topic,
	A_mqtt_port,
	A_mqtt_user_name,
	A_mqtt_password,
	A_no_meta,
	A_timeout,
	A_alarm_no_data_after_start,
	A_sleep_before_startup,
	A_quiet,
	A_verbose,
	A_help,
};

int main(int argc, char **argv) {
	char portname[128] = "/dev/ttyAMA0";
	int serialfd;
	int i,n;
	int _baud = B4800;
	MODES *_mode = 0;
	char *_M = 0;

	while (1) {
		// int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			/* normal program */
		        {"protocol",                         1,                 0, A_protocol },
		        {"input-port",                       1,                 0, A_input_port },
		        {"input-speed",                      1,                 0, A_input_speed },
		        {"special-handling",                 1,                 0, A_special_handling },
		        {"mqtt-host",                        1,                 0, A_mqtt_host },
		        {"mqtt-topic",                       1,                 0, A_mqtt_topic },
		        {"mqtt-port",                        1,                 0, A_mqtt_port },
		        {"mqtt-user-name",                   1,                 0, A_mqtt_user_name },
		        {"mqtt-passwd",                      1,                 0, A_mqtt_password },
		        {"timeout",                          1,                 0, A_timeout },
		        {"alarm-no-data-after-start",        1,                 0, A_alarm_no_data_after_start },
		        {"sleep-before-startup",             1,                 0, A_sleep_before_startup },
			{"no-meta",                          no_argument,       0, A_no_meta },
			{"disable-mqtt",                     no_argument,       0, A_disable_mqtt },
			{"quiet",                            no_argument,       0, A_quiet, },
			{"verbose",                          no_argument,       0, A_verbose, },
		        {"help",                             no_argument,       0, A_help, },
			{},
		};

		n = getopt_long(argc, argv, "", long_options, &option_index);

		if (n == -1) {
			break;
		}
		

	/* command line arguments */
		switch (n) {
			case A_no_meta:
				no_meta = 1;
				fprintf(stderr,"# no meta-data to be output\n");
				break;
			case A_disable_mqtt:
				disable_mqtt_output = 1;
				break;
			case A_mqtt_topic:	
				strncpy(mqtt_topic,optarg,sizeof(mqtt_topic));
				break;
			case A_mqtt_host:	
				strncpy(mqtt_host,optarg,sizeof(mqtt_host));
				break;
			case A_mqtt_port:
				mqtt_port = atoi(optarg);
				break;
			case A_mqtt_user_name:
				mqtt_user_name = strsave(optarg);
				break;
			case A_mqtt_password:
				mqtt_passwd = strsave(optarg);
				break;
			case A_input_speed:
				_do_speed(&_baud,optarg);
				break;
			case A_special_handling:	/* overRide mode paraments */
				_M = strsave(optarg);
				break;
			case A_protocol:	/* set the mode of operation */
				set_mode(&_mode,optarg);
				break;
			case A_timeout:
				milliseconds_timeout=atoi(optarg);
				fprintf(stdout,"# timeout packet after %d milliseconds since start\n",milliseconds_timeout);
				break;
			case A_alarm_no_data_after_start:
				alarmSeconds=atoi(optarg);
				fprintf(stdout,"# terminate program after %d seconds without receiving data\n",alarmSeconds);
				break;
			case A_sleep_before_startup:
				n=atoi(optarg);
				fprintf(stdout,"# Delaying startup for %d seconds ",n);
				fflush(stdout);
				for ( i=0 ; i<n ; i++ ) {
					sleep(1);
					fputc('.',stdout);
					fflush(stdout);
				}
				fprintf(stdout," done\n");
				fflush(stdout);
				break;
			case A_input_port:
				strncpy(portname,optarg,sizeof(portname));
				portname[sizeof(portname)-1]='\0';
				fprintf(stderr,"# serial port = %s\n",portname);
				break;
			case A_verbose:
				outputDebug=1;
				fprintf(stderr,"# verbose (debugging) output to stderr enabled\n");
				break;
			case A_quiet:
				quiet_flag = 1;
				fprintf(stderr,"# quiet no output packets to standard out\n");
				break;
			case A_help:
				fprintf(stdout,"# --no-meta\t\t\tdisable meta-data output\n");
				fprintf(stdout,"# --disable-mqtt\t\tdisable mqtt output\n");
				fprintf(stdout,"# --special-handing\t\tmode or protocol special handling\n");
				fprintf(stdout,"# --mqtt-topic\t\t\tmqtt topic\n");
				fprintf(stdout,"# --mqtt-host\t\t\tmqtt host\n");
				fprintf(stdout,"# --mqtt-port\t\t\tmqtt port(optional)\n");
				fprintf(stdout,"# --mqtt-user-name\t\tmaybe required depending on system\n");
				fprintf(stdout,"# --mqtt-passwd\t\t\tmaybe required depending on system\n");
				fprintf(stdout,"# --protocol\t\t\tprotocol\n");
				fprintf(stdout,"# --alarm-no-data-after-start\tseconds\tTerminate after seconds without data\n");
				fprintf(stdout,"# --timeout\t\t\tmilliseconds\tTimeout packet after milliseconds since start\n");
				fprintf(stdout,"# --sleep-before-startup\tseconds\tstartup delay\n");
				fprintf(stdout,"# --verbose\t\t\tOutput verbose / debugging to stderr\n");
				fprintf(stdout,"# --input-port\t\t\tserial device to use (default: /dev/ttyAMA0)\n");
				fprintf(stdout,"# --input-speed\t\t\tserial baud to use (default: 4800)\n");
				fprintf(stdout,"#\n");
				fprintf(stdout,"# --help\t\t\tThis help message then exit\n");
				fprintf(stdout,"#\n");
				exit(0);
		}
	}
	if ( 0 == _mode ) { 
		fputs("# --protocol is missing and is required.\n",stderr); 
		exit(1); 
	}
	if ( 0 == disable_mqtt_output && ' ' >= mqtt_host[0] ) { 
		fputs("# <--mqtt-host is requied when outputting to mqtt>\n",stderr); 
		exit(1); 
	} else {
		fprintf(stderr,"# --mqtt_host=%s\n",mqtt_host);
	}
	if ( 0 == disable_mqtt_output && ' ' >= mqtt_topic[0] ) { 
		fputs("# <--mqtt_topic> is required  when outputting to mqtt\n",stderr); 
		exit(1); 
	} else {
		fprintf(stderr,"# --mqtt_topic=%s\n",mqtt_topic);
	}

	if ( 0 == disable_mqtt_output && 0 == _mosquitto_startup() ) {
		return	1;
	}


	/* install signal handler */
	signal(SIGALRM, signal_handler); /* timeout */
	signal(SIGUSR1, signal_handler); /* user signal to do data block debug dump */
	signal(SIGPIPE, signal_handler); /* broken TCP connection */

	/* setup serial port for NMEA */
	serialfd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	
	if (serialfd < 0) {
		_report_reason_for_serialfd_failure(portname);
		exit(1);
	}	

	set_interface_attribs (serialfd, _baud, 0);  /* set speed to default or -b baud  bps, 8n1 (no parity) */
	set_blocking (serialfd, 0, 0);		/* blocking with 0 second timeout.  this will be re-set in the mode */

	_M = ( 0 == _M ) ? "" : _M;


	(*_mode->packet_processor_func)(serialfd,_M );	/* call the mode specified on the cmd_line with its special stuff */

	if ( 0 == disable_mqtt_output ) {
		_mosquitto_shutdown();
	}

	return	0;
}
