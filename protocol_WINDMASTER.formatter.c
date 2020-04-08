
#include <math.h>
#include <math.h>
static double _atof(const char *s ) {
	if ( '\0' == s[0] ) {
		return	NAN;
	}
	return	atof(s);
}
static int _nmea_checksum(const char *s) {
    int c = 0;

    while (*s)
        c ^= *s++;

    return c;
}
static int check_the_checksum(char *s) {
	char buffer[128];
	char *d = buffer;
	int cs,cs2;
	for ( ; '$' != s[0] ;	s++ )	{
	}
	for ( ; '*' != s[0] ;	s++ )	{
		d[0] = s[0];
		d++;
	}
	d[0] = '\0';
	cs =_nmea_checksum(buffer+1)  ;
	sscanf(++s,"%x",&cs2);
	return	cs != cs2;
}
static int _arg(int n, char *s ) {
	char	buffer[128];
	char	*p,*q;
	strncpy( q = buffer,s,sizeof(buffer));
	for ( ; n > 0; n-- ) {
		p = strsep(&q,",");
	}

	return ( 0 != p ) ? p[0] : -1;
}
static struct json_object * _NOTFOUND( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	static int _Count;

	if ( 0 == _Count ) {
		retainedFlag = 1;
	}

	_Count++;

	jobj = json_object_new_object();
	strncpy(buffer,s,sizeof(buffer));

	q = buffer;
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	p = strsep(&q,",=");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	parse_failed:
	return jobj;
}
static struct json_object * _M1( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	static int _Count;

	if ( 5 == _Count ) {
		retainedFlag = 1;
	}

	_Count++;

	jobj = json_object_new_object();
	strncpy(buffer,s,sizeof(buffer));

	json_object_object_add(jobj,"messageType",json_object_new_string("M1"));
	q = buffer + 1;
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"AnemometerID",
		json_string_division(p,"letter A-Z","letter"));
	
	p = strsep(&q,",=");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"UWindVelocity",
		json_division(_atof(p),"wind velocity u-axis","mps"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"VWindVelocity",
		json_division(_atof(p),"wind velocity v-axis","mps"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"WWindVelocity",
		json_division(_atof(p),"wind velocity w-axis","mps"));
	p = strsep(&q,",");
	if ( 0 == p  || 'M' != p[0] ) {
		goto parse_failed;
	}
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"statusCode",
		json_string_division(p,"status code 00 - 0B","status code"));
	parse_failed:
	return jobj;
}

	
static struct json_object *do_WINDMASTER_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();

	alarm(0);
	if ( check_the_checksum(s) ) {
		json_object_object_add(jobj,"Bad checksum",json_object_new_string(""));

	} else if ( 'M' == _arg(5,s) ) {
		 json_object_object_add(jobj,"M1",_M1(s));	
	} else {
		 json_object_object_add(jobj,"notFound",_NOTFOUND(s));	
	}
	return	jobj;
}
