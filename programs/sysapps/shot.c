#include <stdio.h>

#define charset 0x4800
#define screens	0x5000
#define vicback	0xd021

int vicscr=0x4000;
int viccols=0x6000;
int dimensions[]={0,0,319,199};
int extras[]={320,1,320,200};

/* Pallette thanks to MagerValp .. I changed a few 
 colours to look more like what I see on my screen*/

unsigned char pallette[]=
{   0,  0,  0,  
    0xfd,0xfe,0xfc, 
    0xbe,0x1a,0x24, 
    0x30,0xE6,0xC6,
    0xB4,0x1A,0xE2,
    0x1F,0xD2,0x1E,
    0x21,0x1B,0xAE,
    0xDF,0xF6,0x0A,
    0xB8,0x41,0x04,
    0x6A,0x33,0x04,
    0xFE,0x4A,0x57,
    83,83,83,
    155,155,155,
    0x59,0xFE,0x59,
    153,161,255,
    205,205,205
};
int runlen=0;
unsigned int last=1000;

void enclen(int this, FILE *fp) {
	while(runlen>=63) {
		fputc(192+63,fp);
		fputc(last,fp);
		runlen-=63;
	}
	if (runlen>=2)
		fputc(192+runlen,fp);
	if (runlen>=1)
		fputc(last,fp);
	runlen=0;
	last=this;
};

void xypixel(int x, int y, int screen, FILE *fp) {
	int yoff,ydiv;
	unsigned int ch,byte;
	unsigned int this;
	int col1,col2;
	unsigned char *scr;
	
	yoff = y % 8;
	ydiv = y / 8;
	if (screen == 0) {
		ch = * (unsigned char *)(viccols+(40*ydiv)+x);
		col1 = ch & 0x0f;
		col2 = (ch & 0xf0) >> 4;
		byte = * (unsigned char *)(vicscr+(320*ydiv)+(x*8)+yoff);
	} else {
		col1 = *(char *)vicback & 0x0f;
		scr = (unsigned char *)(screens+(2000*(screen-1))+(40*ydiv)+x);
		col2 = *(scr+1000) & 0x0f;
		ch = *scr;
		byte = * (unsigned char *)(charset+(8*ch)+yoff);
	}
	for (yoff=0;yoff<8;yoff++) {
		if ((byte & (128>>yoff)))
			this = col2;
		else 
			this = col1;
		if (this!=last)
			enclen(this, fp);
		runlen++;
	}
		
}

int main(int argc, char *argv[]) {
	FILE *fp;
	int screen;
	char *filename="outfile.pcx";
	int x,y;
	
	if (argc<2) {
		fprintf(stderr,"Usage: shot screen [file]\n");
		exit(1);
	}
	
	screen = atoi(argv[1]);
	if (argc>2)
		filename = argv[2];
	fp = fopen(filename,"wb");
	if (fp) {
		if (screen==0) {
			vicscr = 0x4000 + (* (unsigned char *)0xd018 & 0x08) * 0x400;
			viccols = 0x4000 + ((* (unsigned char *)0xd018 & 0xf0) >> 4) * 0x400;
		}
		
		fputc(10,fp);		/* zsoft */
		fputc(0,fp);		/* version */
		fputc(1,fp);		/* encoding */
		fputc(8,fp);		/* bits per pixel */
		fwrite(&dimensions,4,sizeof(int),fp);
		for (x=0;x<53;x++)
			fputc(0,fp);	/* clear it */
		fputc(1,fp);		/* one colour plane */
		fwrite(&extras,4,sizeof(int),fp);
		for (x=0;x<54;x++)
			fputc(0,fp);	/* clear it */		
		for (y=0;y<200;y++) {
			for (x=0;x<40;x++)
				xypixel(x,y,screen,fp);
			enclen(1000,fp);
		}
		fputc(12,fp);
		fwrite(&pallette,3,16,fp);
		for (x=0;x<3*(256-16);x++)
			fputc(0,fp);
	}
}
