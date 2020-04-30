
#include <math.h>
#include <math.h>
static double _atof(const char *s ) {
	if ( '\0' == s[0] ) {
		return	NAN;
	}
	return	atof(s);
}
static struct json_object *_WC( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char sps[64];
	static int _Count;

	if (  0 == _Count ) {
		retainedFlag = 1;
	}

	_Count++;

	jobj = json_object_new_object();
	strncpy(buffer,s,sizeof(buffer));

	json_object_object_add(jobj,"messageType",json_object_new_string("WC"));
	snprintf(sps,sizeof(sps),"%d SamplesPerSecond",loadStarSamplesPerSecond);
	json_object_object_add(jobj,"WeighContinuous",
		json_division(_atof(s),sps,loadStarUNIT));
	return jobj;
}

	
static struct json_object *do_LOADSTAR_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();

	alarm(0);
	json_object_object_add(jobj,"WC",_WC(s));	
	return	jobj;
}
