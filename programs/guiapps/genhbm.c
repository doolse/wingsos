#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winlib.h>

#define OFF    0
#define ON     1
#define TOGGLE 2

typedef struct {
  char RiffIdent[4];
  long TotalSize;
  char RiffType[4];
} Riff;

typedef struct {
  char FormatIdent[4];
  long FormatSize;
  int PCMflag;
  int Channels;
  long SampleRate;
  long ByteSec;
  int ByteSamp;
  int BitSamp;
} Format;

typedef struct {
  char DataIdent[4];
  long DataSize;
} RData;

typedef struct wavbuf_s {
  unsigned long totalbytes;
  unsigned char * wavbuffer;
  void * zoominput;
  Riff   * riff;
  Format * format;
  RData  * rdata;
} wavbuf;

unsigned char * bitmap;
void * app, *image;

unsigned long COLS,ROWS;
int BW;

unsigned char bits[8] = {128,64,32,16,8,4,2,1};

wavbuf * loadwavfile();
void drawwavform(void * renderbutton);
void testdraw();

unsigned long * rowlookup; 

void bitset(unsigned long x, unsigned long y,int mode) {
  unsigned long row, character;
  unsigned int line, bit;

  row       = y/8;
  character = x&(65535-7);
  line      = y&7;
  bit       = x&7;
  
  if(mode == ON)
    bitmap[character + rowlookup[row] + line] |= bits[bit];
  else if(mode == OFF)
    bitmap[character + rowlookup[row] + line] &= ~(bits[bit]);
  else
    bitmap[character + rowlookup[row] + line] ^= bits[bit];
}

void writebitmaptodisk() {
  FILE * outfile;
  unsigned int i;
  //Write out all bitmap data to Disk.

  outfile = fopen("outfile.hbm", "w");

  for(i=0;i<2 + (COLS * ROWS * 8) + (COLS * ROWS);i++) 
    fputc(bitmap[i],outfile);

  fclose(outfile);
}

void main(int argc, char * argv[]) {
  void *wnd, *imagepane, *scroll, *topcontainer, *botcontainer;
  void *renderbutton, *leftleft, *leftright, *rightleft, *rightright;
  void *zoominput; 
  wavbuf * wavbuff;
  int asc,x;
  unsigned long i,y;
  FILE * outfile;

  app = JAppInit(NULL,0);
  wnd = JWndInit(NULL, "DigiWave v0.1",JWndF_Resizable);

  wavbuff = loadwavfile();

  JWSetBounds(wnd, 8,8, 280,160);
  JAppSetMain(app,wnd);
  
  ((JCnt*)wnd)->Orient = JCntF_Vert;

  topcontainer = JCntInit(NULL);
  botcontainer = JCntInit(NULL);

  zoominput = JTxfInit(NULL);
  renderbutton = JButInit(NULL, " Zoom ");

  JTxfSetText(zoominput, "1");

  wavbuff->zoominput = zoominput;
  JWSetData(renderbutton, wavbuff);
  JWSetData(zoominput, wavbuff);

  JWinCallback(renderbutton, JBut, Clicked, drawwavform);
  JWinCallback(zoominput, JTxf, Entered, drawwavform);

  JCntAdd(wnd, topcontainer);
  JCntAdd(topcontainer, zoominput);
  JCntAdd(topcontainer, renderbutton);

  //COLS = wavbuff->riff->TotalSize/8;
  COLS = 40;
  ROWS = 25;

  BW = 6; //blue on lt.grey.

  //generate lookup table for rows.
  rowlookup = (unsigned long *)malloc(sizeof(long) * ROWS);
  y = 0;
  i = COLS * 8;
  for(x = 0; x<ROWS; x++) {
    rowlookup[x] = y;
    y += i;
  }

  //DEBUG 
/* 
  printf("COLS = %d\n", COLS); 
  for(i = 10; i<ROWS; i++)
    printf("row %ld, byte offset %ld\n", i, rowlookup[i]);

  printf("Total memory allocated = %ld\n", 2+(COLS*ROWS*8)+(COLS*ROWS));
  exit(1);
*/

  //malloc for 2 byte header plus bitmap memory plus color memory
  bitmap = (unsigned char *)malloc(2+(COLS * ROWS * 8)+(COLS * ROWS));

  //initialize header
  *bitmap     = 0;
  *(bitmap+1) = 2; 

  //Clear all bitmap bits
  memset(bitmap+2, 0, COLS * ROWS * 8);

  //Set all Color bits
  memset(bitmap+2+(COLS*ROWS*8), BW, COLS * ROWS);

  image     = JBmpInit(NULL,COLS*8,ROWS*8,bitmap);
  imagepane = JViewWinInit(NULL, image);
  scroll    = JScrInit(NULL, imagepane, 0);

  JCntAdd(wnd, scroll);
  JWinShow(wnd);

  drawwavform(renderbutton);

  //writebitmaptodisk();

  retexit(1);
  JAppLoop(app);
}

wavbuf * WavBufferInit() {
  wavbuf * w;

  w = (wavbuf *)malloc(sizeof(wavbuf));
  w->riff = (Riff *)malloc(sizeof(Riff));
  w->format = (Format *)malloc(sizeof(Format));
  w->rdata = (RData *)malloc(sizeof(RData));
  
  return(w);
}

wavbuf * loadwavfile() {
  FILE * wavfile;
  wavbuf * wavbuff;

  wavfile = fopen("test.wav", "rb");
  wavbuff = WavBufferInit();

  fread(wavbuff->riff,   1, sizeof(Riff),   wavfile);
  fread(wavbuff->format, 1, sizeof(Format), wavfile);
  fread(wavbuff->rdata,  1, sizeof(RData),  wavfile);

  wavbuff->wavbuffer  = (unsigned char *)malloc(wavbuff->riff->TotalSize);
  wavbuff->totalbytes = fread(wavbuff->wavbuffer, 1,wavbuff->riff->TotalSize, wavfile);
  fclose(wavfile);

  printf("riff size = %ld, bytes read = %ld\n",wavbuff->riff->TotalSize, wavbuff->totalbytes);

  return(wavbuff);
}

void drawwavform(void * renderbutton) {
  unsigned long x,y1,y2;
  unsigned long counter;
  unsigned char * ptr;  
  int zoom;
  wavbuf * wavbuff;

  wavbuff = JWGetData(renderbutton);
  zoom = atoi(JTxfGetText(wavbuff->zoominput));

  //printf("Zoom is %d\n", zoom);
  if(zoom > 200)
    zoom = 200;
  else if(zoom < 1)
    zoom = 1;

  //Clear all bitmap bits
  memset(bitmap+2, 0, COLS * ROWS * 8);

  ptr = wavbuff->wavbuffer;
  y1 = *ptr;

  counter = 0;

  //for(x = 0; x < wavbuff->totalbytes; x++) {
  for(x = 0; x<COLS*8;x++) {
    ptr += zoom;
    counter += zoom;
    if(counter > wavbuff->totalbytes)
      break;

    y2 = *ptr;

    if(y1>y2) {
      while(y1 != y2) {
        //printf("drawing line %d\n", x);
        bitset(x,y1,ON);
        y1--;
      }      
    } else if(y2>y1){
      while(y2 != y1) {
        //printf("drawing line %d\n", x);
        bitset(x,y1,ON);
        y1++;
      }
    } else
      bitset(x,y1,ON);

    y1 = y2;
  }
  //printf("done!\n");
  JWReDraw(image);
}

void testdraw() {

  //Turn upper half on
  /*
  for(x = 0; x<320; x++) {
    for(y = 0; y<100; y++)
      bitset(x,y,ON);
  }
  */

  //Saw tooth. 
  /*
  y = 10;
  asc = 1;
  for(x = 10; x < 300; x++) {
    if(y > 50)
      asc = 0;
    if(y< 10)
      asc = 1;
    if(asc)
      y++;
    else 
      y--;
    bitset(x,y,TOGGLE);
  }
  */

  //Pseudo Sine Wave.
  /*
  y = 20;  
  asc = 1;
  for(x = 0; x < 320; x++) {
    if(asc) {
      if(y > 100)
        y += (190-y)/10;
      else
        y += y/10;
    } else {
      if(y > 100)
        y -= (190-y)/10;
      else
        y -= y/10;
    }

    if(y<20) {
      y = 20;
      asc = 1;
    } else if(y>180) {
      y = 180;
      asc = 0;
    }
    bitset(x,y,TOGGLE);
  }
  */

}
