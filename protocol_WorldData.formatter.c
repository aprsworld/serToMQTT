
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
	i += s[99];
	json_object_object_add(jobj,"intervalMS",json_object_new_int(i));

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
