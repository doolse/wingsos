#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <wgslib.h>

struct stat buf;
char yesbuf[32];

int force,verb,recursive;

void doerr(char *str, char *str2) {
	fprintf(stderr, "Error %s ", str);
	perror(str2);
}

int ask(char *str) {
	printf("Delete dir %s ? ", str);
	fflush(stdout);
	fgets(yesbuf, 31, stdin);
	return (yesbuf[0] == 'y');
}

int delfile(char *source) {
	DIR *dir;
	struct dirent *entry;
	
	if (verb)
		printf("Deleting %s\n", source);
	if (stat(source, &buf) == -1) {
		doerr("statting", source);
		return -1;
	} else {
		if (S_ISDIR(buf.st_mode)) {
			if (!recursive) {
				fprintf(stderr, "rm: %s is a dir\n", source);
				return -1;
			}
			if (!force) {
				if (!ask(source))
					return 0;
			}
			if (!(dir = opendir(source))) {
				doerr("opening", source);
				return -1;
			}
			while ((entry = readdir(dir))) {
				char *str;
				str = fpathname(entry->d_name, source, 0);
				delfile(str);
				free(str);
			}
			closedir(dir);
			if (rmdir(source))
				doerr("removing", source);
			return 0;
		} else {
			if (remove(source))
				doerr("removing", source);
		}
		
		
	}
}

int main(int argc, char *argv[]) {
	int i=1,err,ch;
	
	while ((ch = getopt(argc, argv, "rvf")) != EOF) {
		switch(ch) {
		case 'v': 
			verb = 1;
			break;
		case 'f':
			force = 1;
			break;
		case 'r':
			recursive=1;
			break;
		}
			
	}
	if (optind >= argc) {
		fprintf(stderr,"Usage: rm [-rvf] file1 file2 ...\n");
		exit(1);
	}
	i = optind;
	while (i < argc) {
		delfile(argv[i]);
		i++;
	}
}
