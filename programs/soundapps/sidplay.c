#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wgsipc.h>
#include <string.h>
#include <sys/types.h>

unsigned char header[0x7c];

void main(int argc, char *argv[]) {
	char *bank = (char *)(((uint32) balloc(0xfff0))&0xff0000);
	FILE *fp;
	uint load;
	uint i;
	
	i=1;
	setup();
	while (i < argc)
	{
		fp = fopen(argv[i], "r");
		if (fp) {
			fread(header, 1, 0x7c, fp);
			printf("%s\n", header+0x16);
			printf("%s\n", header+0x36);
			printf("%s\n", header+0x56);
			load = fgetc(fp) + fgetc(fp)*256;
			fread(bank+load, 1, 0x3000, fp);
			playsid(bank, header);
			getchar();
			resetsid();
			fclose(fp);
		}
		i++;
	}
}
