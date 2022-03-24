/*
 * The sphere sends the same data as the mini except:
 * All humidity data is omitted.
 * All magnetic data is omitted.
 *
 * To set up a new sphere you must do the following
 *    1.   Set minicom 115200n81
 *    2.  Turn off hardware handshake
 *    3.  Turn on software handshake
 *    4.  ctrl-C
 *    5.  display
 *    6.  use the show command for each column of data.
 *    7.  use nvwrite to save that status.
 */

static struct json_object *do_TRISONIC_SPHERE_FORMAT(char *s) {
	struct json_object *jobj;
	jobj = json_object_new_object();
	char buffer[512] = {};
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

	return	jobj;
		

	format_failure:
	
	fprintf(stderr,"format_failure %s\n",p);

	return	jobj;
}

