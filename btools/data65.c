#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include "getopt.h"
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include "asm.h"

uchar o65head[] = {2, 8, 'J', 'o', 's', 0};

char *outname = "data.o65";

uint32 addr = 0x4000;
int segflags = S_NOCROSS|S_DBR;
int c64exec;
FILE *outfp;

int f16(uint16 val) {
	fputc(val&0xff, outfp);
	fputc(val>>8, outfp);
}

int f32(uint32 val) {
	fputc(val&0xff, outfp);
	fputc(val>>8, outfp);
	fputc(val>>16, outfp);
	fputc(val>>24, outfp);
}

void output(uchar *seg, uint32 len) {
	FILE *fp;
	
	fp = fopen(outname, "w");
	if (fp) {
		
		outfp = fp;
		/* Jos Magic */
		fwrite(o65head, 1, sizeof(o65head), fp);
		
		/* Flags */
		f16(0);	
		
		/* Version */
		f16(0x0100);	
		
		/* Stacksize */
		f16(0);
				
		/* Write segments */

		fputc(SEGMENTS, fp);
		f32(16+len);
		f16(1);
		f32(addr);
		f32(len);
		f32(0);
		f16(segflags);
		
		fwrite(seg, 1, len, fp);
		fclose(fp);
	} else perror(outname);
}

void showins()
{
	printf("data65 - Create linkable .o65 files out of data files\n"
	"Usage: data65 [-s SEG] [-o outname] [-a ADDRESS] files..\n"
	"-s SEG       - One of t,d or b for text,data or bss\n"
	"-o outname   - Output filename\n"
	"-a ADDRESS   - Start address\n"
	"Default output name is \"data.o65\"\n"
	"Default segment is data, address 0x4000\n");
	exit(1);
	
}

int main(int argc, char *argv[]) {
	int ch;
	FILE *fp = stdin;
	uint32 seglen=0;
	uint32 segalloc=0x4000;
	uchar *alloced=malloc(0x4000);
	uchar *upto=alloced;

	while ((ch = getopt(argc, argv, "o:s:a:hc")) != EOF) 
	{
		switch (ch)
		{
			case 'o':
				outname = optarg;
				break;
			case 'a':
				addr = strtol(optarg, NULL, 0);
				break;
			case 's':
				switch (optarg[0])
				{
					case 't':
						segflags = S_RO|S_NOCROSS;
						break;
					case 'd':
						segflags = S_NOCROSS|S_DBR;
						break;
					case 'b':
						segflags = S_NOCROSS|S_DBR|S_BLANK;
						break;
				}
				break;
			case 'c':
				c64exec = 1;
				break;
			default:
				showins();
		}
	}
	if (optind<argc) {
		fp = fopen(argv[optind], "r");
		if (!fp) {
			perror(argv[optind]);
			exit(1);
		}
	}
	if (c64exec) {
		addr = fgetc(fp) + (fgetc(fp)<<8);
	}
	while ((ch = fgetc(fp)) != EOF)
	{
		*upto = ch;
		upto++;
		seglen++;
		while (seglen >= segalloc)
		{
			segalloc <<= 1;
			alloced = realloc(alloced, segalloc);
			upto = alloced+seglen;
		}
	}
	output(alloced, seglen);
	return 0;
}
