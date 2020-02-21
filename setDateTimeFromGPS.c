/* ---------------------------------------------------------------------------------
$GPRMC   sends both the date and time

no other function sends  date

when we get the date and time from _RMC then we will set the system time 
-------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

int setDateTimeFromGPS(char *gps_date,char *gps_time ) {
	struct tm now = {};
	struct timeval tv = {};
	int rc = -1;
	/*  gps_date format is mm-dd-yyyy */
	now.tm_year = atoi(gps_date+6) - 1900;
	now.tm_mon = atoi(gps_date) -1 ;
	now.tm_mday = atoi(gps_date+3);
	now.tm_hour = atoi(gps_time);
	now.tm_min = atoi(gps_time+3);
	now.tm_sec = atoi(gps_time+6);

	tv.tv_sec = mktime(&now);
	if ( -1 == tv.tv_sec ) {
		fprintf(stderr,"# cannot mktime().  %s\n",strerror(errno));
	} else {
		/* so far so good */
		rc = settimeofday(&tv,( const struct timezone *) 0);
		if ( 0 != rc ) {
			fprintf(stderr,"# cannot settimeofday().  %s\n",strerror(errno));
		}
	}
	
return	rc;

}

