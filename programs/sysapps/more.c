#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termio.h>
#include <sys/stat.h>

int rows=24;
int cols=40;

int doinp() {
	int ch='q';
	int i;
	
	read(STDOUT_FILENO, &ch, 1);
	putchar('\r');
	for (i=0;i<cols;i++) 
		putchar(' ');
	putchar('\r');
	switch(ch) {
		case 'q':
			exit(1);
		case '\n':
			return rows-1;
		default:
			return 0;
	}
}

int main(int argc, char *argv[]) {
	int chcount=0;
	int rowcount=0;
	int itskey=0;
	int ch,i,piped=0;
	FILE *input;
	struct termios tio;
	struct stat buf;
	long done;
	char *str,*cur;
	
	if (isatty(STDOUT_FILENO)) {
		itskey = 1;
		gettio(STDOUT_FILENO, &tio);
		rows = tio.rows-1;
		cols = tio.cols;
		tio.flags &= ~(TF_ICANON|TF_ECHO|TF_ECHONL);
		tio.MIN = 1;
		settio(STDOUT_FILENO, &tio);
	}
	if (argc == 1) {
		if (isatty(STDIN_FILENO)) {
			fprintf(stderr, "Usage: more [files]\n");
			exit(1);
		}
		argc = 2;
		piped = 1;
	}
	i=1;
	while (i<argc) {
		cur = argv[i];
		i++;
		if (piped) {
			input = stdin;
		} else {
			input = fopen(cur,"rb");
			if (input) 
				fstat(fileno(input), &buf);
		}
		if (!input) 
			perror(cur);
		else {
			rowcount=0;
			if (argc != 2) {
				printf(":::::::::\n");
				str = strrchr(cur, '/');
				if (!str)
					str = cur;
				else str++;
				printf("%s\n:::::::::\n",str);
				rowcount=3;
			}
			done = 0;
			while((ch = fgetc(input)) != EOF) {
				switch(ch) {
				default:
					chcount++;
					if (chcount<=cols)
						break;
				case '\n':
					rowcount++;
				case '\r':
					chcount=0;
					break;
				}
				putchar(ch);
				done++;
				if (itskey) {
					if (rowcount>=rows) {
						printf("-- More --");
						if (!piped) {
							printf("(%d%%)", (int) (done*100L / buf.st_size));
						}
						fflush(stdout);
						rowcount = doinp();
					}
				}
			}
			if (!piped)
				fclose(input);
			if (i < argc && itskey) {
				printf("\n-- More -- (Nextfile: %s)", argv[i]);
				fflush(stdout);
				rowcount = doinp();
			}
		}
	}

}
