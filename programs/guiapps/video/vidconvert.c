#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wgsipc.h>
#include <winlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int filefd;
FILE *outfp;
uchar *segbuf = NULL;
uint segsize = 0;
uint seglen;
uint segin;

//int check;
//int chan;

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
uint height,width,bmpwidth,bmpheight,width8,imgsize;
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

void abort (char * reason) {
  printf("Aborting: %s\n", reason);
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
    return(ch);

  ch = getbyte();
  seglen=0;
  segin=0;

  if (ch>=0xd0 && ch<=0xd9)
    return(ch);

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
  return(ch); 
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

int segDHT() {
  int ch,t,i,sum;
  HuffTable *tab;
  uint min,nrbits,bitoff;
	
  while (seglen) {
    ch = fromseg();
    t = ch & 0x0f;

    if (t>=4)   
      return(0);
    if (ch & 16)
      t = t + 4;

    tab = &htables[t];
    sum = 0;
    nrbits = 1;
    bitoff = 0;
    min = 0;
    for (i=0;i<16;i++) {
      ch = fromseg();
      //printf("Table[%d]=%d\n", i, ch); 
      if (ch) {
        tab->min[bitoff] = min;
        min += ch;
        tab->max[bitoff] = min;
        tab->nrbits[bitoff] = nrbits;
        tab->codeoff[bitoff] = sum;
        //printf("nrbits %u, max %u, codeoff %u\n", nrbits, min, sum);
        nrbits = 0;
        bitoff++;
      }

      nrbits++;
      min <<= 1;
      sum += ch;
    }
    tab->nrbits[bitoff] = 0;
    tab->maxc = sum;

    for (i=0;i<sum;i++)
      tab->codes[i] = fromseg();

  }
  return(1);
}

int segDQT() {
  int ch,t;

  while (seglen) {
    ch = fromseg();
    t = ch & 0x0f;

    if (t>=3) 
      return(0);

    if (ch & 0xf0)
      return(0);

    for(ch=0;ch<64;ch++)
      quantable[t][natord[ch]] = fromseg();

  }
  return(1);
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

  widthDU  = maxx*8;
  heightDU = maxy*8;
  colsDU   = (width-1)/widthDU + 1;
  cols8    = maxx*colsDU;
  linesize = (widthDU * colsDU + 2) * 2;
  dusize   = (uint32) (heightDU + 1) * linesize;
  coltab   = malloc(maxy * cols8);

  for (i=0;i<ch;i++) {
    buff[i] = malloc(dusize);
    repx[i] = maxx/sampx[i];
    repy[i] = maxy/sampy[i];
    addx[i] = repx[i] * 8 * 2;
    addy[i] = repy[i] * 8 * linesize;
  }

  //printf("Width %d, Height %d\n", width, height);
  //printf("Display unit is %dx%d\n", widthDU, heightDU);
  //printf("Size of display unit %ld\n", dusize);
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

/* rle code modified from an example found on www.compuphase.com  */

void rle() {
  int startpos,index,i;
  unsigned char pixel;
  char* input = bmploc;
  unsigned int length = (unsigned int)imgsize;

  startpos=0;
  while (startpos<length) {

    index=startpos;

    pixel=input[index++];

    while (index<length && index-startpos < 127 && (unsigned char)(input[index]) == pixel) {
      index++;
    }

    if (index-startpos==1) {

      /* Failed to "replicate" the current pixel. See how many to copy.
       * Avoid a replicate run of only 2-pixels after a literal run. There
       * is no gain in this, and there is a risc of loss if the run after
       * the two identical pixels is another literal run. So search for
       * 3 identical pixels.
       */

      while (index < length && 
             index-startpos < 127 && 
             (input[index]!=input[index-1] || index>1 && input[index]!=input[index-2]))
        index++;

      /* Check why this run stopped. If it found two identical pixels, reset
       * the index so we can add a run. Do this twice: the previous run
       * tried to detect a replicate run of at least 3 pixels. So we may be
       * able to back up two pixels if such a replicate run was found.
       */

      while (index < length && input[index]==input[index-1])
        index--;

      fputc((unsigned char)(startpos-index),outfp);
 
      for (i=startpos; i<index; i++)
        fputc(input[i],outfp);
    } else {
      fputc((unsigned char)(index-startpos), outfp);
      fputc(pixel,outfp);
    }
    startpos=index;
  } 
}

int main(int argc, char *argv[]) {
  int j, seg=0, done=0, ch, frames, i, startframe = 1, endframe = 0;
  uint32 temp,len;
  char *filename, *prefilename;

  if(argc < 5) {
    printf("vidconvert USAGE %s [-g gamma][-b brightness][-c contrast] -f frames                         -n prefilename [-s startframe][-e endframe]\n",argv[0]);
    exit(1);
  }

  while ((ch = getopt(argc, argv, "g:b:c:f:n:s:e:")) != EOF) {
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
      case 's':
        startframe = atoi(optarg);
      break;
      case 'e':
        endframe = atoi(optarg);
      break;
      case 'f':
        frames = atoi(optarg);
      break;
      case 'n':
        prefilename = strdup(optarg);
      break;
      default:
        printf("vidconvert USAGE %s [-g gamma][-b brightness][-c contrast] -f frames                         -n prefilename [-s startframe][-e endframe]\n",argv[0]);
    }
  }

  if(!endframe)
    endframe = frames;

  if(strlen(prefilename)>11) {
    printf("invalid prefilename length.\n");
    exit(1); 
  }

  filename = malloc(17);

  sprintf(filename, "%s.rvd", prefilename);
  
  outfp = fopen(filename,"a");

  if(!outfp) {
    printf("couldn't open output stream.\n");
    exit(1);
  }

  maketab(); //generated from gamma contrast and brightness

  for(i = startframe; i<=frames; i++) {

    printf("Opening Frame %d\n", i);

    if(!endframe)
      break;
    endframe--;

    //Reset All variables... 

    segbuf = NULL;
    seg = segsize = seglen = segin = yup = xup = ncomp = 0;
    heightDU = widthDU = colsDU = cols8 = maxy = linesize = restinv = ducount = 0;
    buff[0] = buff[1] = buff[2] = NULL;
    coltab = colloc = bmploc = bmpup = NULL;
    dc[0] = dc[1] = dc[2] = 0;

    reset();

    //take into account for the zeros with greater frame totals
    if(frames < 100) {
      if(i<10)
        sprintf(filename, "%s 0%d.jpg", prefilename, i);
      else
        sprintf(filename, "%s %d.jpg", prefilename,i);
    } else {  
      if(i<10)
        sprintf(filename, "%s 00%d.jpg", prefilename, i);
      else if(i<100)
        sprintf(filename, "%s 0%d.jpg", prefilename, i);
      else
        sprintf(filename, "%s %d.jpg", prefilename,i);
    }

    filefd = open(filename, O_READ);
    if (filefd == -1) {
      printf("Unable to open file %s... re-using previous frame.\n", filename);
      j=1;
      while(i-j > 0) {
        if(frames < 500) {
          if(i<10)
            sprintf(filename, "%s 0%d.jpg", prefilename, i-j);
          else
            sprintf(filename, "%s %d.jpg", prefilename,i-j);
        } else {  
          if(i<10)
            sprintf(filename, "%s 00%d.jpg", prefilename, i-j);
          else if(i<100)
            sprintf(filename, "%s 0%d.jpg", prefilename, i-j);
          else
            sprintf(filename, "%s %d.jpg", prefilename,i-j);
        }
        filefd = open(filename, O_READ);
        if(filefd == -1)
          j++;
        else
          break;        
      }
      if(i-j < 1)
        continue;
    }

    seg = loadseg();

    if (seg == 0xd8)
      printf("It's a jpeg!\n");
    else { 
      printf("Image file %s, is not a JPEG '%d'... skipping it.\n", filename, seg);
      exit(1);
    }

    while(seg != 0xda) {
      seg = loadseg();

      if(!seg)
        goto skiptonext;

      switch(seg) {
        case 0xdb:
          if(!segDQT())
            goto skiptonext;
        break;
        case 0xc4:
          if(!segDHT())
            goto skiptonext;
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
      }
    }

    bmpwidth  = round8(width);
    bmpheight = round8(height);
    imgsize   = ((bmpheight*bmpwidth)/8) + ((bmpwidth*bmpheight)/64);
    width8    = bmpwidth/8;
    temp      = (uint32) width8 * bmpheight;
    len       = temp/8+temp;
    bmpup     = bmploc = calloc(len, 1);
    colloc    = bmploc+temp;

    while (!done) {
      imagedata();
      render();
      yup += heightDU;
      if (yup >= height) {
        done=1;
	printf("Finished rendering frame %d\n", i);
        rle();
        printf("Finished RLE encoding frame %d\n", i);
      } 
    }
    done = 0;
    skiptonext:
    close(filefd);
  }
  fclose(outfp);
  return(1);
}
