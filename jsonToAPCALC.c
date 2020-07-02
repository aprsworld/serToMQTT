

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <json.h>
#include <errno.h>

#include "json_pointer.h"
#include <json_object_iterator.h>
#include "serToMQTT.h"

typedef struct Llist {
	char *name;
	json_object *jobj;
	enum json_type type;
	int periodCount;	
	int arrayOfObjects;
	struct Llist *next;
} LLIST;

static void add_ll(LLIST ** rootP, char *name, json_object *value ) {
	LLIST *p;
	LLIST *q = calloc(1,sizeof(LLIST));

	q->name = strsave(name);
	q->jobj = value;

	if ( 0 == *rootP ) {
		*rootP = q;
		return;
	}

	p = *rootP;
	while ( p->next ) {
		p = p->next;
	}
	p->next = q;


}
static void ll_names_value(LLIST *p, FILE *out) {
	enum json_type type;


	while ( 0 != p ) {
		type = json_object_get_type(p->jobj);
		switch ( type ) {
			case json_type_boolean: 
				fprintf(out,"%s=%s\n;",p->name,json_object_get_boolean(p->jobj)? "true": "false");
				break;
			case json_type_double: 
				fprintf(out,"%s=%lf;\n",p->name,json_object_get_double(p->jobj));
				break;
			case json_type_int: 
				fprintf(out,"%s=%d;\n",p->name,json_object_get_int(p->jobj));
				break;
			case json_type_string:
				fprintf(out,"%s=\"%s\";\n",p->name,json_object_get_string(p->jobj));
				break;
		}
		p = p->next;
	}
}
#if 0
static void ll_output_names_value(LLIST *p, FILE *out) {
	enum json_type type;


	while ( 0 != p ) {
		type = json_object_get_type(p->jobj);
		switch ( type ) {
			case json_type_boolean: 
			case json_type_double: 
			case json_type_int: 
			case json_type_string:
				fprintf(out,"print \"%s =\",%s\n",p->name,p->name);
				break;
		}
		p = p->next;
	}
}
#endif
static int count_periods( char *s ) {
	char *q;
	int i = 0;
	
	while ( 0 != s ) {
		q = strchr(s,'.');
		if ( 0 != q ) {
			i++;
			q++;
		}
		s = q;
	}
	return i;
}
static void ll_set_stuff(LLIST *p) {

	while ( 0 != p ) {
		p->periodCount = count_periods(p->name);
		p->type = json_object_get_type(p->jobj);
		p = p->next;
	}
}
static int ll_get_stuff_max_periodCount( LLIST *p) {
	int periodCount = 0;

	while ( 0 != p ) {
		periodCount = (p->periodCount > periodCount) ? p->periodCount : periodCount;
		p = p->next;
	}

	return periodCount;
}

static char  top_level[256];
static char * get_key( char *s, int periodCount ) {
	static char buffer[128];
	char *p,*q;

	memset(buffer,'\0',sizeof(buffer));
	strcpy(buffer,s);

	q = buffer;

	while ( periodCount-- ) {
		p = strchr(q,'.');
		if ( 0 != p ) {
			p++;
		}
		q = p;
	}
	if ( 0 != p ) {
		p[-1] = '\0';
	}
	return buffer;
}
static void ll_set_top_level( LLIST *p, int m ) {
	m -= 1;

	while ( 0 != p ) {
		if ( m == p->periodCount  ) {
			switch ( p->type ) {
				case json_type_boolean: 
				case json_type_double: 
				case json_type_int: 
				case json_type_string:
					strcpy(top_level,get_key(p->name,m));
					return;
			}
		}
		p = p->next;
	}
}
static void ll_names_free(LLIST *p) {
	LLIST  *q;

	while ( 0 != p ) {
		q = p;
		p = p->next;
		free(q);
	}
}
static void traverse( LLIST **root, json_object *jobj , char *label ) {

	char buffer[256];
	struct json_object_iterator now =	json_object_iter_begin (jobj);
	struct json_object_iterator end =	json_object_iter_end (jobj);
	const char *name;
	json_object *vobj;
	json_object *aobj;
	int i, arraylen;


	for ( ; 0 == (  json_object_iter_equal( &end, &now )) ; json_object_iter_next(&now) ) {
		name = json_object_iter_peek_name(&now);
		vobj = json_object_iter_peek_value(&now);
		snprintf(buffer,sizeof(buffer),"%s.%s",label,name);
		add_ll(root,buffer,vobj);
		enum json_type type, atype;
		type = json_object_get_type(vobj);
		switch (type) {
			case json_type_boolean: 
			case json_type_double: 
			case json_type_int: 
			case json_type_string:
				break; 
			case json_type_array: 
				 arraylen = json_object_array_length(vobj);
				for ( i = 0; i < arraylen; i++ ) {
					aobj = json_object_array_get_idx(vobj, i);
					atype = json_object_get_type(aobj);
					if ( atype == json_type_array) {
						/* TODO */
					}
					else {
					snprintf(buffer,sizeof(buffer),"%s.%s[%d]",label,name,i);
					traverse(root,aobj,buffer);
					}
				}
				break;
			case json_type_object:
				snprintf(buffer,sizeof(buffer),"%s.%s",label,name);
				traverse(root,vobj,buffer);
				break;
		}
	}
		


}
static char *strip_subscipt( char *s ) {
	int state = 0;
	static char buffer[128];
	char *d = buffer;

	memset(buffer,'\0',sizeof(buffer));
	

	for ( ;0 != s[0]; s++ ) {
		if ( '[' == s[0] ) {
			state++;
		} else if ( ']' == s[0] ) {
			state--;
		} else if ( 0 != state ) {
			continue;
		} else {
			d[0] = s[0];
			d++;
		}
	}
	return	buffer;	
}
static int match_key(char *s1, char *s2, int periodCount ) {
	int rv = (0 == strcmp(s1,get_key(s2,periodCount)));
	return	rv;
}
static char *base_key( char *s ) {
	char * p;
	p = strrchr(s,'.');
	return	( 0 == p ) ? s : (p + 1 );
}
static void declare(LLIST *p, int periodCount ,FILE *out) {
	int	first = 1;
	int	incomplete = 0;
	char	key[128] = {};


	for ( ; 0 != p ; p = p->next ) {
		if ( periodCount != p->periodCount ) {
			continue;
		}
		first = ( 0 == match_key(key,p->name,periodCount));
		
		if ( 0 != first ) {
			if ( incomplete ) {
				fprintf(out,"};\n");
			}
			strcpy(key,get_key(p->name,periodCount));
			p->arrayOfObjects = ( 0 != strchr(key,'['));
			fprintf(out,"obj OBJ%s{ ",strip_subscipt(base_key(key)));
			incomplete = 1;
		}
		if ( 0 == first ) {
			fprintf(out,",");
		}
		fprintf(out,"%s",base_key(p->name));
	}
	if ( incomplete ) {
		fprintf(out,"};\n");
	}

}
static void ll_object_dec(LLIST *p,int MAXperiodCount,FILE *out ) {
	int i;
	for ( i = 1 ; MAXperiodCount >= i ; i++ ) {
		declare(p,i,out);
	}
}
static void alloc(LLIST *p, int periodCount ,FILE *out) {
	char	t[64];

	for ( ; 0 != p ; p = p->next ) {
		if ( periodCount != p->periodCount ) {
			continue;
		}
		if ( json_type_object != p->type ) {
			continue;
		}
		strcpy(t,base_key(p->name));
		fprintf(out,"obj OBJ%s %s;\n",t,t);
		fprintf(out,"%s=%s\n",p->name,t);
	}

}
static void ll_object_alloc(LLIST *p,int MAXperiodCount,FILE *out ) {
	int i;
	fprintf(out,"global obj OBJ%s %s;\n","data","data");
	for ( i = 1 ; MAXperiodCount > i ; i++ ) {
		alloc(p,i,out);
	}
}
static void do_arrayOfObject(FILE *out, int arraylen, char *t, char *name){
	char buffer[64];
	snprintf(buffer,sizeof(buffer),"%sA",t);
	fprintf(out,"obj OBJ%s %sA;\n",t,t);
	fprintf(out,"mat %s[%d]={",t,arraylen);
	for ( ; 0 < arraylen; arraylen-- ) {
		fputs(buffer,out);
		if ( 1 == arraylen ) {
			continue;
		}
		fputs(",",out);
	}
	fputs("};\n",out);
	fprintf(out,"%s=%s;\n",name,t);
}
static void mat_alloc(LLIST *p, int periodCount ,FILE *out) {
	char	t[64];
	int arraylen;

	for ( ; 0 != p ; p = p->next ) {
		if ( periodCount != p->periodCount ) {
			continue;
		}
		if ( json_type_array != p->type ) {
			continue;
		}
		strcpy(t,base_key(p->name));
		arraylen = json_object_array_length(p->jobj);
		if ( 0 != p->next->arrayOfObjects ) {
			do_arrayOfObject(out,arraylen,t,p->name);
		}
		else {
			fprintf(out,"mat %s[%d];\n",t,arraylen);
		}
	}

}
static void ll_mat_alloc(LLIST *p,int MAXperiodCount,FILE *out ) {
	int i;
	for ( i = 1 ; MAXperiodCount > i ; i++ ) {
		mat_alloc(p,i,out);
	}
}
char apcalc[256] = "/usr/bin/calc";
static void _do_add_channel(json_object * jobj,  char *jsonPath, char *arg1, char *arg2, char *arg3 ) {
	struct json_object *cobj = 0;
	char	*p;

	if ( '"'  == arg1[0] ) {	/* this is a string */
		cobj = json_string_division(arg1,arg2,arg3);
	} if ( 0 != strpbrk(arg1,"0123456789") ) {
		if ( 0 != strchr(arg1,'.') ) {
			cobj = json_division(atof(arg1),arg2,arg3);
		} else {
			cobj = json_int_division(atoi(arg1),arg2,arg3);
		}
	}

	/* okay the object to be added exists */
	p = strrchr(jsonPath,'/');
	p++;
	p[-1] = '\0';
	/* p now is the name of the object and jsonPath is where it goes */
	int rc;
	json_object *tmp = NULL;
	rc = json_pointer_get(jobj,jsonPath,&tmp);

	if ( 0 != rc ) {
		fprintf(stderr,"# cannot find jsonPath %s\n",jsonPath);
		return;
	} 

	json_object_object_add(tmp,p,cobj);

	


}

static void _parse_jsonPath(char *d, char *s ) {
	char	buffer[256];
	int i;

	d[0] = '\0';


	char *p, *q;

	while ( ' ' == s[0] ) {
		s++;
	}
	strncpy(q = buffer,s,sizeof(buffer));
	if ( 0 != strncmp(s,"data.",5) ) {
		strcpy(buffer,top_level);
		strcat(buffer,".");
		strcat(buffer,s);
	}

	while ( 0 != ( p = strsep(&q,"."))) {
		if ( 0 == strcmp(p,"data")) {
			strcat(d,"/");
		} else {
			strcat(d,p);
			strcat(d,"/");
		}
	}
	while ( '/' == d[ i = strlen(d) - 1 ] ) {
		d[i] = '\0';
	}
}
static void _parse_add_channel(json_object * jobj , char *line ) {
	char	buffer[256];
	char	jsonPath[128];
	char	arg1[64];
	char	arg2[64];
	char	arg3[64];

	strncpy(buffer,line,sizeof(buffer));
	char *p, *q;
	
	q = buffer;
	while ( ' ' >= q[0] && 0 != q[0] ) {
		q++;
	}
//add channel data.formattedData.XRW2G.wtf(13,"angle of attack" ,"degrees")
	p = strsep(&q," (");
	if ( 0 != p ) {
		_parse_jsonPath(jsonPath,p);
	} else {
		fprintf(stderr,"# cannot extract  jsonPATH %s \n",p);
		return;
	}
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto bad_format;
	}
	strcpy(arg1,p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto bad_format;
	}
	strcpy(arg2,p);
	p = strsep(&q,",");
	if ( 0 == p ) {
		goto bad_format;
	}
	strcpy(arg3,p);


	_do_add_channel(jobj,jsonPath,arg1,arg2,arg3);

	bad_format:
	;;;
		
}
static void _parse_add(json_object * jobj , char *line ) {
	char	buffer[256];

	strncpy(buffer,line,sizeof(buffer));
	char *p, *q;
	
	q = buffer;
	while ( ' ' >= q[0] && 0 != q[0] ) {
		q++;
	}
	if ( 0 != q[0] ) {

		p = strsep(&q," ");
		if ( 0 == strcmp("channel",p) ) {
			_parse_add_channel(jobj,q);
		} else {
			fprintf(stderr,"# unknown command add %s %s\n",p,q);
		}
	}
}
static void _do_del(json_object ** jobjP , char *path ) {
	char jsonPath[128];

	_parse_jsonPath(jsonPath,path);
	char *p = strrchr(jsonPath,'/');
	p++;
	p[-1] = '\0';

	json_object *tmp = NULL;
	int rc = json_pointer_get(*jobjP,jsonPath,&tmp);

	if ( 0 != rc ) {
		fprintf(stderr,"# del %s failed.\n",path);
	} else {
		json_object_object_del(tmp,p);
	}
}
static void _process_this_line(json_object ** jobjP , char *line ) {
	char	buffer[256];

	strncpy(buffer,line,sizeof(buffer));
	char *p, *q;
	
	q = buffer;
	while ( ' ' >= q[0] && 0 != q[0] ) {
		q++;
	}
	if ( 0 != q[0] ) {
		p = strsep(&q," ");
		if ( 0 == strcmp("add",p) ) {
			_parse_add(*jobjP,q);
		} else if ( 0 == strcmp("del",p) ) {
			_do_del(jobjP,q);
		} else {
			fprintf(stderr,"# unknown command %s %s\n",p,q);
		}
	}


}
static void _process_commands(json_object ** jobjP , char *commands ) {
	char *tmp = malloc(strlen(commands)+1);
	if ( 0 != tmp ) {
		strcpy(tmp,commands);
	}
	char *p, *q;
	q = tmp;

	while ( p = strsep(&q,"\n") ) {
		_process_this_line(jobjP,p);

	}
	
	if ( 0 != tmp ) {
		free(tmp);
	}

	
	
}
static void _process( json_object ** jobjP, FILE *in ) {

	char	buffer[128];

	while ( fgets(buffer,sizeof(buffer),in)) {
		// fputs(buffer,stderr);
		_process_commands(jobjP,buffer);
	}
}
int jsonToAPCALC(json_object ** jobjP , char *commands ) {
	json_object *jobj = *jobjP;
	FILE *in = 0;
	FILE *out = 0;
	int MAXperiodCount = 0;
	char cmd[128] = {};
	char fifo_name[32] = {};
	static int fifo_idx;
	snprintf(fifo_name,sizeof(fifo_name),"./%ld.%d.%d",time((time_t * ) 0),fifo_idx,getpid());

	if ( mkfifo(fifo_name,0777)) {
		fprintf(stderr,"# mkfifo(\"%s\",0777) failed.\n",fifo_name);
		goto cleanup;
		}

	LLIST  *root = 0;

	snprintf(cmd,sizeof(cmd),"%s < %s",apcalc,fifo_name);

	in = popen(cmd,"r");
	if ( 0 == in ) {
		fprintf(stderr,"# %s\n",strerror(errno));
		goto cleanup;
		}

	out = fopen(fifo_name,"w");
	if ( 0 == out ) {
		fprintf(stderr,"# %s\n",strerror(errno));
		goto cleanup;
		}
	/* traverse converts jobj to a linked list LLIST where root is its base. */
	traverse(&root,jobj,"data");
	/* ll_set_stuff  sets (LLIT *p) p->periodCount and (LLIT *p) p->type */
	ll_set_stuff(root);
	/* ll_get_stuff_max_periodCount returns MAX (LLIT *p) p->periodCount */
	MAXperiodCount = ll_get_stuff_max_periodCount(root);
	/*   now set the top_level */
	ll_set_top_level(root,MAXperiodCount);

	if ( 0 == fork() ) {
		/* ll_object_dec declares all obj */
		ll_object_dec(root,MAXperiodCount,out);
		/* ll_object_alloc instantiates all obj */
		ll_object_alloc(root,MAXperiodCount,out);
		/* ll_mat_alloc instantiates all mat  */
		ll_mat_alloc(root,MAXperiodCount,out);
		/* ll_names_value assignes values to all name/value pairs */
		ll_names_value(root,out);
		fputs("/*  perform calculations here */\n",out);
//		ll_output_names_value(root,out);

		fputs(commands,out);

		/* ll_names_free frees the LLIST */
		ll_names_free(root);
		fputs("quit;\n",out);
		fflush(out);
		exit(0);
	}
	int status;
	wait(&status);
	
	cleanup:
#if 1
	if ( 0 != out ) {
		fclose(out);
	}
#endif
	if ( 0 != in ) {
		_process(jobjP,in);
		fclose(in);
	}
	if ( '\0' < fifo_name[0] ) {
		unlink(fifo_name);
	}


	return 0;
}
