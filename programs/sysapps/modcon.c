#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <termio.h>

char buf[0x800];

void docol(int ioctl) {
	sendCon(0, IO_CONTROL, ioctl, atoi(optarg));
}

int main (int argc, char *argv[])
{	
	FILE *fp;
	int ch;
	
	if (argc == 1) {
		printf(
		"Usage: modcon [-n fontfile]\n"
		"[-f fgcol] [-b bgcol]\n"
		"[-r bordercol] [-c cursorcol]\n"
		"stdin is the console to be modified\n"
		"all colours are c64 colour numbers (0-15)\n"
		); 
		exit(1);
	}
	while ((ch = getopt(argc, argv, "n:f:b:r:c:")) != EOF) {
		switch (ch) {
		case 'n':
			fp = fopen(optarg, "r");
			if (!fp) {
				perror(optarg);
				break;
			}
			fgetc(fp);
			fgetc(fp);
			if (fread(buf, 0x800, 1, fp) != 1) {
				fprintf(stderr, "\"%s\" Doesn't appear to be a console font\n", optarg);
				break;
			}
			sendCon(0, IO_CONTROL, IOCTL_Font, FONTF_8x8Char, buf);
			break;
		case 'b':
			docol(IOCTL_ChBG);
			break;
		case 'f':
			docol(IOCTL_ChFG);
			break;
		case 'r':
			docol(IOCTL_ChBord);
			break;
		case 'c':
			docol(IOCTL_ChCurs);
			break;
		}
	}	
}
