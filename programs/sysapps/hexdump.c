#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

unsigned char linebuf[8];

int main (int argc, char *argv[])
{	
	FILE *stream;
	long upto=0;
	int i,ch,j;
	
	while ((ch = getopt(argc, argv, "s:")) != EOF) {
		if (ch == 's')
			upto = strtol(optarg, NULL, 0);
	}
	if (optind<argc) {
		stream = fopen(argv[optind],"rb");
		if (stream && upto)
			if (fseek(stream, upto, SEEK_SET)) {
				upto = 0;
			}
	} else {
		upto = 0;
		stream = stdin;
	}
	
	ch = 0;
	if (stream) {
		while (ch != EOF) {
			printf("%06lx ",upto);
			j=0;
			for (i=0;i<8;i++) {
				ch = fgetc(stream);
				if (ch == EOF) {
					printf("   ");
				} else {
					linebuf[i] = ch;
					printf("%02x ",ch);
					j=i+1;
				}
			}
			for (i=0;i<j;i++) {
				if (isprint(linebuf[i]))
					putchar(linebuf[i]);
				else putchar('.');
			}
			printf("\n");
			upto+=8;
		}
	} else {
		perror("hexdump");
		exit(1);
	}
}
