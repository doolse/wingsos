#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <fcntl.h>
#include <console.h>
#include <termio.h>
#include <xmldom.h>
#include "messagebox.h"

// Message Boxes

/*******************************************
*                                          *
*   Simple message box, draws a box in the *
*   centre of the screen. Automatically    *
*   sizes itself to fit the line length.   *
*                                          *
*      features:                           *
*                                          *
*      one or two line mode                *
*      press any key flag                  *
*                                          *
*   Messagebox Object, Uses a message box  *
*   structure, which is initialized and    *
*   can be kept in memory to be displayed  *
*   at any time.                           *
*                                          *
*     features:                            *
*                                          *
*     one two or three lines               *
*     proportional progress bar            *
*     functions to increment progress      *
*                                          *
*******************************************/

char   PROGBARCHAR = '*'; 

/* msgboxobj functions */

msgboxobj * initmsgboxobj(char * msgline1, char * msgline2, char * msgline3, int showprogress, ulong numofitems) {
  msgboxobj * mb;  
  int numoflines = 0;

  int height;
  int width1, width2, width3;
  int width;

  mb = (msgboxobj *)malloc(sizeof(msgboxobj));

  mb->msgline[0] = NULL;
  mb->msgline[1] = NULL;
  mb->msgline[2] = NULL;

  width1 = strlen(msgline1);
  width2 = strlen(msgline2);
  width3 = strlen(msgline3);

  if(width1) 
    mb->msgline[numoflines++] = msgline1;
  if(width2) 
    mb->msgline[numoflines++] = msgline2;
  if(width3) 
    mb->msgline[numoflines++] = msgline3;

  mb->numoflines = numoflines;

  mb->showprogress     = showprogress;
  mb->numofitems       = numofitems;
  mb->progressposition = 0;

  height = numoflines;
  if(showprogress)
    height += 2;

  height += 2;

  mb->top = (con_ysize - height)/2;
  mb->bottom = mb->top+height;

  width = width1;
  if(width < width2)
    width = width2;
  if(width < width3)
    width = width3;

  mb->left  = (con_xsize - width)/2;
  mb->right = mb->left + (width-1);

  mb->left  -=2;
  mb->right +=2;

  if(showprogress) {
    if(numofitems < width-2)
      mb->progresswidth = numofitems;
    else
      mb->progresswidth = width-2;
  }

  return(mb);
}

void drawmsgboxobj(msgboxobj * mb) {
  int x,y,i, width;
  char * blankline;

  mb->linelength = 0;

  /* Clear the rectangle */

  width = (mb->right - mb->left) +1;

  //allocate maximum width line
  blankline = (char *)malloc(width);
  
  //set string blank
  memset(blankline, ' ',width);
  blankline[width] = '\0';
  
  //clear the rectangle
  for(y=mb->top;y<mb->bottom;y++) {
    con_gotoxy(mb->left,y);
    printf("%s",blankline);
  }

  free(blankline);

  //Draw the frame
  //topline
  for(x=mb->left;x<mb->right;x++) {
    con_gotoxy(x,mb->top);
    putchar('_');
  }
  //botline
  for(x=mb->left+1;x<mb->right;x++) {
    con_gotoxy(x,mb->bottom);
    putchar('_');
  }
  //leftline
  for(y=mb->top+1;y<mb->bottom+1;y++) {
    con_gotoxy(mb->left,y);
    putchar('|');
  }
  //rightline
  for(y=mb->top+1;y<mb->bottom+1;y++) {
    con_gotoxy(mb->right,y);
    putchar('|');
  }

  //Draw Message Lines
  for(i=0;i<mb->numoflines;i++) {
    con_gotoxy(mb->left+2,mb->top+2+i);
    printf("%s",mb->msgline[i]);
  }

  //Draw Blank Progress Bar
  if(mb->showprogress) {
    con_gotoxy(mb->left+2, mb->bottom-1);
    putchar('|');
    con_gotoxy(mb->left+2+mb->progresswidth+1, mb->bottom-1);    
    putchar('|');
  }

  con_update();
}

void updatemsgboxprogress(msgboxobj * mb) {
  int i, x, y;
  int linesize;

  if(mb->progressposition <= mb->numofitems) {
    linesize = (mb->progresswidth * mb->progressposition) / mb->numofitems;

    if(linesize > mb->linelength) {    
      mb->linelength = linesize;

      x = mb->left+3;
      y = mb->bottom-1;
 
      linesize += x;

      for(i=x;i<linesize;i++) {
        con_gotoxy(i,y);
        putchar(PROGBARCHAR);
      }
      con_update();
    }
  } 

  //Sometimes the accuracy is slightly off. It'll only be a few bytes 
  //at most, and should be completely unnoticeable.
}

void incrementprogress(msgboxobj * mb) {
  mb->progressposition++;
  updatemsgboxprogress(mb);
}

void setprogress(msgboxobj * mb,ulong progress) {
  mb->progressposition = progress;
  updatemsgboxprogress(mb);
}

/* END of msgboxobj Functions */

/* Simple Message box call */

void drawmessagebox(char * string1, char * string2, int wait) {
  int width, startcolumn, row, i, padding1, padding2;

  if(strlen(string1) < strlen(string2)) {
    width = strlen(string2);

    if(width > (con_xsize - 6)) {
      width = con_xsize - 6;
      string2[width] = 0;
    }

    padding1 = width - strlen(string1);
    padding2 = 0;
  } else {
    width = strlen(string1);

    if(width > (con_xsize - 6)) {
      width = con_xsize - 6;
      string1[width] = 0;
    }

    padding1 = 0;
    padding2 = width - strlen(string2);
  }
  width = width+6;

  row         = 10;
  startcolumn = (con_xsize - width)/2;

  con_gotoxy(startcolumn, row);

  putchar(' ');
  for(i = 0; i < width-2; i++) 
    putchar('_');
  putchar(' ');

  row++;

  con_gotoxy(startcolumn, row);

  putchar(' ');
  putchar('|');
  for(i = 0; i < width-4; i++)
    putchar(' ');
  putchar('|');
  putchar(' ');
 
  row++;

  con_gotoxy(startcolumn, row);

  putchar(' ');
  printf("| %s", string1);

  for(i=0; i<padding1; i++)
    putchar(' ');

  putchar(' ');
  putchar('|');
  putchar(' ');

  row++;

  if(strlen(string2) > 0) {
    con_gotoxy(startcolumn, row);

    putchar(' ');
    printf("| %s", string2);
 
    for(i=0; i<padding2; i++)
      putchar(' ');

    putchar(' ');
    putchar('|');
    putchar(' ');

    row++;
  }

  con_gotoxy(startcolumn, row);

  putchar(' ');
  putchar('|');
  for(i = 0; i < width-4; i++) 
    putchar('_');
  putchar('|');
  putchar(' ');
  
  row++;

  con_gotoxy(startcolumn, row);

  for(i = 0; i < width; i++)
    putchar(' ');

  con_update();

  if(wait)
    con_getkey();
}

/* END simple messagebox call */
