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




extern char *optarg;
extern int optind, opterr, optopt;

int outputDebug=0;
int milliseconds_timeout=500;
int alarmSeconds=5;

typedef struct {
	char *label;
	void (*packet_processor_func)(char *packet, int length, uint64_t microtime_start, int millisec_since_start);
	int speed;
	int stx;
	int etx;
	char *portname;
	}	MODES;

static MODES this_mode;

#define DATA_BLOCK_N 32 /* maximum number of NMEA sentences */

typedef struct {
	uint64_t microtime_start;	/* time when STX was received*/
	uint8_t sentence[128];		/* full sentence ('$' to second character of checksum), null terminated */
} struct_data_block;
#define DUMP_CURRENT_JSON_SUGGESTED_SIZE (DATA_BLOCK_N * 2 * sizeof(data_block[0].sentence))

/* array of data_blocks to put Magnum network data into */
struct_data_block data_block[DATA_BLOCK_N];


uint64_t microtime() {
	struct timeval time;
	gettimeofday(&time, NULL); 
	return ((uint64_t)time.tv_sec * 1000000) + time.tv_usec;
}



static void signal_handler(int signum) {


	if ( SIGALRM == signum ) {
		fprintf(stderr,"\n# Timeout while waiting for NMEA data.\n");
		fprintf(stderr,"# Terminating.\n");
		exit(100);
	} else if ( SIGPIPE == signum ) {
		fprintf(stderr,"\n# Broken pipe.\n");
		fprintf(stderr,"# Terminating.\n");
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

static void set_blocking (int fd, int vmin, int vtime) {
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

fprintf(stderr,"# %s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));
json_object_put(jobj);
}




enum states {
STATE_LOOKING_FOR_STX,
STATE_IN_PACKET,
};

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
		if ( STATE_LOOKING_FOR_STX == state && this_mode.stx ==  buff[i] ) {
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
			if ( this_mode.etx == buff[i] ) {
				state=STATE_LOOKING_FOR_STX;

				/* process packet */
				(*this_mode.packet_processor_func)(packet,packet_pos,microtime_start,milliseconds_since_stx);
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
void nmea_packet_processor(char *packet, int length, uint64_t microtime_start, int millisec_since_start) {
	int i;
	int lChecksum,rChecksum;

	/* quick sanity check */
	if ( length < 9 )
		return;

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
		return;
	}

	/* compare local and remote checksums */
	if ( lChecksum != rChecksum ) {
		if ( outputDebug ) {
			printf("(remote and local checksum do not match!)\n");
		}
		return;
	}

#if 0
	/* at this point we have a valid NMEA sentence */

	/* scan through data blocks and replace existing sentence with same talker
	or put in first available empty or put in place of oldest */
	for ( i=0, data_pos=-1,oldest_pos=0 ; i<DATA_BLOCK_N ; i++ ) {
		/* empty position */
		if ( -1 ==data_pos && '\0' == data_block[i].sentence[0] ) {
			data_pos=i;
		}

		/* match on first 6 characters */
		if ( 0 ==strncmp(data_block[i].sentence,packet,6) ) {
			data_pos=i;
			break;
		}

		/* update oldest_pos if we are older */
		if ( data_block[i].microtime_start < data_block[oldest_pos].microtime_start ) {
			oldest_pos=i;
		}
	}

	/* copy data to appropriate position */
	if ( -1 != data_pos ) {
		data_block[data_pos].microtime_start=microtime_start;
		strcpy(data_block[data_pos].sentence,packet);
	} else { 
		if ( outputDebug ) {
			printf("(replacing data block %d (oldest) with new data since all blocks are full)\n",oldest_pos);
		}

		data_block[oldest_pos].microtime_start=microtime_start;
		strcpy(data_block[oldest_pos].sentence,packet);
	}

#endif
char	buffer[32] = {};
struct json_object *jobj;

jobj = json_object_new_object();
snprintf(buffer,sizeof(buffer),"%lu",microtime_start);
json_object_object_add(jobj,"epochMicroseconds",json_object_new_string(buffer));
json_object_object_add(jobj,"milliSecondSinceStart",json_object_new_int(millisec_since_start));
json_object_object_add(jobj,"rawData",json_object_new_string(packet));

fprintf(stderr,"# %s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));
json_object_put(jobj);

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
static MODES modes[] = {
{"text",text_packet_processor,B115200,'S','\n'},
{"nmea",nmea_packet_processor,B4800,'$','\n'},
{},	/* sentinnel */
};

static void set_mode(char *s )
{
MODES *p = modes;
for ( ; 0 != p->label && 0 != strcmp(s,p->label) ; p++ )
	;

if ( 0 == p->label )
	{
	fputs("# < -m ",stderr);
	p = modes;
	for ( ; 0 != p->label; p++ )
		fprintf(stderr,"%s ",p->label);
	fputs(">\n",stderr);	
	exit(1);
	}
	
this_mode = p[0];
}
static void _do_speed( char *s )
{
if ( 0 == strcmp(s,"B0"))	this_mode.speed = B0;
else if ( 0 == strcmp(s,"B50"))	this_mode.speed = B50;
else if ( 0 == strcmp(s,"B75"))	this_mode.speed = B75;
else if ( 0 == strcmp(s,"B110"))	this_mode.speed = B110;
else if ( 0 == strcmp(s,"B134"))	this_mode.speed = B134;
else if ( 0 == strcmp(s,"B150"))	this_mode.speed = B150;
else if ( 0 == strcmp(s,"B200"))	this_mode.speed = B200;
else if ( 0 == strcmp(s,"B300"))	this_mode.speed = B300;
else if ( 0 == strcmp(s,"B600"))	this_mode.speed = B600;
else if ( 0 == strcmp(s,"B1200"))	this_mode.speed = B1200;
else if ( 0 == strcmp(s,"B1800"))	this_mode.speed = B1800;
else if ( 0 == strcmp(s,"B2400"))	this_mode.speed = B2400;
else if ( 0 == strcmp(s,"B4800"))	this_mode.speed = B4800;
else if ( 0 == strcmp(s,"B9600"))	this_mode.speed = B9600;
else if ( 0 == strcmp(s,"B19200"))	this_mode.speed = B19200;
else if ( 0 == strcmp(s,"B38400"))	this_mode.speed = B38400;
else if ( 0 == strcmp(s,"B57600"))	this_mode.speed = B57600;
else if ( 0 == strcmp(s,"B115200"))	this_mode.speed = B115200;
else if ( 0 == strcmp(s,"B230400"))	this_mode.speed = B230400;
else 
	{
	fprintf(stderr,"# BAD speed=%s\n"
			"# try speed=B115200\n",s);
	exit(1);
	}
}
char	*_strsave(char *s )
{
char	*ret_val = 0;

ret_val = malloc(strlen(s)+1);
if ( 0 != ret_val) strcpy(ret_val,s);
return ret_val;	
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
	this_mode.stx = q[0];
else if ( 0 == strcmp(p,"etx"))
	this_mode.etx = q[0];
else if ( 0 == strcmp(p,"speed"))
	_do_speed(q);
else if ( 0 == strcmp(p,"serialport"))
	this_mode.portname = _strsave(q);
else
	{
	fprintf(stderr,"# -M option %s=%s not supported.\n",p, q);
	exit(1);
	}
}
static void overRide(char *s )
{
/*  s can contain commands of the type  $cmd=$parameter with one or more commands sepearted by whitespace */
char	buffer[256] = {};
char	*p,*q;

strncpy(buffer,s,sizeof(buffer) - 1);

q= buffer;
while ( p = strsep(&q," \t"))	// this will find zero or more commands
	_do_command(p);
}
	
int main(int argc, char **argv) {
	char portname[128] = "/dev/ttyAMA0";
	int serialfd;
	int i,n;

	/* server socket */
	fd_set active_fd_set, read_fd_set;


	/* command line arguments */
	while ((n = getopt (argc, argv, "a:hi:p:s:vt:m:M:")) != -1) {
		switch (n) {
			case 'M':	/* overRide mode paraments */
				overRide(optarg);
				break;
			case 'm':	/* set the mode of operation */
				set_mode(optarg);
				break;
			case 't':
				milliseconds_timeout=atoi(optarg);
				fprintf(stdout,"# timeout packet after %d milliseconds since start\n",milliseconds_timeout);
				break;
			case 'a':
				alarmSeconds=atoi(optarg);
				fprintf(stdout,"# terminate program after %d seconds without receiving data\n",alarmSeconds);
				break;
#if 0
			case 'p':
				tcpPort=atoi(optarg);
				fprintf(stdout,"# TCP server port = %d\n",tcpPort);
				break;
#endif
			case 's':
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
			case 'i':
				strncpy(portname,optarg,sizeof(portname));
				portname[sizeof(portname)-1]='\0';
				fprintf(stderr,"# serial port = %s\n",portname);
				break;
			case 'v':
				outputDebug=1;
				fprintf(stderr,"# verbose (debugging) output to stderr enabled\n");
				break;
			case 'h':
				fprintf(stdout,"# -a seconds\tTerminate after seconds without data\n");
				fprintf(stdout,"# -t milliseconds\tTimeout packet after milliseconds since start\n");
				fprintf(stdout,"# -s seconds\tstartup delay\n");
				fprintf(stdout,"# -p tcpPort\tTCP server port number\n");
				fprintf(stdout,"# -v\t\tOutput verbose / debugging to stderr\n");
				fprintf(stdout,"# -i\t\tserial device to use (default: /dev/ttyAMA0)\n");
				fprintf(stdout,"#\n");
				fprintf(stdout,"# -h\t\tThis help message then exit\n");
				fprintf(stdout,"#\n");
				exit(0);
		}
	}

#if 0
#endif


	/* install signal handler */
	signal(SIGALRM, signal_handler); /* timeout */
	signal(SIGUSR1, signal_handler); /* user signal to do data block debug dump */
	signal(SIGPIPE, signal_handler); /* broken TCP connection */

	/* setup serial port for NMEA */
	if ( 0 != this_mode.portname )	strncpy(portname,this_mode.portname,sizeof(portname));
	serialfd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	
	if (serialfd < 0) {
		_report_reason_for_serialfd_failure(portname);
		exit(1);
	}	

	/* trisonic-mini  runs at 115200 baud */
	set_interface_attribs (serialfd, this_mode.speed, 0);  // set speed to 4800 bps, 8n1 (no parity)
//	set_blocking (serialfd, 0, 100);		// blocking with 10 second timeout
	set_blocking (serialfd, 0, 0);		// blocking with 10 second timeout

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

	exit(0);
}
