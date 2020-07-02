
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <json.h>
#include <json_object_iterator.h>
#include <mosquitto.h>
#include <time.h>
#include "serToMQTT.h"
#include <ctype.h>
#include <math.h>
#include <sys/timeb.h>

int retainedFlag;

char	*strsave(const char *s )
{
	char	*ret_val = 0;

	ret_val = malloc(strlen(s)+1);
	if ( 0 != ret_val) {
	       	strcpy(ret_val,s); 
	}
	return ret_val;	
}


json_object *parse_a_string(char *string ) {

	json_object *jobj = NULL;
	json_tokener *tok = json_tokener_new();
	const char *mystring = string;
	int stringlen = 0;
	enum json_tokener_error jerr;
	do {
		stringlen = strlen(mystring);
		jobj = json_tokener_parse_ex(tok, mystring, stringlen);
	} while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
	if (jerr != json_tokener_success) {
		fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
		// Handle errors, as appropriate for your application.
	}
	if (tok->char_offset < stringlen) {
		// Handle extra characters after parsed object as desired.
		// e.g. issue an error, parse another object from that point, etc...
	}
	return	jobj;
}
char *file_to_string(char *fname ) {
	struct stat buf;
	if ( 0 != stat(fname,&buf) ) {
		fprintf(stderr,"%s %s\n",fname,strerror(errno));
		return	0;
	}
	int fs = buf.st_size;

	if ( 0 == fs ) {
		fprintf(stderr,"# %s empty file.\n",fname);
		return	0;
	}

	char *file_buffer = calloc(1,fs+1);
	if ( 0 == file_buffer ) {
		fprintf(stderr,"# unable to calloc %d bytes\n",fs);
		return	0;
	}
	

	FILE *in = fopen(fname,"r");
	if ( 0 == in ) {
		fprintf(stderr,"%s %s\n",fname,strerror(errno));
		free(file_buffer);
		return	0;
	}

	int rd = fread(file_buffer,1,fs,in);

	fclose(in);

	if ( rd != fs ) {
		fprintf(stderr,"# %s %d bytes read != %d bytes expected.\n",fname,rd,fs);
		free(file_buffer);
		return	0;
	}
		

	return	file_buffer;


}
int main(int argc, char **argv ) {
	int i;

	if ( 3 > argc ) {
		fprintf(stderr,"%s <json_file_name> <commands>\n",argv[0]);
		return	1;
	}
	char *string = file_to_string(argv[1]);
	char *commands = file_to_string(argv[2]);

	if ( 0 == string  || 0 == commands ) {
		return	1;
	}
	json_object *jobj = parse_a_string(string);
	free(string);

	if ( 0 == jobj ) {
		fprintf(stderr,"# unable to tokenize %s\n",argv[1]);
		return	0;
	}

	struct timeb start,end;
	ftime(&start);
	for ( i = 0; 1 > i; i++ ) {
		jsonToAPCALC(&jobj,commands);
	}
	ftime(&end);

	unsigned long diff;

	diff = end.time * 1000 + end.millitm;
	diff -= start.time * 1000 + start.millitm;

	if ( 0 < i ) {
		fprintf(stderr,"# runtime %ld msec, ave runtime = %ld msec\n",diff,diff/i);
	}

	fputs(json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY),stdout);
	json_object_put(jobj);
	return	0;
}
