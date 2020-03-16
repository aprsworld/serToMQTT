/* see https://www.hemispheregnss.com/wp-content/uploads/2020/02/hemispheregnss_technicalreferencemanual_v3.0_12302019.pdf */
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
	if ( 0 == no_meta  || 0 != retainedFlag ) {
		json_object_object_add(jobj,"description",json_object_new_string(description));
		json_object_object_add(jobj,"units",json_object_new_string(units));
	}
	return	jobj;
}
struct json_object *json_string_division(char * value,char *description, char *units) {
	struct json_object *jobj = json_object_new_object();
	json_object_object_add(jobj,"value",json_object_new_string(value));
	if ( 0 == no_meta  || 0 != retainedFlag ) {
		json_object_object_add(jobj,"description",json_object_new_string(description));
		json_object_object_add(jobj,"units",json_object_new_string(units));
	}
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
	double decimalDegrees;
	jobj = json_object_new_object();
	if ( 0 == no_meta  || 0 != retainedFlag ) {
		json_object_object_add(jobj,"degrees",json_object_new_int(degrees));
		json_object_object_add(jobj,"minutes",json_object_new_double(minutes));
	}
	decimalDegrees = 5.0 * minutes;
	decimalDegrees /= 300.0;
	decimalDegrees += (double) degrees;
	json_object_object_add(jobj,"decimalDegrees",json_object_new_double(decimalDegrees));
	if ( 0 == no_meta  || 0 != retainedFlag ) {
		json_object_object_add(jobj,( 0 == mode ) ? "NS" : "EW",json_object_new_string(flag));
	}


	return jobj;
}
static struct json_object * _VTG( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	double value;
	char	*units;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
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
static struct json_object * _ZDA( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char	timestamp[32];
	char	datestamp[32];
	int	mm,dd,yyyy;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	
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
	dd = atoi(p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	mm = atoi(p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	yyyy = atoi(p);
	snprintf(datestamp,sizeof(datestamp),"%02d-%02d-%04d",mm,dd,yyyy);
	json_object_object_add(jobj,"date",json_object_new_string(datestamp));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"localZoneHours",json_int_division(atoi(p),"local TZ offset","hours"));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"localZoneMinutes",json_int_division(atoi(p),"local TZ offset","minutes"));




	parse_failed:
	return jobj;
}
static struct json_object * _HEV( char *s ) {
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"Heave",
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"heave", "meters"));
	parse_failed:
	return jobj;
}
static struct json_object * _GNS( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char	timestamp[32];
	char *p,*q;
	int degrees;
	double minutes,value;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	
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
	json_object_object_add(jobj,"mode",json_object_new_string(p));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"satCount",json_object_new_int(atoi(p)));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"HDOP",json_division(atof(p),"horizontal dilution of precision","unknown"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"antennaAlt",json_division(value,"antenna altitude","meters")); /* a.a */

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"geodialSep",json_division(value,"geodial separation","meters"));	/*g.g */
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"ageDiff",json_int_division(atoi(p),"age of differential correction","seconds"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"diffID",json_object_new_int(atoi(p)));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"navStatus",json_object_new_string(p));


	parse_failed:
	return jobj;
}
static struct json_object * _GGA( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char	timestamp[32];
	char *p,*q;
	int degrees;
	double minutes,value;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	
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
	json_object_object_add(jobj,"qualityFlag",json_object_new_int(atoi(p)));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"satCount",json_object_new_int(atoi(p)));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"HDOP",json_division(atof(p),"horizontal dilution of precision","unknown"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"antennaAlt",json_division(value,"antenna altitude",p));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"geodialSep",json_division(value,"geodial separation",p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"ageDiff",json_int_division(atoi(p),"age of differential correction","seconds"));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"diffID",json_object_new_int(atoi(p)));


	parse_failed:
	return jobj;
}
static struct json_object * _GSA( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	int i;
	static int _Count;

	if ( 0 == _Count ) {
		retainedFlag = 1;
	}

	_Count++;

	jobj = json_object_new_object();
	struct json_object *satelites = json_object_new_array();;

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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	
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
	static int _Count;

	if ( 0 == _Count ) {
		retainedFlag = 1;
	}

	_Count++;

	static int count;

	count++;

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

	if ( setTimeStartupCount ) {
		int rc = setDateTimeFromGPS(datestamp,timestamp);
		if ( 0 == rc ) {
			setTimeStartupCount--;
		}
	} else if ( 0 != setTimeIntervalCount && 0 == (count % setTimeIntervalCount )) {
		(void) setDateTimeFromGPS(datestamp,timestamp);	/* ignore whether in succeeds */
	}



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
	static int _Count;

	if ( 0 == _Count ) {
		retainedFlag = 1;
	}

	_Count++;

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

static struct json_object * _GST( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char timestamp[32];
	char *p,*q;
	double value;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
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
	value = atof(p);
	json_object_object_add(jobj,"rangeRms", json_division(value,"RMS value of the standard diviation", "meters"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"stdMajor", json_division(value,"standard deviation of semi-major axis", "meters"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"stdMinor", json_division(value,"standard deviation of semi-minor axis", "meters"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"orient", json_int_division(value,"orientation of semi-major axi", "degree"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"stdLat", json_division(value,"standard deviation of latitude", "meters"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"stdLong", json_division(value,"standard deviation of longitude", "meters"));

	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"stdAlt", json_division(value,"standard deviation of altitude", "meters"));
	parse_failed:
	return jobj;
}
	
static struct json_object * _HDT( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	double value;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"currHD", 
		( '\0' == p[0] ) ? NULL :json_division( value ,"current heading true", "degrees"));
	
	parse_failed:
	return jobj;
}
static struct json_object * _ROT( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	double value;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"currROT", 
		( '\0' == p[0] ) ? NULL :json_division( value ,"current rate of turn", "degrees per minute"));
	
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"validityFlag",json_object_new_string(p));
	parse_failed:
	return jobj;
}

static struct json_object * _HPR( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char timestamp[32];
	char *p,*q;
	double value;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
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
	value = atof(p);
	json_object_object_add(jobj,"currHD", 
		( '\0' == p[0] ) ? NULL :json_division( value ,"current heading true", "degrees"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"currPitch", 
		( '\0' == p[0] ) ? NULL :json_division( value ,"current pitch", "degrees"));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"currRoll", 
		( '\0' == p[0] ) ? NULL :json_division( value ,"current roll", "degrees"));
	
	parse_failed:
	return jobj;
}
static struct json_object * _PSAT( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	double value;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"antennaID",json_object_new_string(p));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"manuallyEnteredSeparation",json_object_new_double(atof(p)));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"autoGPSantennaSeparation",json_object_new_double(atof(p)));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	value = atof(p);
	json_object_object_add(jobj,"currHD", 
		( '\0' == p[0] ) ? NULL :json_division( value ,"current heading true", "degrees"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"headingDevice", 
		( '\0' == p[0] ) ? NULL :json_string_division( p ,"N=GNSS, G=Gyro", ""));

	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"pitch", 
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"Pitch", "degrees"));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"roll", 
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"Roll", "degrees"));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"antennaPlacement", 
		( '\0' == p[0] ) ? NULL :json_string_division( p ,"P=front to bank,  R=left to right", "degrees"));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"satCount", 
		( '\0' == p[0] ) ? NULL :json_int_division( atoi(p) ,"Satelite count", "integer"));
	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"satUsed", 
		( '\0' == p[0] ) ? NULL :json_int_division( atoi(p) ,"Satelites used by secondary antenna", "integer"));

	/* there are other paraments but seem to be unimplemented by device makers */
	parse_failed:
	return jobj;
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
	parse_failed:
	return jobj;
}
static struct json_object * _PASHR( char *s ) {
	struct json_object *jobj;
	char buffer[256];
	char *p,*q;
	char timestamp[32];
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
	json_object_object_add(jobj,"messageType",json_object_new_string(p));
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
	json_object_object_add(jobj,"trueHeading", 
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"true heading", "decimalDegrees"));
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	if ( '\0' != p[0] && 'T' != p[0] ) {
		json_object_object_add(jobj,"headingType",json_object_new_string("unknown"));
	}
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"Roll",
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"roll", "decimalDegrees"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"Ptich",
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"pitch", "decimalDegrees"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"Heave",
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"heave", "meters"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"rollDeviation",
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"roll standard deviation", "decimalDegrees"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"pitchDeviation",
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"pitch standard deviation", "decimalDegrees"));

	p = strsep(&q,",");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"heaveDeviation",
		( '\0' == p[0] ) ? NULL :json_division( atof(p) ,"heave standard deviation", "meters"));

	p = strsep(&q,",*");
	if ( 0 == p ) {
		goto parse_failed;
	}
	json_object_object_add(jobj,"qualityFlag",
		( '\0' == p[0] ) ? NULL :json_int_division( atoi(p) ,"0=No position, 1=All non-RTK fixed integer postions " \
		"2=RTK fixed integer positions", "integer flag"));
	parse_failed:
	return jobj;
}
static struct json_object *do_NMEA0183_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();

		alarm(0);
	if ( check_the_checksum(s) ) {
		json_object_object_add(jobj,"Bad checksum",json_object_new_string(""));

	} else if ( 0 == strncmp("RMC",s+3,3)) {
		 json_object_object_add(jobj,"recommendedMinimumData",_RMC(s));	
	} else if ( 0 == strncmp("GSV",s+3,3)) {
		 json_object_object_add(jobj,"satelitesCount",_GSV(s));	
	} else if ( 0 == strncmp("VTG",s+3,3)) {
		 json_object_object_add(jobj,"courseOverGroundAndGroundSpeed",_VTG(s));	
	} else if ( 0 == strncmp("GGA",s+3,3)) {
		 json_object_object_add(jobj,"globalPositioningSystemFixData",_GGA(s));	
	} else if ( 0 == strncmp("GSA",s+3,3)) {
		 json_object_object_add(jobj,"GNSSDOPandActiveSatelites",_GSA(s));	
	} else if ( 0 == strncmp("GLL",s+3,3)) {
		 json_object_object_add(jobj,"latitudeAndLongitudeWithTimeofPositionFixAndStatus",_GLL(s));	
	} else if ( 0 == strncmp("GST",s+3,3)) {
		 json_object_object_add(jobj,"pseudoRangeErrorStatistics",_GST(s));	
	} else if ( 0 == strncmp("HDT",s+3,3)) {
		 json_object_object_add(jobj,"currentHeading",_HDT(s));	
	} else if ( 0 == strncmp("ROT",s+3,3)) {
		 json_object_object_add(jobj,"currentHeading",_ROT(s));	
	} else if ( 0 == strncmp("HPR",s+3,3)) {
		 json_object_object_add(jobj,"headingPitchRoll",_HPR(s));	
	} else if ( 0 == strncmp("GNS",s+3,3)) {
		 json_object_object_add(jobj,"GNSMessage",_GNS(s));	
	} else if ( 0 == strncmp("ZDA",s+3,3)) {
		 json_object_object_add(jobj,"timeDateUTC",_ZDA(s));	
	} else if ( 0 == strncmp("HEV",s+3,3)) {
		 json_object_object_add(jobj,"heave",_HEV(s));	
	} else if ( 0 == strncmp("PSAT",s+1,4)) {
		 json_object_object_add(jobj,"secondaryAntenna",_PSAT(s));	
	} else if ( 0 == strncmp("PASHR",s+1,5)) {
		 json_object_object_add(jobj,"trueHeadingHeavePitchRold",_PASHR(s));	
	} else {
		 json_object_object_add(jobj,"notFound",_NOTFOUND(s));	
	}
	return	jobj;
}
