
#ifndef _DIRENT_H
#define	_DIRENT_H

#include <stdio.h>
#include <sys/stat.h>

struct dirent {
	char d_name[128];
	int d_namelen;
	int d_type;
};

/* This structure should not be declared in a program! Just referenced */

typedef struct {
	FILE *stream;
	char *prefix;
	int mode;
	struct dirent adir;
} DIR;

extern DIR *opendir(char *);
extern struct dirent *readdir(DIR *);
extern int closedir(DIR *);

#endif /* _DIRENT_H */
