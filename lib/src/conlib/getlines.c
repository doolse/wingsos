//Custom Getline functions.  

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

//getmyline is a complete interactive line editor, including a 
//password option, specified size, and x,y location on screen.

char * getmyline(char * original, int size, int x, int y, int password) {
  int i,pos,count = 0;
  char * linebuf, * ptr, *clearstr, *mvptr;

  linebuf = calloc(size+1,1);
  clearstr = calloc(size+1,1);

  memset(clearstr, ' ',size);

  if(!original)
    original = strdup("");

  i = strlen(original);
  if(i > size)
    i = size;

  strncpy(linebuf,original,i);
  pos = count = i;
  ptr = strchr(linebuf,0);

  if(!password) {
    con_gotoxy(x,y);
    printf("%s",linebuf);
  } else {
    for(i=0;i<strlen(linebuf);i++) {
      con_gotoxy(x+i,y);
      putchar('*');
    }
  }
  con_update();

  while(1) {
    i = con_getkey();
  
    if(i == ESC) { 
      free(linebuf);
      return(original);
    }

    //Cursor left
    if(i == CURL && pos > 0) {
      pos--;
      ptr--;
      con_gotoxy(x+pos,y);
      con_update();
    }

    //Cursor right
    if(i == CURR && *ptr != 0) {
      pos++;
      ptr++;
      con_gotoxy(x+pos,y);
      con_update();
    }

    if(i > 31 && i < 127 && i != 47 && count < size) {
      if(*ptr != 0) {
        mvptr = strchr(ptr,0);
        while(mvptr != ptr) {
          *mvptr = *(mvptr-1);
          mvptr--;
        }
      }
      *ptr = i;
      ptr++;
      count++;
      pos++;
      con_gotoxy(x,y);
      printf("%s",clearstr);
      con_gotoxy(x,y);
      if(!password)
        printf("%s",linebuf);
      else {
        for(i=0;i<strlen(linebuf);i++) {
          con_gotoxy(x+i,y);
          putchar('*');
        }
      }
      con_gotoxy(x+pos,y);
      con_update();
    } else if(i == DEL && count > 0 && pos > 0) {
      mvptr = ptr;
      while(*mvptr != 0) {
        *(mvptr-1) = *mvptr;
        mvptr++;
      }
      *(mvptr-1) = 0;
      ptr--;
      pos--;
      count--;
      con_gotoxy(x,y);
      printf("%s",clearstr);
      con_gotoxy(x,y);
      if(!password)
        printf("%s",linebuf);
      else {
        for(i=0;i<strlen(linebuf);i++) {
          con_gotoxy(x+i,y);
          putchar('*');
        }
      }
      con_gotoxy(x+pos,y);
      con_update();
    } else if(i == '\n' || i == '\r')
      break;
  }

  free(original);
  return(linebuf);
}

int redrawline(char * original, char * start, char * pos, int size, int x, int y, int pass) {
  char * ptr = start;
  int i = 0;

  con_gotoxy(x,y);

  if(original < start) {
    putchar('<');
    i++;
  }
  
  if(!pass) { 
    while(i<size) {
      if(*ptr) {
        putchar(*ptr);
        ptr++;
      } else {
        putchar(' ');
      }
      i++;
    }
  } else {
    while(i<size) {
      if(*ptr) {
        putchar('*');
        ptr++;
      } else {
        putchar(' ');
      }
      i++;
    }
  }
 
  if(*ptr) {
    con_gotoxy(x+size-1,y);
    putchar('>');
  }

  if(start == original) {
    con_gotoxy(x+(pos-start),y);
    con_update();
    return(pos-start);
  } else {
    con_gotoxy(x+(pos-start)+1,y);
    con_update();
    return(pos-start+1);
  }
}

char * getmylinerestrict(char * original, long size, int displaysize, int x, int y, char * restrict, int password) {
  int i,pos,count = 0;
  char * linebuf, * ptr, *clearstr, *mvptr, *displayptr;

  if(displaysize < 10) {
    //drawmessagebox("fatal error:","displaysize must be greater than 10.",1);
    exit(1);
  }

  displayptr = linebuf = calloc(size+1,1);

  if(!original)
    original = strdup("");

  count = i = strlen(original);
  if(i > size)
    count = i = size;

  strncpy(linebuf,original,i);
  ptr = strchr(linebuf,0);

  while((ptr - displayptr) > displaysize)
    displayptr += 10;

  pos = redrawline(linebuf, displayptr, ptr, displaysize, x, y, password);

  while(1) {
    i = con_getkey();
  
    if(i == ESC) { 
      free(linebuf);
      return(original);
    }

    //Cursor left
    if(i == CURL && ptr > linebuf) {
      ptr--;
      if(pos > 0) {
        pos--;
        con_gotoxy(x+pos,y);
        con_update();
      } else {
        if(displayptr - linebuf > 10)
          displayptr -= 10;
        else 
          displayptr = linebuf;
        pos = redrawline(linebuf, displayptr, ptr, displaysize, x, y, password);
      }
    }

    //Cursor right
    else if(i == CURR && *ptr != 0) {
      ptr++;
      if(pos < displaysize) {
        pos++;
        con_gotoxy(x+pos,y);
        con_update();
      } else {
        displayptr += 10;
        pos = redrawline(linebuf, displayptr, ptr, displaysize, x, y, password);
      }
    } 

    //Start of line: C= a
    else if(i == 1) {
      displayptr = ptr = linebuf;
      pos = redrawline(linebuf, displayptr, ptr, displaysize, x, y, password);
    }

    //End of line: C= e
    else if(i == 5) {
      ptr = strchr(linebuf,0);
      while(ptr - displayptr > displaysize)
        displayptr += 10;
      pos = redrawline(linebuf, displayptr, ptr, displaysize, x, y, password);
    }
    
    if(restrict) {
      if(strchr(restrict,i))
        continue;
    }

    if(i > 31 && i < 127 && count < size) {
      if(*ptr != 0) {
        mvptr = strchr(ptr,0);
        while(mvptr != ptr) {
          *mvptr = *(mvptr-1);
          mvptr--;
        }
      }
      *ptr = i;
      ptr++;
      count++;
      pos++;
      if(pos > displaysize)
        displayptr += 10;

      pos = redrawline(linebuf, displayptr, ptr, displaysize, x, y, password);

    } else if(i == 8 && count > 0 && ptr > linebuf) {
      mvptr = ptr;
      while(*mvptr != 0) {
        *(mvptr-1) = *mvptr;
        mvptr++;
      }
      *(mvptr-1) = 0;
      ptr--;
      count--;
      if(pos == 0) {
        if(displayptr - linebuf < 10)
          displayptr = linebuf;
        else
          displayptr -= 10;
      } else {
        pos--;
      }
      pos = redrawline(linebuf, displayptr, ptr, displaysize, x, y, password);

    } else if(i == '\n' || i == '\r')
      break;
  }

  free(original);
  return(linebuf);
}

char * getmylinen(int size, int x, int y) {
  int i,count = 0;
  char * linebuf;

  linebuf = (char *)malloc(size+1);

  con_gotoxy(x,y);
  con_update();

  while(1) {
    i = con_getkey();
    if(i > 47 && i < 58 && count < size) {
      linebuf[count] = i;
      con_gotoxy(x+count,y);
      putchar(i);
      linebuf[++count] = 0;
      con_update();
    } else if(i == DEL && count > 0) {
      count--;
      con_gotoxy(x+count,y);
      putchar(' ');
      con_gotoxy(x+count,y);
      linebuf[count] = 0;
      con_update();
    } else if(i == '\n' || i == '\r')
      break;
  }
  return(linebuf);

}
