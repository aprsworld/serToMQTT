

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
		json_object_object_add(jobj,"Preasure",json_division(atof(p)/100.0,"Atmospheric preasure","hPa"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"Temperature",json_division(atof(p)/100.0,"Atmospheric temperature","degree C"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"RelativeHumidity",json_division(atof(p)/10.0,"Relative humidity","percent"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"RelativeHumidityTemperature",
			json_division(atof(p)/100.0,"Relative temperature","degree C"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"Date",json_object_new_string(_date_clean(p)));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"Time",json_object_new_string(p));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"Longitude", json_division(atof(p)/10000000.0,"Longitude","degree E-W"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"Latitude",json_division(atof(p)/10000000.0,"Latitue","degree N-S"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"Altitude", json_division(atof(p)/1000.0,"Elevation","Meters"));
	}
	if ( 0 != (p = strsep(&q,",") )) {
		json_object_object_add(jobj,"SateliteCount",json_object_new_int(atoi(p)));
	}
		

	return	jobj;
}


