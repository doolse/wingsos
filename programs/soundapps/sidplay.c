#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wgsipc.h>
#include <string.h>
#include <termio.h>
#include <sys/types.h>

unsigned char header[0x7c];

void main(int argc, char *argv[]) {
	char *bank = (char *)(((uint32) balloc(0xfff0))&0xff0000);
	FILE *fp;
	uint load;
	uint i;
        int option,j,songs;
	struct termios tio;
	
	gettio(STDIN_FILENO, &tio);
	tio.flags &= ~TF_ICANON;
	tio.MIN = 1;
	settio(STDIN_FILENO, &tio);
	i=1;
	setup();
	printf("Next song (n), Prev song (p), Quit (q)\n");
	while (i < argc)
	{
		fp = fopen(argv[i], "r");
		if (fp) {
			fread(header, 1, 0x7c, fp);
                        printf("Playing song#%d in the list\n", i);
			printf("%s\n", header+0x16);
			printf("%s\n", header+0x36);
			printf("%s\n", header+0x56);
			load = fgetc(fp) + fgetc(fp)*256;
			fread(bank+load, 1, 0xff00, fp);
			fclose(fp);
			songs = header[0x0f];
			j=0;
			while (1)
			{
				playsid(bank, header, j);
				option = getchar();
				if(option == 'n')
				  j++;
                		else if(option == 'p')
                		  j--;
				else if(option == 'q')
				{
                		  i = argc;
				  break;
			  	}
				if (j>=songs)
				{
				  i++;
				  break;
			  	}
				if (j<0)
				{
				  i--;
				  break;
			  	}
				
			}
		}
		resetsid();
		if (i < 1)
		  i=1;
	}
}
