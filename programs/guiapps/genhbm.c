#include <stdlib.h>
#include <stdio.h>

unsigned char * bitmap;

int COLS,ROWS,BW;

unsigned char bits[8] = {128,64,32,16,8,4,2,1};
unsigned char offbits[8] = {254,253,251,247,239,223,191,127};

unsigned int * rowlookup;

void biton(int x, int y) {
  int row, character, line, bit;

  row       = y/8;
  character = x&(65535-7);
  line      = y&7;
  bit       = x&7;

  bitmap[character * rowlookup[row] + line] |= bits[bit];
}

void bitoff(int x, int y) {
  int row, character, line, bit;

  row       = y/8;
  character = x&(65535-7);
  line      = y&7;
  bit       = x&7;

  bitmap[character + rowlookup[row] + line] &= offbits[bit];
}

void main(int argc, char * argv[]) {

  int asc;

  int x,y,i;
  FILE * outfile;

  if(argc < 3) {
    COLS = 40;
    ROWS = 25;
  } else {
    COLS = atoi(argv[1]);
    ROWS = atoi(argv[2]);
  }

  BW = 240;
  
  //generate lookup table for rows.
  rowlookup = (unsigned int *)malloc(sizeof(int) * ROWS);
  y = 0;
  i = COLS * 8;
  for(x = 0; x<ROWS; x++) {
    rowlookup[x] = y;
    y += i;
  }

  bitmap = (unsigned char *)malloc(COLS * ROWS * 8);

  memset(bitmap, 0, 8 * COLS*ROWS);

  //Draw a horizontal line
  for(x = 0; x< 320;x++)
    biton(x,1);
  for(x = 0; x< 320;x++)
    biton(x,2);
  for(x = 0; x< 320;x++)
    biton(x,7);
  for(x = 0; x< 320;x++)
    biton(x,9);
  for(x = 0; x< 320;x++)
    biton(x,10);

/*
  //Draw a vertical line
  for(y = 0; y< 200;y++)
    biton(0,y);
*/

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
    biton(x,y);
  }

  y = 0;  
  asc = 1;
  for(x = 0; x < 320; x++) {
    if(asc)
      y += (200-y)/10;
    else
      y -= (200-y)/10;

    if(y<0) {
      y = 0;
      asc = 1;
    } else if(y>200) {
      y = 200;
      asc = 0;
    }
    biton(x,y);
  }
*/
  outfile = fopen("outfile.hbm", "w");

  for(i=0;i<ROWS*COLS*8;i++) 
    fwrite(&bitmap[i], 1,1, outfile);

  for(i=0;i<ROWS*COLS;i++) 
    fputc(BW, outfile);

  fclose(outfile);
}
