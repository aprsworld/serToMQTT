
static struct json_object *do_TRISONIC_MINI_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	char buffer[512];
	char *p;
	strncpy(buffer,s,sizeof(buffer));
		
	if ( 0 != (p = strtok(buffer," ") )) {
		if ( 'S' != p[0] ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"speed3D",json_division(atof(p),"Wind Speed 3D","m/s"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("S2",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"speed2D",json_division(atof(p),"Wind Speed 2D","m/s"));
	}
		
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("D",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"HorizontalWind",json_division(atof(p),"Horizontal Wind Direction","degrees"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("DV",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"VerticalWind",json_division(atof(p),"Vertical Wind Direction","degrees"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("U",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"UVector",json_division(atof(p),"U Vector","m/s"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("V",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"VVector",json_division(atof(p),"V Vector","m/s"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("W",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"WVector",json_division(atof(p),"W Vector","m/s"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("T",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"Temperature",json_division(atof(p),"Air Temperature","C"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("C",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"SpeedOfSound",json_division(atof(p),"Speed of Sound","m/s"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("RHST",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"RHTempSensor",json_division(atof(p),"RH Temperature Sensor","C"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("RHSH",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"RHHumiditySensor",json_division(atof(p),"RH Humidity Sensor","%"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("H",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"Humidty",json_division(atof(p),"Humidity","%"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("DP",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"DewPoint",json_division(atof(p),"Dew Point","C"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("PST",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"PreasureTempSensor",json_division(atof(p),"Preasure Temperature Sensor","C"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("P",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"PreasureSensor",json_division(atof(p),"Atmospheric Preasure","hPa"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("AD",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"AirDensity",json_division(atof(p),"Air Density","kg/m^3"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("AX",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"LevelX",json_division(atof(p),"Level X","unknown"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("AY",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"LevelY",json_division(atof(p),"Level Y","unknown"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("AZ",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"LevelZ",json_division(atof(p),"Level Z","unknown"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("PI",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"Pitch",json_division(atof(p),"Pitch","degrees"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("RO",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"Roll",json_division(atof(p),"Roll","degrees"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("MT",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"CompassTemp",json_division(atof(p),"Compass Temperature","C"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("MX",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"MagX",json_division(atof(p),"Compass X","unknown"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("MY",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"MagY",json_division(atof(p),"Compass Y","unknown"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("MZ",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"MagZ",json_division(atof(p),"Compass Z","unknown"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("MD",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"Heading",json_division(atof(p),"Compass Heading","degrees"));
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		if (0 != strcmp("TD",p) ) {
			goto format_failure;
		}
	}
	if ( 0 != (p = strtok((char *) 0," ") )) {
		json_object_object_add(jobj,"TrueHead",json_division(atof(p),"True Heading","degrees"));
	}

		

	format_failure:
	return	jobj;
}


#if 0
MD  318 TD  318 \r

#endif
