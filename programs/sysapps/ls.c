#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <wgslib.h>
#include <sys/stat.h>

int linfo=0;
int totty;
char modey[4];

int getType(int mode) {
	if (S_ISDIR(mode)) {
		return 'd';
	}
	if (S_ISDEV(mode)) 
		return 's';
	return '-';
}


char *getMode(int mode) {
	if (mode & S_IROTH) {
		modey[0] = 'r';
	} else modey[0] = '-';
	if (mode & S_IWOTH) {
		modey[1] = 'w';
	} else modey[1] = '-';
	if (mode & S_IXOTH) {
		modey[2] = 'x';
	} else modey[2] = '-';
	modey[3]=0;
	return modey;
}

void showEnt(char *name, struct stat *buf) {
	int i;
	int len;
	
	if (linfo) {
		printf("%c%s%10ld %s", getType(buf->st_mode), getMode(buf->st_mode), buf->st_size, name);
	} else {
		len = strlen(name);
		fputs(name,stdout);
	}
	if (S_ISDIR(buf->st_mode)) {
		putchar('/');
		len++;
	}
	if (linfo || !totty)
		putchar('\n');
	else {
		i = 20-len%20;
		while (i) {
			putchar(' ');
			i--;
		}
	}
}
			
void showDir(char *name, int putname) {

	DIR *dir;
	struct dirent *entry;
	struct stat buf;
	int err;
	char *fullname;
	
	if (putname)
		printf("\n%s:\n",name);
	dir = opendir(name);
	if (!dir) 
		perror("ls");
	else {
		while (entry = readdir(dir)) {
			fullname = fpathname(entry->d_name, name, 1);
			err = stat(fullname, &buf);
			if (err != -1)
				showEnt(entry->d_name, &buf);
			else {
				fprintf(stderr, "ls:");
				perror(entry->d_name);
			}
			free(fullname);
		}
		closedir(dir);
	}
}

int main(int argc, char *argv[]) {

	struct dirent *entry;
	char *outbuf;
	char *name="./";
	int opt,err;
	struct stat buf;
	
	totty = isatty(STDOUT_FILENO);
	while ((opt = getopt(argc, argv, "l")) != EOF) {
		switch(opt) {
		case 'l': 
			linfo = 1;
			break;
		}
	}
	opt = 0;
	while (optind < argc) {
		err = stat(argv[optind], &buf);
		if (err == -1)
			perror("ls");
		else {
			if (S_ISDIR(buf.st_mode)) {
				showDir(argv[optind], 1);
			} else showEnt(argv[optind], &buf);
		}
		optind++;
		opt = 1;
	}
	if (!opt)
		showDir(".", 0);
	if (!linfo && totty)
		putchar('\n');
}

