#include <stdio.h>
#include <wgslib.h>
#include <winlib.h>
#include <string.h>
#include <stdlib.h>

FILE *fp;
void quit();
void *window;

int main(int argc, char* argv[]){

  void *appl, *TxtArea;
  char *buf=NULL;
  char *buf2=NULL;
  int stream = 0;
  int size=0;

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

  JWinOveride(window, MJW_RButton, quit);

  TxtArea = JTxtInit(NULL, window, 0, "");
  JWinGeom(TxtArea, 0, 0, 0, 0, GEOM_TopLeft | GEOM_BotRight2);
  JWinSetBack(TxtArea, COL_White);

  if(stream) {
    while(getline(&buf, &size, stdin) != -1)
      JTxtAppend(TxtArea, buf);
  } else {
    fp = fopen(argv[1], "r");
    if(!fp) 
      JTxtAppend(TxtArea, "Error, File Couldn't be opened. It probably doesn't exist.\n\nUSAGE: guitext filename.txt\n");
    else{
      while(getline(&buf, &size, fp) != -1){
        JTxtAppend(TxtArea, buf);
      }
    }
  }
  JBarSetVal(JTxtVBar(TxtArea), 0L, 1);
  JWinShow(window);
  JAppLoop(appl);

  return -1;
}
      
void quit(){
  fclose(fp);
  exit(-1);
}  
  
