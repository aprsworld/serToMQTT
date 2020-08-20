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
#include <time.h>
#include <ctype.h>
#include <math.h>

extern int setDateTimeFromGPS(char *aprsworld_date);

int outputDebug=0;
int alarmSeconds=5;
int milliseconds_timeout = 500;

uint64_t microtime() {
	struct timeval time;
	gettimeofday(&time, NULL); 
	return ((uint64_t)time.tv_sec * 1000000) + time.tv_usec;
}
static double _atof(const char *s ) {
	if ( '\0' == s[0] ) {
		return	NAN;
	}
	return	atof(s);
}
static int parse_failed( char *s ) {
	if ( 0!= outputDebug ) {
		fprintf(stderr,"# %s\n",s);
	}
	return	1;
}
int  _RMC( char *s ) {
	char	timestamp[32];
	char	datestamp[32];
	char buffer[256];
	char *p,*q,*r;
	int degrees;
	double minutes;

	if ( 0 != outputDebug ) {
		fprintf(stderr,"# %s\n",s);
	}

	strncpy(buffer,s,sizeof(buffer));
	q = buffer;
	p = strsep(&q,",");
	if ( 0 == p ) {
		
		return  parse_failed("messageType");
	}
	p = strsep(&q,",");
	if ( 0 == p ) {
		return  parse_failed("timestamp");
	}
	snprintf(timestamp,sizeof(timestamp),"%2.2s:%2.2s:%s",p,p+2,p+4);
	p = strsep(&q,",");
	if ( 0 == p ) {
		return  parse_failed("status");
	}
	r = "A = Data Valid; V = Navigation Receiver Warning";
	p = strsep(&q,",");
	if ( 0 == p ) {
		return  parse_failed("latitude");
	}
	degrees = atoi(p) / 100;
	minutes = _atof(p+3);

	p = strsep(&q,",");
	if ( 0 == p ) {
		return  parse_failed("latitudeFlag");
	}
	p = strsep(&q,",");
	if ( 0 == p ) {
		return  parse_failed("longitude");
	}
	/* first 3 bytes is degrees E-W  0-180,  remainder of string is minutes */
	degrees = atoi(p) / 100;
	minutes = _atof(p+3);
	p = strsep(&q,",");
	if ( 0 == p ) {
		return  parse_failed("longitudeFlag");
	}

	p = strsep(&q,",");
	if ( 0 == p ) {
		return  parse_failed("speed");
	}


	p = strsep(&q,",");
	if ( 0 == p ) {
		return  parse_failed("course");
	}


	p = strsep(&q,",");
	if ( 0 == p ) {
		return  parse_failed("datestamp");
	}
	snprintf(datestamp,sizeof(datestamp),"20%2.2s-%2.2s-%2.2s", p+4,p+2,p);
	snprintf(buffer,sizeof(buffer),"%s %s",datestamp,timestamp);
	int rc = setDateTimeFromGPS(buffer);
	if ( 0 != rc && 1 == errno ) {
		exit(1);
	}
	return	rc;
}

enum arguments {
	A_input_port = 512,
	A_input_speed,
	A_verbose,
	A_help,
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
#include <sys/time.h>


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
	} else if ( SIGTERM == signum ) {
		fprintf(stderr,"# Terminating.\n");
		exit(0);
	} else {
		fprintf(stderr,"\n# Caught unexpected signal %d.\n",signum);
		fprintf(stderr,"# Terminating.\n");
		exit(102);
	}

}
int wait_for_read(char *s, int len, FILE *in) {
	if ( 0 != fgets(s,len,in) )
		return 1;
	do {
		usleep(50000);	/* 50 msecond */
	} while ( 0 == fgets(s,len,in));
	return	1;

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


	static char packet[256];
	static int packet_pos=0;
	static uint64_t microtime_start=0;
	static enum states state=STATE_LOOKING_FOR_STX;

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
	if ( '\0' == buff[0] ) {
		return	rc;
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
		if ( STATE_LOOKING_FOR_STX == state ) {
			if ('$' ==  buff[i] ) {
				packet_pos=0;
				microtime_start=microtime_now;
				state=STATE_IN_PACKET;
				memset(packet,'\0',sizeof(packet));
				packet[0]='$';
				milliseconds_since_stx  = 0;
			}
		}

		if ( STATE_IN_PACKET == state ) {
			if ( milliseconds_since_stx > milliseconds_timeout ) {
				packet_pos=0;
				state=STATE_LOOKING_FOR_STX;
				memset(packet,'\0',sizeof(packet));

				if ( outputDebug ) {
					fprintf(stderr, "(timeout while reading NMEA sentence)\n");
				}
				break;
			}
	
			if ( '\r' == buff[i] || '\n' == buff[i] ) {
				state=STATE_LOOKING_FOR_STX;
				alarm(0);
				/* process packet */
				if ( 0 != strstr(packet,"RMC") ) {
					rc = ( 0 ==  _RMC(packet));
				}
				if ( outputDebug ) {
					fprintf(stderr,"# %s\n",packet);
				}
				microtime_start=0;
				break;
			}

			if ( packet_pos < sizeof(packet)-1 ) {
				packet[packet_pos]=buff[i];
				packet_pos++;
				if ( outputDebug ) {
					fprintf(stderr,"# %s (%d) (%d)\n",packet,packet_pos,(int)strlen(packet));
				}
			} else {
				if ( outputDebug ) {
					printf("(packet length exceeded length of buffer!)\n");
				}
			}

		}

	}

return	rc;
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
static int set_interface_attribs (int fd, int speed, int parity) {
	struct termios tty;

	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) {
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
int main(int argc, char **argv) {
	char portname[128] = "/dev/ttyS0";
	int i,n;
	int rc = 0;
	int _baud = B9600;
	int serialfd;

	while (1) {
		// int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			/* normal program */
		        {"input-port",                       1,                 0, A_input_port },
		        {"input-speed",                      1,                 0, A_input_speed },
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
			case A_input_speed:
				_do_speed(&_baud,optarg);
				fprintf(stderr,"# --input-speed=%s\n",optarg);
				break;
			case A_input_port:
				strncpy(portname,optarg,sizeof(portname));
				portname[sizeof(portname)-1]='\0';
				fprintf(stderr,"# --input-port = %s\n",portname);
				break;
			case A_verbose:
				outputDebug=1;
				fprintf(stderr,"# --verbose (debugging) output to stderr enabled\n");
				break;
			case A_help:
				fprintf(stdout,"# --verbose\t\t\tOutput verbose / debugging to stderr\n");
				fprintf(stdout,"# --input-port\t\t\tserial device to use (default: /dev/ttyAMA0)\n");
				fprintf(stdout,"# --input-speed\t\t\tserial baud to use (default: 4800)\n");
				fprintf(stdout,"#\n");
				fprintf(stdout,"# --help\t\t\tThis help message then exit\n");
				fprintf(stdout,"#\n");
				exit(0);
		}
	}

	/* install signal handler */

	signal(SIGALRM, signal_handler); /* timeout */
	signal(SIGUSR1, signal_handler); /* user signal to do data block debug dump */
	signal(SIGTERM, signal_handler); /* user signal to terminate */
	signal(SIGPIPE, signal_handler); /* broken TCP connection */

	/* setup serial port for NMEA */
	serialfd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	
	if (serialfd < 0) {
		_report_reason_for_serialfd_failure(portname);
		exit(1);
	}	
	set_interface_attribs (serialfd, _baud, 0);  /* set speed to default or -b baud  bps, 8n1 (no parity) */

	set_blocking (serialfd, 1, 0);		// blocking wait for a character
	/* server socket */
	fd_set active_fd_set, read_fd_set;

	FD_ZERO( &active_fd_set);


	/* add serial fd to fd_set for select() */
	FD_SET(serialfd, &active_fd_set);

	/* set an alarm to send a SIGALARM if data not received within alarmSeconds */
	alarm(alarmSeconds);


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
	
	fflush(stderr);

	return	0;
}
