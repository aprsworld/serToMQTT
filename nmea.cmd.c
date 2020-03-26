#include <stdio.h>
#include <string.h>

int main(int argc, char **argv ) {
	int lChecksum,length,i;
	char	buffer[128] = {};
	if ( 2 != argc ) {
		return	-1;
	}
	length = strlen(argv[1]);
	/* calculate local checksum */
	lChecksum=0;
	for ( i=0 ; i<length ; i++ ) {
		lChecksum = lChecksum ^ argv[1][i];
	}
	snprintf(buffer,sizeof(buffer),"$%s*%02x\r\n",argv[1],lChecksum);
	fputs(buffer,stdout);
	return	0;
}

