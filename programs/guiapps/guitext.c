#include <ctype.h>
#include <stdio.h>
#include <wgslib.h>
#include <winlib.h>
#include <string.h>
#include <stdlib.h>

FILE *fp;
void *window, *tmpbut1;
void handlemenu(void *Self, MenuData *item);
void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs);

unsigned char iconup[] = {
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x66, 0xff
};

unsigned char icondown[] = {
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff, 0x66
};

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

void addtext(void *self) {
  void *textarea;
  char *temp;

  temp = (char *)malloc(10);

  sprintf(temp, "%d", ((JChk *)tmpbut1)->Status);

  textarea = JWGetData(self);  

  JTxtAppend(textarea, temp);
  JTxtAppend(textarea, "This is the example text\n");
}

int main(int argc, char* argv[]){

  void *appl, *scr, *butcon, *tmpbut, *textarea1, *textarea2;
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

  if(filename == NULL) 
    tempbuf = strdup("GuiText Stream -Greg/DAC-");
  else {
    tempbuf = (char *)malloc(strlen("GuiText  -Greg/DAC-")+strlen(filename)+2);
    sprintf(tempbuf, "GuiText %s -Greg/DAC-", filename);
  }

  appl = JAppInit(NULL, 0);
  window  = JWndInit(NULL, tempbuf, JWndF_Resizable);
  //((JCnt *)window)->Orient = 1;

  JWSetBounds(window, 0,40, 100, 100);

  JAppSetMain(appl, window);

  textarea1 = JTxtInit(NULL);
  scr = JScrInit(NULL, textarea1, JScrF_HAlways);
  JCntAdd(window, scr);

  textarea2 = JTxtInit(NULL);
  scr = JScrInit(NULL, textarea2, JScrF_VAlways);
  JCntAdd(window, scr);

  butcon = JCntInit(NULL);

  ((JCnt*)butcon)->Orient = 2;

  JCntAdd(window, butcon);

  tmpbut = JButInit(NULL, "Test  ");
  JWSetData(tmpbut, textarea1);
  JWinCallback(tmpbut, JBut, Clicked, addtext);
  JCntAdd(butcon, tmpbut);
  tmpbut = JButInit(NULL, "Test1 ");
  JWSetData(tmpbut, textarea2);
  JWinCallback(tmpbut, JBut, Clicked, addtext);
  JCntAdd(butcon, tmpbut);
  tmpbut = JButInit(NULL, "Test2");
  JCntAdd(butcon, tmpbut);
  tmpbut = JButInit(NULL, "Test3");
  JCntAdd(butcon, tmpbut);
  tmpbut = JButInit(NULL, "Test4");
  JCntAdd(butcon, tmpbut);
  tmpbut1 = JChkInit(NULL, "Test5");
  JCntAdd(butcon, tmpbut1);
  tmpbut = JChkInit(NULL, "Test6");
  JCntAdd(butcon, tmpbut);

/*
  tmpbut = JIbtInit(NULL, 0, 0, 16, 16, iconup, icondown);
  JCntAdd(butcon, tmpbut);
*/

  JWSetBack(textarea1, COL_Green);
  JWSetBack(textarea2, COL_Yellow);
  JWinCallback(window, JWnd, RightClick, RightBut);

  JWinShow(window);

/*
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
*/

  //JBarSetVal(scr, 0L, 1);

  JAppLoop(appl);

  return(0);
}
      
void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs){
  void *temp=NULL;
  temp = JMnuInit(NULL, themenu, XAbs, YAbs, handlemenu);
  JWinShow(temp);
}  

void handlemenu(void *Self, MenuData *item) {
  switch(item->command) {
    case CMD_EXIT:
      exit(1);
      break;
  }
}

