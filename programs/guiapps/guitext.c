#include <ctype.h>
#include <stdio.h>
#include <wgslib.h>
#include <winlib.h>
#include <string.h>
#include <stdlib.h>

FILE *fp;
void *window;
void handlemenu(void *Self, MenuData *item);
void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs);

MenuData themenu[]={
  {"Greg/DAC in 2002", 0, NULL, 0, 0,        NULL, NULL},
  {"Exit",             0, NULL, 0, CMD_EXIT, NULL, NULL},
  {NULL,               0, NULL, 0, 0,        NULL, NULL}
};

void helptext() {
  fprintf(stderr, "USAGE: guitext [-h height][-w width][-f filename]\n");
  fprintf(stderr, "       if no filename is supplied guitext takes from standard in"); 
  exit(1);
}

int main(int argc, char* argv[]){

  void *appl, *TxtArea;
  int ch;

  char *filename = NULL;
  char *buf      = NULL;
  char *tempbuf  = NULL;
  int size       = 0;
  int height     = 190;
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

  appl = JAppInit(NULL, 0);

  if(filename == NULL) 
    tempbuf = strdup("GuiText Stream -Greg/DAC-");
  else {
    tempbuf = (char *)malloc(strlen("GuiText  -Greg/DAC-")+strlen(filename)+2);
    sprintf(tempbuf, "GuiText %s -Greg/DAC-", filename);
  }

  window  = JWndInit(NULL, NULL,   0, tempbuf, JWndF_Resizable);
  TxtArea = JTxtInit(NULL, window, 0, "");

  JWinSize(window, width, height);
  JWinGeom(TxtArea, 0, 0, 0, 0, GEOM_TopLeft | GEOM_BotRight2);
  JWinSetBack(TxtArea, COL_White);

  JWinCallback(window, JWnd, RightClick, RightBut);

  if(!filename) {
    while(getline(&buf, &size, stdin) != -1) 
      JTxtAppend(TxtArea, buf);
  } else {
    fp = fopen(filename, "r");
    if(!fp) 
      JTxtAppend(TxtArea, "Error, File Couldn't be opened. It probably doesn't exist.\n\n");
    else{
      while(getline(&buf, &size, fp) != EOF)
        JTxtAppend(TxtArea, buf);
    }
  }

  JBarSetVal(JTxtVBar(TxtArea), 0L, 1);

  JAppSetMain(appl, window);
  JWinShow(window);
  JAppLoop(appl);

  return(0);
}
      
void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs){
  void *temp=NULL;
  temp = JMnuInit(NULL, NULL, themenu, XAbs, YAbs, handlemenu);
  JWinShow(temp);
}  

void handlemenu(void *Self, MenuData *item) {
  switch(item->command) {
    case CMD_EXIT:
      exit(1);
      break;
  }
}

