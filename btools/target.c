#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include "asm.h"

char *sysdir=".";
char *libdir=".";

void inittarget() {
#ifdef __JOS__

	
#elif defined(_WIN32)
	char *home = getenv("LCC65INPUTS");
	if( home )
	{
		int len;
		sysdir=strdup(home);
		len=strlen(sysdir);
		if( sysdir[len-1]=='\\' || sysdir[len-1]=='/' )
			sysdir[len-1]=0;
	}
#else
	char *home = getenv("HOME");

	if (home) {
		char *fname,*line;
		FILE *fp;
		
		fname = mymalloc(strlen(home)+5);
		line = mymalloc(256);
		strcpy(fname, home);
		strcat(fname, "/.ja");
		fp = fopen(fname,"r");
		free(fname);
		if (fp) {
			fgets(line, 256, fp);
			if (line[0]) {
				if (fname = strrchr(line, '\n'))
					*fname = 0;
				sysdir = strdup(line);
			}
			fgets(line, 256, fp);
			if (line[0]) {
				if (fname = strrchr(line, '\n'))
					*fname = 0;
				libdir = strdup(line);
			}
		}
		free(line);
	}
#endif
}

void *mymalloc(uint32 size) {
	void *val = malloc(size);
	if (val)
		return val;
	fprintf(stderr,"Ran out of memory!\n");
	exit(1);
}
