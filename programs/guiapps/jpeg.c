#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wgsipc.h>
#include <winlib.h>
#include <string.h>
#include <sys/types.h>

int filefd;
uchar *segbuf = NULL;
uint segsize = 0;
uint seglen;
uint segin;

int check;
int chan;

extern int round8(int);

extern char natord[64];

typedef struct {
	uint maxc;
	uchar nrbits[16];
	uchar codeoff[16];
	uint max[16];
	uint min[16];
	uchar codes[256];
} HuffTable;

HuffTable htables[8];
uchar quantable[3][64];

HuffTable *hpoint[3][2];
uchar *qpoint[3];
int *buff[3];
uchar *coltab;
uchar *colloc;
uchar *bmploc,*bmpup;
int dc[3];
int ncomp;
int sampx[3];
int sampy[3];
int repx[3];
int repy[3];
int addx[3];
int addy[3];
int heightDU;
int widthDU;
int colsDU;
int cols8;
int maxy;
uint height,width,bmpwidth,bmpheight,width8;
int linesize;
uint restinv;
uint ducount;

uchar diff7[512];
uchar diff5[512];
uchar diff3[512];
uchar diff1[512];
uint multab[512];
uchar gbc[256];

uint gamma=128, contrast=128, bright=128;

int xup;
int yup;

/*
MenuData mainmenu[] = {
  {"exit", 0, NULL, 0, 5, NULL, NULL},
  {NULL,   0, NULL, 0, 0, NULL, NULL}
};

void handlemenu(void *Self, MenuData *item) {
  switch(item->command) {
    case 5: 
      exit(1);
    break;
  }
}

void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs) {
  void *temp;
  temp = JMnuInit(NULL, mainmenu, XAbs, YAbs, handlemenu);
  JWinShow(temp);
}
*/
void maketab() {
	uint i;
	int k;
	int32 j;
	
	for (i=0;i<512;i++) {
		j = i;
		multab[i] = j*j/4;
	}
	for (i=0;i<512;i++) {
		diff7[i] = i*7/16;
		diff5[i] = i*5/16;
		diff3[i] = i*3/16;
		diff1[i] = i/16;
	}
	for (i=0;i<256;i++) {
		k = contrast*i/128 + (bright-128) + (gamma-128)/i;
		if (k<0)
			k=0;
		else if (k>255)
			k=255;
		gbc[i] = k;
	}
}

void abort(char *reason) {
	fprintf(stderr, "Aborted because %s!\n", reason);
	getchar();
	exit(1);
}

uint getword() {
	return getbyte() * 0x100 + getbyte();
}

int fromseg() {
	seglen--;
	return segbuf[segin++];
}

int wordseg() {
	return fromseg() * 0x100 + fromseg();
}

int loadseg() {
	int ch;
	char *upto;
	
	ch = getbyte();
	if (ch != 0xff)
		abort("Badone");
	ch = getbyte();
	seglen=0;
	segin=0;
	if (ch>=0xd0 && ch<=0xd9)
		return ch;
	seglen = getword()-2;
	if (seglen>segsize) {
		segsize = seglen;
		segbuf = realloc(segbuf, seglen);
	}
	while (segin < seglen) {
		segbuf[segin] = getbyte();
		segin++;
	}
	segin = 0;
	return ch; 
}

/*
000
001
010
011
100
101

1100
1101

11100
11101
11110

111110

*/

void segDHT() {
	int ch,t,i,sum;
	HuffTable *tab;
	uint min,nrbits,bitoff;
	
	while (seglen) {
		ch = fromseg();
		t = ch & 0x0f;
		if (t>=4) 
			abort("Invalid DHT!\n");
		if (ch & 16)
			t = t + 4;
		tab = &htables[t];
		sum = 0;
		nrbits = 1;
		bitoff = 0;
		min = 0;
		for (i=0;i<16;i++) {
			ch = fromseg();
/*			printf("Table[%d]=%d\n", i, ch); */
			if (ch) {
				tab->min[bitoff] = min;
				min += ch;
				tab->max[bitoff] = min;
				tab->nrbits[bitoff] = nrbits;
				tab->codeoff[bitoff] = sum;
/*				printf("nrbits %u, max %u, codeoff %u\n", nrbits, min, sum); */
				nrbits = 0;
				bitoff++;
			}
			nrbits++;
			min <<= 1;
			sum += ch;
		}
		tab->nrbits[bitoff] = 0;
		tab->maxc = sum;
		for (i=0;i<sum;i++) {
			tab->codes[i] = fromseg();
		}
	}
}

void segDQT() {
	int ch,t;
	
	while (seglen) {
		ch = fromseg();
		t = ch & 0x0f;
		if (t>=3) 
			abort("Invalid DQT!\n");
		if (ch & 0xf0)
			abort("DQT can only be 8bit!\n");
		for(ch=0;ch<64;ch++)
			quantable[t][natord[ch]] = fromseg();
	}
}

void segSOS() {
	int i,ch,com,temp;
	
	ch = fromseg();
	for (i=0;i<ch;i++) {
		com = fromseg();
		temp = fromseg();
		com--;
		hpoint[com][0] = &htables[temp>>4];
		hpoint[com][1] = &htables[(temp&0x0f)+4];
	}
}

void segDRI() {
	ducount = restinv = wordseg();
}

void segSOF() {
	int i,ch,com,temp,maxx,temp2;
	uint32 dusize;
	
	maxx=maxy=0;
	ch = fromseg();
	height = wordseg();
	width = wordseg();
	ch = fromseg();
	ncomp = ch;
	for (i=0;i<ch;i++) {
		com = fromseg();
		com--;
		temp = fromseg();
		temp2 = temp>>4;
		if (temp2 > maxx)
			maxx = temp2;
		sampx[com] = temp2;
		temp &= 0x0f;
		if (temp > maxy)
			maxy = temp;
		sampy[com] = temp;
		temp = fromseg();
		qpoint[com] = &quantable[temp][0];
	}
	widthDU = maxx*8;
	heightDU = maxy*8;
	colsDU = (width-1)/widthDU + 1;
	cols8 = maxx*colsDU;
	linesize = (widthDU * colsDU + 2) * 2;
	dusize = (uint32) (heightDU + 1) * linesize;
	coltab = malloc(maxy * cols8);
	for (i=0;i<ch;i++) {
		buff[i] = malloc(dusize);
		repx[i] = maxx/sampx[i];
		repy[i] = maxy/sampy[i];
		addx[i] = repx[i] * 8 * 2;
		addy[i] = repy[i] * 8 * linesize;
	}
	printf("Width %d, Height %d\n", width, height);
	printf("Display unit is %dx%d\n", widthDU, heightDU);
	printf("Size of display unit %ld\n", dusize);
}

void render() {
	int j,y;
	int *upto=&buff[0][1];
	char *colup = coltab;

	dither();
	y=yup;
	for (j=0;j<heightDU;j++) {
		if (y>=height)
			break;
		if (!(y&7)) {
			memcpy(colloc, colup, width8);
			colloc += width8;
			colup += cols8;
		}
		fillrow(upto, bmpup);
		upto = (int *) ((uchar *) upto + linesize);
		y++;
		bmpup++;
		if (!(y&7)) {
			bmpup += bmpwidth-8;
		} 
	} 
	
}

int main(int argc, char *argv[]) {
	int RcvId;
	void *msg;
	void *App, *window, *bmp, *scr, *view;
	int seg=0,done=0,ch;
	uint32 temp,len;
	
	while ((ch = getopt(argc, argv, "g:b:c:")) != EOF) {
		switch(ch) {
		case 'g': 
			gamma = atoi(optarg);
			break;
		case 'b':
			bright = atoi(optarg);
			break;
		case 'c':
			contrast = atoi(optarg);
			break;
		}
			
	}
	if (argc-optind < 1)
		exit(1);
	filefd = open(argv[optind], O_READ);
	if (filefd == -1) {
		perror(argv[optind]);
		exit(1);
	}
	maketab();
	chan = makeChan();
	seg = loadseg();
	if (seg == 0xd8) {
		printf("It's a jpeg!\n");
	} else abort("Not a jpeg!\n");
	while(seg != 0xda) {
		seg = loadseg();
/*		printf("It's %2x, size %4x\n", seg, seglen); */
		switch(seg) {
			case 0xdb:
				segDQT();
				break;
			case 0xc4:
				segDHT();
				break;
			case 0xda:
				segSOS();
				break;
			case 0xc0:
				segSOF();
				break;
			case 0xdd:
				segDRI();
				break;
			default:
				printf("Unknown segment %d\n", seg);
				break;
		}
	}
	bmpwidth = round8(width);
	bmpheight = round8(height);
	width8 = bmpwidth/8;
	temp = (uint32) width8 * bmpheight;
	len = temp/8+temp;
	bmpup = bmploc = calloc(len, 1);
	colloc = bmploc+temp;
	printf("%dx%d, %lx bytes, %lx,%lx,%lx\n", bmpwidth, bmpheight, len, bmpup, colloc, bmpup+len);

	App = JAppInit(NULL, chan);
	window = JWndInit(NULL, "Jpeg viewer by J. Maginnis and S. Judd", JWndF_Resizable);
	bmp = JBmpInit(NULL, bmpwidth, bmpheight, bmploc);
	view = JViewWinInit(NULL, bmp);
	scr = JScrInit(NULL, view, 0);
//        JWinCallback(window, JWnd, RightClick, RightBut);
	JCntAdd(window, scr);
	JWinShow(window);

        retexit(1);

	while(1) {
		while (!done && !chkRecv(chan)) {
			imagedata();
			render();
			JWReDraw(bmp);
			yup += heightDU;
			if (yup >= height) {
				FILE *fp;
				done=1;
				printf("Finished render\n");
/*				fp = fopen("bmpout","wb");
				if (fp) {
					fwrite(bmploc, 320*25+1000, 1, fp);
					fclose(fp);
				} */
			}
		}
		RcvId = recvMsg(chan, &msg);
		switch (* (int *)msg) {
		case WIN_EventRecv:
			JAppDrain(App);
			break;			
		}
		replyMsg(RcvId,0);
	}

}
