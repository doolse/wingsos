#include <ctype.h>
#include <stdio.h>
#include <wgslib.h>
#include <winlib.h>
#include <string.h>
#include <stdlib.h>

unsigned char app_icon[] = {
0,63,80,151,240,23,16,23,
0,248,4,244,4,244,4,244,
16,23,16,16,23,24,15,0,
4,244,4,4,255,2,252,0,
0x01,0x01,0x01,0x01
};

void helptext() {
  fprintf(stderr, "USAGE: guitext [-h height][-w width][-f filename]\n");
  fprintf(stderr, "       if no filename is supplied guitext takes from standard in"); 
  exit(1);
}

int main(int argc, char* argv[]){
  FILE *fp;
  char * tempbuf;
  void *appl, *scr, *textarea, *window;
  int ch;
  char *filename = NULL;
  char *buf      = NULL;
  int size       = 0;
  int height     = 152;
  int width      = 184;
  JMeta * metadata = malloc(sizeof(JMeta));

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

  tempbuf = filename;
  if(!tempbuf) 
    tempbuf = strdup("GuiText Stream");

  metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
  metadata->title = strdup(tempbuf);
  metadata->icon = app_icon;
  metadata->showicon = 1;
  metadata->parentreg = -1;

  appl   = JAppInit(NULL, 0);
  window = JWndInit(NULL, metadata->title, JWndF_Resizable,metadata);
  JAppSetMain(appl, window);

  JWSetBounds(window, 40,16, width,height);
  JWSetMin(window,40,16);
  JWSetMax(window,288,168);
  JWndSetProp(window);

  textarea = JTxtInit(NULL);
  scr = JScrInit(NULL, textarea, JScrF_VNotEnd|JScrF_HNotEnd);
  JCntAdd(window, scr);

  JWSetBack(textarea, COL_White);
  JWSetPen(textarea, COL_Blue);

  if(!filename) {
    while(getline(&buf, &size, stdin) != EOF) 
      JTxtAppend(textarea, buf);
  } else {
    if(fp = fopen(filename, "rb")) {
      while(getline(&buf, &size, fp) != EOF)
        JTxtAppend(textarea, buf);
      fclose(fp);
    } else {
      JTxtAppend(textarea, "Error, File '");
      JTxtAppend(textarea, filename);
      JTxtAppend(textarea, "' Couldn't be opened.\n\n");
    }
  }

  retexit(0);

  JWinShow(window);
  JAppLoop(appl);

  return(0);
}

