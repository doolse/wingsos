#include <stdlib.h>
#include <stdio.h>

typedef struct cell_s {
  int line[8];
  int colors;
} cell;

cell bitmap[40][25];

int COLS,ROWS,BW;

void biton(int x, int y) {
  int row, line, character, bit;

  row       = y/8;
  character = x/8;
  line      = y - (row*8);
  bit       = 7 - (x-(character*8));

  printf("line = %d, row = %d, char = %d\n", line, row, character);

  switch(bit) {
    case 0:
      bitmap[character][row].line[line] |= 1;
    break;
    case 1:
      bitmap[character][row].line[line] |= 2;
    break;
    case 2:
      bitmap[character][row].line[line] |= 4;
    break;
    case 3:
      bitmap[character][row].line[line] |= 8;
    break;
    case 4:
      bitmap[character][row].line[line] |= 16;
    break;
    case 5:
      bitmap[character][row].line[line] |= 32;
    break;
    case 6:
      bitmap[character][row].line[line] |= 64;
    break;
    case 7:
      bitmap[character][row].line[line] |= 128;
    break;
  }
}

void bitoff(int x, int y) {
  int row, line, character, bit;

  row       = y>>3;
  character = x>>3;
  line      = y - (row*8);
  bit       = 7 - (x-(character*8));

  switch(bit) {
    case 0:
      bitmap[character][row].line[line] &= 254;
    break;
    case 1:
      bitmap[character][row].line[line] &= 253;
    break;
    case 2:
      bitmap[character][row].line[line] &= 251;
    break;
    case 3:
      bitmap[character][row].line[line] &= 247;
    break;
    case 4:
      bitmap[character][row].line[line] &= 239;
    break;
    case 5:
      bitmap[character][row].line[line] &= 223;
    break;
    case 6:
      bitmap[character][row].line[line] &= 191;
    break;
    case 7:
      bitmap[character][row].line[line] &= 127;
    break;
  }
}

void main(int argc, char * argv[]) {

  int asc;

  int x,y,i;
  FILE * outfile;

  COLS = 40;
  ROWS = 25;
  BW = 240;

  for(x = 0;x<COLS;x++) {
    for(y = 0;y<ROWS;y++) {
      bitmap[x][y].line[0] = 0;
      bitmap[x][y].line[1] = 0;
      bitmap[x][y].line[2] = 0;
      bitmap[x][y].line[3] = 0;
      bitmap[x][y].line[4] = 0;
      bitmap[x][y].line[5] = 0;
      bitmap[x][y].line[6] = 0;
      bitmap[x][y].line[7] = 0;
      bitmap[x][y].colors = BW;
    }
  }

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
  for(x = 0; x<COLS; x++)
    bitmap[x][3].line[7] = 255;
*/

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

  for(y=0;y<ROWS;y++) {
    for(x=0;x<COLS;x++) {
        fwrite(&bitmap[x][y].line[0], 1,1, outfile);
        fwrite(&bitmap[x][y].line[1], 1,1, outfile);
        fwrite(&bitmap[x][y].line[2], 1,1, outfile);
        fwrite(&bitmap[x][y].line[3], 1,1, outfile);
        fwrite(&bitmap[x][y].line[4], 1,1, outfile);
        fwrite(&bitmap[x][y].line[5], 1,1, outfile);
        fwrite(&bitmap[x][y].line[6], 1,1, outfile);
        fwrite(&bitmap[x][y].line[7], 1,1, outfile);
    }
  } 

  for(y=0;y<ROWS;y++) {
    for(x=0;x<COLS;x++)
      fwrite(&bitmap[x][y].colors, 1,1, outfile);
  }

  fclose(outfile);
}
