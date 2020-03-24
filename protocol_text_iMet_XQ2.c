

static char *_date_clean(char *s ) {
	static char	buffer[16] = {};
	char *d = buffer;
	for ( ; '\0' != s[0] ; s++ ) {
		s[0] = ('/' == s[0] ) ? '-' : s[0];
		d[0] = s[0];
		d++;
	}
		
	return	buffer;
}


static struct json_object *do_iMet_XQ2_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	char buffer[128];
	char *p,*q,*r;
	static int _Count;

	if ( 0 == _Count ) {
		retainedFlag = 1;
	}

	_Count++;
	strncpy(buffer,s,sizeof(buffer));
	q = buffer;
		
	if ( 0 != (p = strsep(&q,",") )) {
		/* json_object_object_add(jobj,"XQ",json_object_new_string(p))*/;
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"pressure",json_division(atof(p)/100.0,"atmospheric pressure","hPa"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"temperature",json_division(atof(p)/100.0,"atmospheric temperature","degrees C"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"relativeHumidity",json_division(atof(p)/10.0,"relative humidity","%"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"relativeHumidityTemperature",
			json_division(atof(p)/100.0,"relative temperature","degrees C"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		 r = _date_clean(p);
	}
	if ( 0 != (p = strsep(&q,",") )) {
		snprintf(buffer,sizeof(buffer),"%s %s",r,p);
		json_object_object_add(jobj,"date",json_string_division(buffer,"date stamp","time"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"longitude", json_division(atof(p)/10000000.0,"longitude","degrees"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"latitude",json_division(atof(p)/10000000.0,"latitude","degrees"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"altitude", json_division(atof(p)/1000.0,"altitude","meters"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"sateliteCount",json_int_division(atoi(p),"count","satelites"));
	}
		

	return	jobj;
}


