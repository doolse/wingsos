#include <stdio.h>
#include <stdlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>
#include <termio.h>

long bauds[12] = {0, 110, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400};

int globsock;

void fromServer(int *tlc) {
	FILE *stream;
	int ch;
	
	stream = fdopen(*tlc,"w");
	
	stdin->_flags |= _IOBUFEMP;
	if (!stream)
		exit(1);
	while (!feof(stdin)) {
		while ((ch = getchar()) != EOF)
			fputc(ch,stream);
		fflush(stream);
	}
	exit(1);
}

void main(int argc, char *argv[]) {
	int ch;
	FILE *stream;
	struct termios T1;
	unsigned int baud=0;
	long brate,min=1000000,dif;

	while ((ch = getopt(argc, argv, "b:")) != EOF) {
		switch(ch) {
		case 'b': 
			brate = strtol(optarg, NULL, 0);
			ch = 0;
			while (ch<11) {
				dif = bauds[ch] - brate;
				if (dif<0)
					dif = -dif;
				if (dif<min) {
					baud = ch;
					min = dif;
				}
				ch++;
			}
			break;
		}
			
	}
	if (argc-optind<1) {
		fprintf(stderr, "Usage: term [-b baudrate] device\nE.g. term /dev/modem\n");
		exit(1);
	}
		
	stream = fopen(argv[optind],"rb");
	if (!stream) {
		perror("term");
		exit(1);
	}
	gettio(fileno(stream),&T1);
	T1.flags = TF_OPOST|TF_IGNCR;
	if (baud)
		T1.Speed = baud;
	settio(fileno(stream),&T1);
	gettio(STDIN_FILENO,&T1);
	T1.flags &= ~(TF_ICANON|TF_ECHO|TF_ICRLF);
	settio(STDIN_FILENO,&T1);
	globsock = fileno(stream);
	newThread(fromServer,256,&globsock);
	stream->_flags |= _IOBUFEMP;
	while(!feof(stream)) {
		while ((ch = fgetc(stream)) != EOF)
			putchar(ch);
		fflush(stdout);
	}
	
}

