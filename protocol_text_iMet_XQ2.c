

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

static struct json_object *json_division(double value,char *description, char *units) {
	struct json_object *jobj = json_object_new_object();
	json_object_object_add(jobj,"value",json_object_new_double(value));
	json_object_object_add(jobj,"description",json_object_new_string(description));
	json_object_object_add(jobj,"units",json_object_new_string(units));
	return	jobj;
}



static struct json_object *do_iMet_XQ2_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	char buffer[128];
	char *p,*q;
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
		json_object_object_add(jobj,"date",json_object_new_string(_date_clean(p)));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"time",json_object_new_string(p));
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
		json_object_object_add(jobj,"sateliteCount",json_object_new_int(atoi(p)));
	}
		

	return	jobj;
}


