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
  {"Greg/DAC in 2002", 0, NULL, 0, 0, NULL, NULL},
  {"Exit", 0, NULL, 0, CMD_EXIT, NULL, NULL},
  {NULL, 0, NULL, 0, 0, NULL, NULL}
};

int main(int argc, char* argv[]){

  void *appl, *TxtArea;
  char *buf  = NULL;
  char *buf2 = NULL;
  int stream = 0;
  int size   = 0;

  if(argc < 2) 
    stream = 1;

  buf2 = (char *)malloc(256);

  if(buf2 == NULL){
    printf("Malloc error\n\n");
    exit(-1);
  }

  appl = JAppInit(NULL, 0);

  if(stream)
    sprintf(buf2, "GuiText Stream -Greg/DAC-");
  else
    sprintf(buf2, "GuiText %s -Greg/DAC-", argv[1]);
  window = JWndInit(NULL, NULL, 0, buf2, JWndF_Resizable);
  JAppSetMain(appl, window);
  JWinSize(window, 180, 190);

  JWinOveride(window, MJW_RButton, RightBut);

  TxtArea = JTxtInit(NULL, window, 0, "");
  JWinGeom(TxtArea, 0, 0, 0, 0, GEOM_TopLeft | GEOM_BotRight2);
  JWinSetBack(TxtArea, COL_White);

  if(stream) {
    while(getline(&buf, &size, stdin) != -1) {
      JTxtAppend(TxtArea, buf);
      free(buf);
      buf = NULL;
      size = 0;
    }
  } else {
    fp = fopen(argv[1], "r");
    if(!fp) 
      JTxtAppend(TxtArea, "Error, File Couldn't be opened. It probably doesn't exist.\n\nUSAGE: guitext filename.txt\n");
    else{
      while(getline(&buf, &size, fp) != -1){
        JTxtAppend(TxtArea, buf);
        free(buf);
        buf = NULL;
        size = 0;
      }
    }
  }
  JBarSetVal(JTxtVBar(TxtArea), 0L, 1);
  JWinShow(window);
  JAppLoop(appl);

  return -1;
}
      
void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs){
  void *temp=NULL;
  if(Type == EVS_But2Up) {
    temp = JMnuInit(NULL, NULL, themenu, XAbs, YAbs, handlemenu);
    JWinShow(temp);
  }
}  

void handlemenu(void *Self, MenuData *item) {
  switch(item->command) {
    case CMD_EXIT:
      exit(1);
      break;
  }
}

