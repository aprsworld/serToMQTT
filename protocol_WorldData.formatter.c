
enum formats {
	F_XRW2G = 23,
};

struct json_object *json_object_new_analog( uint8_t *s ) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	uint16_t i;

	i = s[0];
	i <<= 8;
	i += s[1];
	json_object_object_add(jobj,"analogCurrent",json_object_new_int(i));

	i = s[2];
	i <<= 8;
	i += s[3];
	json_object_object_add(jobj,"analogAverage",json_object_new_int(i));

	i = s[4];
	i <<= 8;
	i += s[5];
	json_object_object_add(jobj,"analogSTD",json_object_new_int(i));

	return	jobj;
}
struct json_object *json_object_new_pulse( uint8_t *s ) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	uint16_t i;
	unsigned int I;
	i = s[0];
	i <<= 8;
	i += s[1];
	json_object_object_add(jobj,"pulseCount",json_object_new_int(i));

	i = s[2];
	i <<= 8;
	i += s[3];
	json_object_object_add(jobj,"pulseTime",json_object_new_int(i));

	i = s[4];
	i <<= 8;
	i += s[5];
	json_object_object_add(jobj,"pulseMinTime",json_object_new_int(i));

	i = s[6];
	i <<= 8;
	i += s[7];
	json_object_object_add(jobj,"pulseMaxTime",json_object_new_int(i));

	I = s[8];
	I <<= 8;
	I += s[9];
	I <<= 8;
	I += s[10];
	I <<= 8;
	I += s[11];
	json_object_object_add(jobj,"pulseSum",json_object_new_int(I));

	return	jobj;
}
static void do_LL_xrw2g_pulseCountAnemometer(struct json_object *jobj, int timeInterval, struct json_object *pulses ) {
	char buffer[64] = {};
	int rc, pulseCount;
	double adjusted_pulseCount;

	LL_xrw2g_pulseCountAnemometer *p = xrw2g_pulseCountAnemometers_root;

	for ( ; 0 != p; p = p->next ) {
		struct json_object *tmp;
		snprintf(buffer,sizeof(buffer),"/%d/pulseCount",p->pulse_channel);
		rc = json_pointer_get(pulses,buffer,&tmp);
		if ( 0 == rc ) { /* success */
			pulseCount = json_object_get_int(tmp);
			if ( 0 == pulseCount || 65535 == pulseCount ) {
				adjusted_pulseCount = 0.0;
			} else {
				if ( 0 != timeInterval ) {
					adjusted_pulseCount = ((double) pulseCount/(double) timeInterval)*(p->M) + p->B;
				} else {
					fprintf(stderr,"# 0 == timeInterval\n");
					adjusted_pulseCount = 0.0;
				}
			}
	
		json_object_object_add(jobj,p->channelName,json_division(adjusted_pulseCount ,p->Title,p->Units));
		}
	}

}
static void do_LL_xrw2g_pulseTimeAnemometer(struct json_object *jobj,  struct json_object *pulses ) {
	char buffer[64] = {};
	int rc, pulseTime;
	double adjusted_pulseTime;

	LL_xrw2g_pulseTimeAnemometer *p = xrw2g_pulseTimeAnemometers_root;

	for ( ; 0 != p; p = p->next ) {
		struct json_object *tmp;
		snprintf(buffer,sizeof(buffer),"/%d/pulseTime",p->pulse_channel);
		rc = json_pointer_get(pulses,buffer,&tmp);
		if ( 0 == rc ) { /* success */
			pulseTime = json_object_get_int(tmp);
			if ( 0 == pulseTime || 65535 == pulseTime ) {
				adjusted_pulseTime = 0.0;
			} else {
				adjusted_pulseTime = (p->M *10000.0)/pulseTime + p->B;
			}
	
		json_object_object_add(jobj,p->channelName,json_division(adjusted_pulseTime ,p->Title,p->Units));
		}
	}

}
static void do_LL_xrw2g_linear(struct json_object *jobj, struct json_object *analogs ) {
	char buffer[64] = {};
	int rc, analogCurrent;
	double adjusted_analogCurrent;

	LL_xrw2g_linear *p = xrw2g_linears_root;

	for ( ; 0 != p; p = p->next ) {
		struct json_object *tmp;
		snprintf(buffer,sizeof(buffer),"/%d/analogCurrent",p->analog_channel);
		rc = json_pointer_get(analogs,buffer,&tmp);
		if ( 0 == rc ) { /* success */
			analogCurrent = json_object_get_int(tmp);
			adjusted_analogCurrent = p->M * analogCurrent + p->B;
	
			json_object_object_add(jobj,p->channelName,json_division(adjusted_analogCurrent ,p->Title,p->Units));
		}
	}
}
static void do_LL_xrw2g_thermistorNTC(struct json_object *jobj, struct json_object *analogs ) {
	char buffer[64] = {};
	int rc, analogCurrent;
	double adjusted_analogCurrent;

	LL_xrw2g_thermistorNTC *p = xrw2g_thermistorNTCs_root;

	for ( ; 0 != p; p = p->next ) {
		struct json_object *tmp;
		snprintf(buffer,sizeof(buffer),"/%d/analogCurrent",p->analog_channel);
		rc = json_pointer_get(analogs,buffer,&tmp);
		if ( 0 == rc ) { /* success */
			analogCurrent = json_object_get_int(tmp);
			//1.0/( (1.0/beta)*ln(((voltage*rsource)/(vsource-voltage))/beta25) + 1.0/298.15); 
			adjusted_analogCurrent = 7.25;	// stub
	
			json_object_object_add(jobj,p->channelName,json_division(adjusted_analogCurrent ,p->Title,p->Units));
		}
	}
}
struct json_object *xrw2g_math( int timeInterval, struct json_object *pulses, struct json_object *analogs ) {
	struct json_object *jobj;
	jobj = json_object_new_object();

	do_LL_xrw2g_pulseCountAnemometer(jobj , timeInterval , pulses);
	do_LL_xrw2g_pulseTimeAnemometer(jobj ,  pulses);
	do_LL_xrw2g_linear(jobj ,  analogs);
	do_LL_xrw2g_thermistorNTC(jobj ,  analogs);

	return jobj;
}
struct json_object *_XRW2G(uint8_t *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	char	buffer[32] = {};
	uint16_t i;

	buffer[0] = s[0];
	json_object_object_add(jobj,"stx",json_object_new_string(buffer));

	buffer[0] = s[1];
	json_object_object_add(jobj,"unitIdPrefix",json_object_new_string(buffer));

	i = s[2];
	i <<=8;
	i += s[3];
	json_object_object_add(jobj,"unitId",json_object_new_int(i));

	i = s[4];
	json_object_object_add(jobj,"packetLength",json_object_new_int(i));

	i = s[5];
	json_object_object_add(jobj,"packetType",json_object_new_int(i));

	i = s[6];
	i <<=8;
	i += s[7];
	json_object_object_add(jobj,"sequence",json_object_new_int(i));

	struct json_object *pulses = json_object_new_array();;

	json_object_array_add(pulses,json_object_new_pulse(s+8));
	json_object_array_add(pulses,json_object_new_pulse(s+20));
	json_object_array_add(pulses,json_object_new_pulse(s+32));

	json_object_object_add(jobj,"pulses",pulses);

	struct json_object *analogs = json_object_new_array();;
	json_object_array_add(analogs,json_object_new_analog(s+44));
	json_object_array_add(analogs,json_object_new_analog(s+50));
	json_object_array_add(analogs,json_object_new_analog(s+56));
	json_object_array_add(analogs,json_object_new_analog(s+62));
	json_object_array_add(analogs,json_object_new_analog(s+68));
	json_object_array_add(analogs,json_object_new_analog(s+74));
	json_object_array_add(analogs,json_object_new_analog(s+80));
	json_object_array_add(analogs,json_object_new_analog(s+86));
	json_object_object_add(jobj,"analogs",analogs);

	
	i = s[92];
	i <<=8;
	i += s[93];
	json_object_object_add(jobj,"uptimeMinutes",json_object_new_int(i));

	i = s[94];
	i <<=8;
	i += s[95];
	json_object_object_add(jobj,"intervalMS",json_object_new_int(i));

	json_object_object_add(jobj,"derivatives",xrw2g_math(i,pulses,analogs));

	

	return	jobj;
}
	
	
static struct json_object *do_WorldData_FORMAT(uint8_t *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();

	alarm(0);
	//json_object_object_add(jobj,"WC",_WC(s));	
	switch ( s[5])  {
		case F_XRW2G:
			json_object_object_add(jobj,"XRW2G",_XRW2G(s));	



	}
	return	jobj;
}
