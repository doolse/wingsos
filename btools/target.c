#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include "asm.h"

void inittarget() {
#ifdef __JOS__

	
#else
	char *home = getenv("HOME");

	if (home) {
		char *fname;
		FILE *fp;
		
		fname = mymalloc(strlen(home)+5);
		strcpy(fname, home);
		strcat(fname, "/.ja");
		fp = fopen(fname,"r");
		free(fname);
		if (fp) {
			fgets(ident, MAXLABEL, fp);
			if (ident[0]) {
				if (fname = strrchr(ident, '\n'))
					*fname = 0;
				sysdir = strdup(ident);
			}
		}
	}
#endif
}
