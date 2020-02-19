
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

struct json_object *json_int_division(int value,char *description, char *units) {
	struct json_object *jobj = json_object_new_object();
	json_object_object_add(jobj,"value",json_object_new_int(value));
	json_object_object_add(jobj,"description",json_object_new_string(description));
	json_object_object_add(jobj,"units",json_object_new_string(units));
	return	jobj;
}
struct json_object *json_string_division(char * value,char *description, char *units) {
	struct json_object *jobj = json_object_new_object();
	json_object_object_add(jobj,"value",json_object_new_string(value));
	json_object_object_add(jobj,"description",json_object_new_string(description));
	json_object_object_add(jobj,"units",json_object_new_string(units));
	return	jobj;
}
struct json_object * json_object_new_satelite(int svid,int elv,int az,int cno) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	json_object_object_add(jobj,"svid", json_int_division(svid,"Satelite ID", "numeric"));
	json_object_object_add(jobj,"elv", json_int_division(elv,"Elevation 0-90", "degrees"));
	json_object_object_add(jobj,"az", json_int_division(az,"Azimuth 0-359", "degrees"));
	json_object_object_add(jobj,"cno", json_int_division(cno,"c/no signal strength.  null when not tracking.", "degrees"));
	return jobj;
}
struct json_object *_longitude_latitude(int mode, int degrees,double minutes,char *flag){
	struct json_object *jobj;
	jobj = json_object_new_object();
	json_object_object_add(jobj,"degrees",json_object_new_int(degrees));
	json_object_object_add(jobj,"minutes",json_object_new_double(minutes));
	json_object_object_add(jobj,( 0 == mode ) ? "NS" : "EW",json_object_new_string(flag));


	return jobj;
}
static struct json_object * _VTG( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	double value;
	char	*units;
	jobj = json_object_new_object();

	strncpy(buffer,s,sizeof(buffer));
	q = buffer;
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"messageTtpe",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	units = p;

	json_object_object_add(jobj,"cogt", json_division(value,"course over ground (true)", "degrees true"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	units = p;

	json_object_object_add(jobj,"cogm", json_division(value,"course over ground (magnetic)", "degrees magnetic"));


	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	units = p;

	json_object_object_add(jobj,"sogn", json_division(value,"speed over ground", "knots"));


	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	units = p;

	json_object_object_add(jobj,"sogk", json_division(value,"speed over ground", "kph"));

	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}

	json_object_object_add(jobj,"posMode", json_object_new_string(p));

	parse_failed:
	return jobj;
}
static struct json_object * _GGA( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	jobj = json_object_new_object();

	strncpy(buffer,s,sizeof(buffer));
	q = buffer;
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"messageTtpe",json_object_new_string(p));
	json_object_object_add(jobj,"noFormatInfo",json_object_new_string("No format information provided."));
	
	parse_failed:
	return jobj;
	}
static struct json_object * _GSA( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	int i;
	jobj = json_object_new_object();
	struct json_object *satelites = json_object_new_array();;

	strncpy(buffer,s,sizeof(buffer));
	q = buffer;
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"messageTtpe",json_object_new_string(p));
	
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"opMode",json_string_division(p,
		"M = manually set to operate in 2D or 3D mode, A = Automatically switching between 2D and 3D","none"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"navMode",json_int_division(atoi(p),
		"Navigation mode.  see position fix flags description","integer"));

	for ( i = 0; 12 >i ; i++ ) {   /* fixed length array */
		p = strsep(&q,",");
		if ( 0 == p ) {
			goto parse_failed;
		}

		if ( '\0' == p[0] ) {
			continue;
		}
		json_object_array_add(satelites,json_object_new_int(atoi(p)));
	}
	json_object_object_add(jobj,"svid",satelites);

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"PDOP",json_division(atof(p),"position dilution of precision","unknown"));


	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"HDOP",json_division(atof(p),"horizontal dilution of precision","unknown"));


	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"VDOP",json_division(atof(p),"vertical dilution of precision","unknown"));




	parse_failed:
	return jobj;
}
static struct json_object * _GLL( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	int degrees;
	double minutes;
	char	timestamp[32];
	jobj = json_object_new_object();

	strncpy(buffer,s,sizeof(buffer));
	q = buffer;
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"messageTtpe",json_object_new_string(p));
	
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	/* first 2 bytes is degrees N-S  0-90,  remainder of string is minutes */
	degrees = atoi(p) / 100;
	minutes = atof(p+3);

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"latitude",_longitude_latitude(0,degrees,minutes,p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	/* first 3 bytes is degrees E-W  0-180,  remainder of string is minutes */
	degrees = atoi(p) / 100;
	minutes = atof(p+3);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"longitude",_longitude_latitude(1,degrees,minutes,p));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	snprintf(timestamp,sizeof(timestamp),"%2.2s:%2.2s:%s",p,p+2,p+4);
	json_object_object_add(jobj,"time",json_object_new_string(timestamp));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"status",json_object_new_string(p));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}

	json_object_object_add(jobj,"posMode", json_object_new_string(p));

	parse_failed:
	return jobj;
}
static struct json_object * _RMC( char *s ) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	char	timestamp[32];
	char	datestamp[32];
	char buffer[256];
	char *p,*q;
	int degrees;
	double minutes;

	strncpy(buffer,s,sizeof(buffer));
	q = buffer;
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"messageTtpe",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	snprintf(timestamp,sizeof(timestamp),"%2.2s:%2.2s:%s",p,p+2,p+4);
	json_object_object_add(jobj,"time",json_object_new_string(timestamp));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"status",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	/* first 2 bytes is degrees N-S  0-90,  remainder of string is minutes */
	degrees = atoi(p) / 100;
	minutes = atof(p+3);

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"latitude",_longitude_latitude(0,degrees,minutes,p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	/* first 3 bytes is degrees E-W  0-180,  remainder of string is minutes */
	degrees = atoi(p) / 100;
	minutes = atof(p+3);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"longitude",_longitude_latitude(1,degrees,minutes,p));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}

	json_object_object_add(jobj,"speed",json_division(atof(p),"speed over ground", "knots"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}

	json_object_object_add(jobj,"course",json_division(atof(p),"course over ground", "degrees"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	snprintf(datestamp,sizeof(datestamp),"%2.2s-%2.2s-20%2.2s", p+2,p,p+4);
	json_object_object_add(jobj,"date",json_object_new_string(datestamp));



	parse_failed:
	return	jobj;
}

static struct json_object * _GSV( char *s ) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	struct json_object *satelites = json_object_new_array();;
	char buffer[256];
	strncpy(buffer,s,sizeof(buffer));
	char *p,*q;
	int loopcount = 4;
	int i,numMsg,msgNum,numSV;

	q = buffer;
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"messageTtpe",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"numMsg", json_int_division(numMsg = atoi(p),"Numberof GSV messages being output", "count"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"msgNum", json_int_division( msgNum = atoi(p),"Numberof this message", "count"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"numSV", 
			json_int_division(numSV = atoi(p),"Numberof satelites in view knowing both takeID and signalID", "count"));

	if ( msgNum == numMsg ) {
		loopcount = numSV % 4;
	} 


	for ( i = 0; loopcount > i; i++ ) {
		int svid,elv,az,cno;
		p = strsep(&q,",");
		if ( 0 == p ) {
			goto parse_failed;
		}
		svid = atoi(p);
		p = strsep(&q,",");
		if ( 0 == p ) {
			goto parse_failed;
		}
		elv = atoi(p);
		p = strsep(&q,",");
		if ( 0 == p ) {
			goto parse_failed;
		}
		az = atoi(p);
		p = strsep(&q,",");
		if ( 0 == p ) {
			goto parse_failed;
		}
		cno = atoi(p);
		json_object_array_add(satelites,json_object_new_satelite(svid,elv,az,cno));

	}



	parse_failed:
	json_object_object_add(jobj,"satelites",satelites);
	return jobj;


}

	

static struct json_object *do_NMEA0183_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();

		alarm(0);
	if ( check_the_checksum(s) ) {
		json_object_object_add(jobj,"Bad checksum",json_object_new_string(""));

	} else if ( 0 == strncmp("GSV",s+3,3)) {
		 json_object_object_add(jobj,"satelitesCount",_GSV(s));	
	} else if ( 0 == strncmp("RMC",s+3,3)) {
		 json_object_object_add(jobj,"recommendedMinimumData",_RMC(s));	
	} else if ( 0 == strncmp("VTG",s+3,3)) {
		 json_object_object_add(jobj,"courseOverGroundAndGroundSpeed",_VTG(s));	
	} else if ( 0 == strncmp("GGA",s+3,3)) {
		 json_object_object_add(jobj,"globalPositioningSystemFixData",_GGA(s));	
	} else if ( 0 == strncmp("GSA",s+3,3)) {
		 json_object_object_add(jobj,"GNSSDOPandActiveSatelites",_GSA(s));	
	} else if ( 0 == strncmp("GLL",s+3,3)) {
		 json_object_object_add(jobj,"latitudeAndLongitudeWithTimeofPositionFixAndStatus",_GLL(s));	
	}
	return	jobj;
}
