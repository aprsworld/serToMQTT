
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
static struct json_object * _WPV( char *s ) {
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
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"measuredWindSpeed",json_division(_atof(p),"measured wind speed","mps"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"measuredWindDirection",
		json_division(_atof(p),"measured wind direction relative to datum","degrees"));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"errorFlag",
		json_string_division(p,"0 == No error",""));
	parse_failed:
	return jobj;
}
static struct json_object * _MWV( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q,*r;
	static int _Count;

	if ( 5 == _Count ) {
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	r = strsep(&q,",");
	if ( 0 == r  || 'R' != r[0] ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"measuredWindAngle",json_division(_atof(p),"measured wind angel","degrees"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	r = strsep(&q,",");
	if ( 0 == r  || 'M' != r[0] ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"measuredWindSpeed",json_division(_atof(p),"measured wind speed","mps"));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"errorFlag",
		json_string_division(p,"A == No error",""));
	parse_failed:
	return jobj;
}
static struct json_object *do_FL702LT_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();

		alarm(0);
	if ( check_the_checksum(s) ) {
		json_object_object_add(jobj,"Bad checksum",json_object_new_string(""));

	} else if ( 0 == strncmp("WVP",s+4,3)) {
		 json_object_object_add(jobj,"windVelocityPolar",_WPV(s));	
	} else if ( 0 == strncmp("MWV",s+3,3)) {
		 json_object_object_add(jobj,"windVelocityNMEA",_MWV(s));	
	} else {
		 json_object_object_add(jobj,"notFound",_NOTFOUND(s));	
	}
	return	jobj;
}
