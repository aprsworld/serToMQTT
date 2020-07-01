#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <json.h>
#include <math.h>

extern int retainedFlag;

struct json_object *json_division(double value,char *description, char *units) {
	struct json_object *jobj = json_object_new_object();
	if ( 0 == retainedFlag ) {
		if ( isnan(value )) {
			json_object_object_add(jobj,"value",NULL);
		} else {
			json_object_object_add(jobj,"value",json_object_new_double(value));
		}
	} else {
		json_object_object_add(jobj,"description",json_object_new_string(description));
		json_object_object_add(jobj,"units",json_object_new_string(units));
	}
	return	jobj;
}
struct json_object *json_int_division(int value,char *description, char *units) {
	struct json_object *jobj = json_object_new_object();
	if ( 0 == retainedFlag ) {
		json_object_object_add(jobj,"value",json_object_new_int(value));
	} else {
		json_object_object_add(jobj,"description",json_object_new_string(description));
		json_object_object_add(jobj,"units",json_object_new_string(units));
	}
	return	jobj;
}
struct json_object *json_string_division(char * value,char *description, char *units) {
	struct json_object *jobj = json_object_new_object();
	if ( 0 == retainedFlag ) {
		json_object_object_add(jobj,"value",json_object_new_string(value));
	} else {
		json_object_object_add(jobj,"description",json_object_new_string(description));
		json_object_object_add(jobj,"units",json_object_new_string(units));
	}
	return	jobj;
}
