#include <ctype.h>
#include <stdio.h>
#include <wgslib.h>
#include <winlib.h>
#include <string.h>
#include <stdlib.h>

FILE *fp;
void *window;

void helptext() {
  fprintf(stderr, "USAGE: guitext [-h height][-w width][-f filename]\n");
  fprintf(stderr, "       if no filename is supplied guitext takes from standard in"); 
  exit(1);
}

int main(int argc, char* argv[]){

  char * tempbuf;
  void *appl, *scr, *textarea1;
  int ch;

  char *filename = NULL;
  char *buf      = NULL;
  int size       = 0;
  int height     = 150;
  int width      = 180;

  while((ch = getopt(argc, argv, "h:w:f:")) != EOF) {
    switch(ch) {
      case 'h':
       height = atoi(optarg);
      break;
      case 'w':
       width = atoi(optarg);
      break;
      case 'f':
        filename = strdup(optarg);
      break;
      default:
        helptext();
      break;
    }
  }

  if(filename == NULL) 
    tempbuf = strdup("GuiText Stream");
  else {
    tempbuf = filename;
  }

  appl = JAppInit(NULL, 0);
  window  = JWndInit(NULL, tempbuf, 0);

  JWSetBounds(window, 40,0, width,height);
  JWSetMin(window,16,16);
  JWSetMax(window,288,168);

  JAppSetMain(appl, window);

  textarea1 = JTxtInit(NULL);
  scr = JScrInit(NULL, textarea1, JScrF_VNotEnd|JScrF_HNotEnd);
  JWSetMin(scr,8,8);

  JCntAdd(window, scr);

  JWSetBack(textarea1, COL_White);
  JWSetPen(textarea1, COL_Blue);

  JWndSetProp(window);
  JWinShow(window);

  if(!filename) {
    while(getline(&buf, &size, stdin) != EOF) 
      JTxtAppend(textarea1, buf);
  } else {
    fp = fopen(filename, "r");
    if(!fp) { 
      JTxtAppend(textarea1, "Error, File '");
      JTxtAppend(textarea1, filename);
      JTxtAppend(textarea1, "' Couldn't be opened.\n\n");
    } else {
      while(getline(&buf, &size, fp) != EOF)
        JTxtAppend(textarea1, buf);
      fclose(fp);
    }
  }

  retexit(1);

  JAppLoop(appl);

  return(0);
}

