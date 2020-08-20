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

static int antoi(char *s, int len ) {
	char	buffer[32] = {};
	memcpy(buffer,s,len);
	return	atoi(buffer);
}

int setDateTimeFromGPS(char *aprsworld_date) {
	struct tm now = {};
	struct timeval tv = {};
	struct timeval tv_before = {};
	int rc = -1;
	/*  aprsworld _date format is yyyy-mm-dd hh:mm:ss */
	/*                            01234567890123456789 */
	now.tm_year = antoi(aprsworld_date + 0,4) - 1900;
	now.tm_mon = antoi(aprsworld_date + 5,2) -1 ;
	now.tm_mday = antoi(aprsworld_date+8,2);
	now.tm_hour = antoi(aprsworld_date +  11,2);
	now.tm_min = antoi(aprsworld_date +14,2);
	now.tm_sec = antoi(aprsworld_date+17,2);

	tv.tv_sec = mktime(&now);
	if ( -1 == tv.tv_sec ) {
		fprintf(stderr,"# cannot mktime().  %s\n",strerror(errno));
	} else {
		/* so far so good */
		rc = gettimeofday(&tv_before,( struct timezone *) 0);
		rc |= settimeofday(&tv,( const struct timezone *) 0);
		if ( 0 != rc ) {
			fprintf(stderr,"# cannot settimeofday(). errno = %d %s\n",errno,strerror(errno));
		} 
		fprintf(stderr,"# settimeofday=%s\n",aprsworld_date);
		fprintf(stderr,"# settimeofday delta %ld seconds %ld micro seconds\n",
			tv_before.tv_sec - tv.tv_sec, tv_before.tv_usec - tv.tv_usec);
	}
	
return	rc;

}

