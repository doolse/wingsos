#include <stdio.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <termio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 300

struct shexp_t {
	char *buf;
	char ***coms;
	char **vwords;
	int wsize;
	int csize;
	int numcoms;
	int term;
	int bufsize;
};

struct termios *tptr;
int inter;
extern void shellexp(struct shexp_t **shx, char **com, int fds[2]);
extern void expfree(struct shexp_t *shx);
extern char *getLine(char *str, int strsize, FILE *fp, int cols);

int prinres(int err) {
	if (err == -1) 
		perror("sh");
	return 1;
}

void exsh(int ex) {
	settio(STDIN_FILENO,tptr);
	exit(ex);
}

int chkBuiltin(char *coms[]) {
	int fd;
	int nargs=0;
	
	char **comup = coms;
	while (*comup != NULL)
	{
		nargs++;
		comup++;
	}
	if (!strcmp(coms[0],"setcon")) {
		fflush(stdin);
		fflush(stdout);
		fflush(stderr);
		fd = open(coms[1],O_READ|O_WRITE);
		if (fd != -1) {
			close(fd);
			close(0);
			close(1);
			close(2);
			open(coms[1],O_READ|O_WRITE);
			dup(0);
			dup(0);
		} else {
			fprintf(stderr, "No such console!\n");
		}
		return 1;
	}
	if (!strcmp(coms[0],"cd")) {
		return prinres(chdir(coms[1]));
	}
	if (!strcmp(coms[0],"rmdir")) {
		return prinres(rmdir(coms[1]));
	}
	if (!strcmp(coms[0],"mount")) {
		return prinres(mount(coms[1],coms[2],coms[3]));
	}
	if (!strcmp(coms[0],"umount")) {
		return prinres(umount(coms[1]));
	}
	if (!strcmp(coms[0],"mkdir")) {
		return prinres(mkdir(coms[1]));
	}
	if (!strcmp(coms[0],"sync")) {
		return prinres(syncfs(coms[1]));
	}
	if (!strcmp(coms[0],"exit")) {
		if (inter)
			printf("Exiting Shell...\n");
		exsh(0);
	}
	if (!strcmp(coms[0],"sex")) {
		printf("An is sexy\n");
		return 1;
	}
	if (!strcmp(coms[0],"setenv")) {
		if (nargs != 3)
		{
			printf("Usage: setenv VAR VALUE\nE.g. setenv PATH /wings/programs:.:/\n");
			return 1;
		}
		else
		{
			setenv(coms[1], coms[2], 1);
			return 1;
		}	
	}
	if (!strcmp(coms[0],"getenv")) {
		if (nargs != 2)
		{
			printf("Usage: getenv VAR\nE.g. getenv PATH\n");
			return 1;
		} else {
			printf("%s=%s\n", coms[1], getenv(coms[1]));
			return 1;
		}
	}        
	if (!strcmp(coms[0],"reset")) {
		int fd;
		fd = open("/sys/blk.iec",O_READ);
		sendCon(fd,PMSG_ShutDown);
	}        
	return 0;
}

void main(int argc, char *argv[]) {
	int pid,pid2;
	char *comline;
	char *comup;
	int fds[2];
	int stdfds[3];
	unsigned int i,havecom=0,doex=0;
	struct termios tio,tio2;
	int flags,temp;
	int debug;
	FILE *infile;
	struct shexp_t *shexp=NULL;

	inter = isatty(STDIN_FILENO);
	tptr = &tio;
	while ((temp = getopt(argc, argv, "t:c:")) != EOF) {
		switch(temp) {
			case 't':
				close(0);
				close(1);
				close(2);
				temp = open(optarg,O_READ|O_WRITE);
				if (temp == -1) {
					perror("sh");
					exit(1);
				}
				dup(0);
				dup(0);
				break;
			case 'c':
				havecom = 1;
				inter = 0;
				comup = optarg;
				break;
				
		}
	}
	if (!havecom && optind < argc) {
		inter = 0;
		infile = fopen(argv[optind],"rb");
		if (!infile) {
			perror("sh");
			exit(1);
		}
	} else infile = stdin;
	if (inter) {
		printf("Welcome to J-Shell V0.1\n" 
			"(c) Jolse Maginnis\n");
	}
	gettio(STDIN_FILENO,&tio);
	comline = (char *) xmalloc(MAXLINE); 
	do {	
		*comline='\0';
		memcpy(&tio2, &tio, sizeof(tio));
		if (inter) {	
			tio2.flags &= ~(TF_ICANON|TF_ECHO|TF_ISIG);
		   	tio2.flags |= TF_ICRLF;
			tio2.MIN = 1;
			settio(STDIN_FILENO,&tio2);
			sprintf(comline,"<%s>%%: ",wgswd());
//			printf("\x1b[r");
//			fflush(stdout);
			comup = getLine(comline,MAXLINE,infile,tio.cols);
		} else {
			if (!havecom) {
				if (!fgets(comline,MAXLINE,infile)) {
					exit(1);
				}
			   	comup=comline;
			}
		}
		tio2.flags |= (TF_ICANON|TF_ECHO|TF_ISIG);
		settio(STDIN_FILENO,&tio2);		
		do {
			shellexp(&shexp, &comup, stdfds);
			i=0;
			fds[0] = -1;
			while (i < shexp->numcoms) {
				resetredir();
				fds[1] = -1;
				temp = fds[0];
				if (chkBuiltin(shexp->coms[i])) {
						pid = 0;
						gettio(STDOUT_FILENO,&tio2);
						break;
				}
				if (!i) {
					flags = S_LEADER;
					redir(stdfds[STDIN_FILENO],STDIN_FILENO);
				} else {
					flags = 0;
					redir(temp,STDIN_FILENO);
				}
				if (i == shexp->numcoms-1) {
					redir(stdfds[STDOUT_FILENO],STDOUT_FILENO);
					redir(stdfds[STDERR_FILENO],STDERR_FILENO);
				} else {
					pipe(fds);
					redir(fds[1],STDOUT_FILENO);
				}
				if (!strcmp(shexp->coms[i][0], "exec")) {
					shexp->coms[i]++;
					doex=1;
				}
				pid = spawnvp(flags, shexp->coms[i]);
				if (doex)
					exit(0);
				if (!i)
					pid2 = pid;
				close(fds[1]);
				close(temp);
				if (pid == -1)
					perror("sh");
				i++;
			}
			close(stdfds[STDIN_FILENO]);
			close(stdfds[STDOUT_FILENO]);
			close(stdfds[STDERR_FILENO]);
			if (pid && i && shexp->term != '&') {
				setfg(STDIN_FILENO,pid2);
				waitPID(pid);
			}
			expfree(shexp);
		} while (shexp->term != '#');
	}
	while (!havecom);
	exsh(0);
}

