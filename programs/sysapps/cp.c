#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <wgslib.h>

#define THEBUF 0x7fff

char diskbuf[THEBUF];
char yesbuf[32];

struct stat buf;

int force,verb,recursive;

void doerr(char *str, char *str2) {
	fprintf(stderr, "Error %s ", str);
	perror(str2);
}

int ask(char *str) {
	printf("Overwrite file %s ? ", str);
	fflush(stdout);
	fgets(yesbuf, 31, stdin);
	return (yesbuf[0] == 'y');
}


int copyfile(char *source, char *dest) {
	int err,done,count;
	int fd,fd2;
	int mode = O_WRITE|O_EXCL|O_CREAT;
	DIR *dir;
	struct dirent *entry;

	if (verb)
		printf("Copying %s to %s\n", source, dest);
		
	if (stat(source, &buf) == -1) {
		doerr("statting", source);
		return -1;
	} else {
		if (S_ISDIR(buf.st_mode)) {
			if (!recursive) {
				fprintf(stderr, "cp: %s is a dir\n", source);
				return -1;
			}
			if (mkdir(dest, 0)) {
				doerr("creating", dest);
				return -1;
			}
			if (!(dir = opendir(source))) {
				doerr("opening", source);
				return -1;
			}
			while ((entry = readdir(dir))) {
				char *str,*str2;
				str = fpathname(entry->d_name, source, 1);
				str2 = fpathname(entry->d_name, dest, 1);
				copyfile(str, str2);
				free(str);
				free(str2);
			}
			closedir(dir);
			return 0;
		}
	}
		
	if ((fd = open(source,O_READ)) == -1) {
		doerr("opening", source);
		return -1;
	}
	
	opentry:
	if ((fd2 = open(dest, mode)) == -1) {
		if (errno == EEXIST) {
			mode = O_WRITE|O_TRUNC|O_CREAT;
			if (!force) {
				if (ask(dest))
					goto opentry;
				else {
					close(fd);
					return -1;
				}
			} else goto opentry;
		} else {
			close(fd);
			doerr("opening", dest);
			return -1;
		}
	}

	while((count = read(fd, diskbuf, THEBUF)) > 0) {
		done = 0;
		while (count) {
			int this;
			err = write(fd2, diskbuf+done, count);
			if (err == -1) {
				doerr("writing", dest);
				goto finished;
			}
			done += this;
			count -= this;
		}
	}
	err = count;
	if (err == -1)
		doerr("reading", source);
	finished:
	close(fd2);
	close(fd);
	return err;
}

int main(int argc, char *argv[]) {
	int ch;
	char *str1,*str2,*cur,*dir;
	int i,isdir=0;
	
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
	if (argc-optind < 2) {
		fprintf(stderr,
		"Usage: cp [options] INFILE1 [INFILE2..] OUTDIR|OUTFILE\n"
		" -v   Show filenames as they are copied\n"
		" -r   Recurse into directories\n"
		" -f   Force overwriting files\n");
		exit(1);
	}
	--argc;
	dir = argv[argc];
	if (stat(dir, &buf) == -1) {
		if (errno != ENOENT) {
			doerr("statting", dir);
			exit(1);
		}
	} else if (S_ISDIR(buf.st_mode)) {
		isdir=1;
	}
	if (argc-optind>2 && !isdir) {
		fprintf(stderr,"cp: Last arguement isn't a directory!\n");
		exit(1);
	}
	i = optind;
	while (i < argc) {
		cur = argv[i];
		ch = strlen(cur);
		if (ch && cur[ch-1] == '/')
			cur[ch-1] = 0;
		if (isdir) {
			str1 = strrchr(cur,'/');
			if (!str1)
				str1=cur;
			else
				str1++;
			str2 = fpathname(str1, dir, 1);
		} else str2 = dir;
		copyfile(cur, str2);
		if (isdir)
			free(str2);
		i++;
	}
}
