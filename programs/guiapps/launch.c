#include <winlib.h>
#include <stdio.h>
#include <wgslib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define CMD_BROWSE 1
#define CMD_HELP   2
#define JPEG       3
#define JOSMOD     4
#define GUNZIP     5
#define GUITEXT    6
#define MINE       7
#define WAVPLAY    8
#define AJIRC      9
#define EXIT       10
#define LS         11
#define PS         12
#define MEM        13
#define CREDITS    14

void *TxtArea       = NULL;
void *TxtBar        = NULL;
void *MainWindow    = NULL;
void *OptionsWindow = NULL;
void *Appl          = NULL;

int Jpeg    = 0; 
int Winapp  = 0;
int Josmod  = 0;
int Wavplay = 0;
int Credits = 0;

char buf[255];

MenuData mediamenu[]={
  {"JosPEG",  0, NULL, 0, JPEG,    NULL, NULL},
  {"JosMOD",  0, NULL, 0, JOSMOD,  NULL, NULL},
  {"WavPlay", 0, NULL, 0, WAVPLAY, NULL, NULL},
  {NULL,      0, NULL, 0, 0,       NULL, NULL}
};

MenuData progmenu[]={
  {"AJirc",       0, NULL, 0, AJIRC, NULL, NULL},
  {"MineSweeper", 0, NULL, 0, MINE,  NULL, NULL}, 
  {NULL,          0, NULL, 0, 0,     NULL, NULL}
};

MenuData toolsmenu[]={
  {"GunZIP",       0, NULL, 0, GUNZIP,  NULL, NULL},
  {"GuiText",      0, NULL, 0, GUITEXT, NULL, NULL},
  {"File List",    0, NULL, 0, LS,      NULL, NULL},
  {"Process List", 0, NULL, 0, PS,      NULL, NULL},
  {"Memory Info",  0, NULL, 0, MEM,     NULL, NULL},
  {NULL,           0, NULL, 0, 0,       NULL, NULL}
};

MenuData rootmenu[]={
  {"Multi-Media",      0, NULL, 0, 0,          NULL, mediamenu},
  {"Programs",         0, NULL, 0, 0,          NULL, progmenu},
  {"Tools",            0, NULL, 0, 0,          NULL, toolsmenu},
  {"File Browser",     0, NULL, 0, CMD_BROWSE, NULL, NULL},
  {"Help",             0, NULL, 0, CMD_HELP,   NULL, NULL},
  {"WiNGs Credits",    0, NULL, 0, CREDITS,    NULL, NULL}, 
  {"Exit",             0, NULL, 0, EXIT,       NULL, NULL},
  {NULL,               0, NULL, 0, 0,          NULL, NULL}
};

void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs);
void handlemenu(void *Self, MenuData *item);

void ShowHelp();
void RunWinapp(); 
void puttext(); //executes as a shell command.

void RunAjirc();   void RunMine();   void RunJpeg();    void RunJosmod();
void RunWavplay(); void RunGunzip(); void RunGuitext(); void RunPS();
void RunLS();      void RunMem();    void RunCredits();

void main() {
  Appl       = JAppInit(NULL, 0);
  MainWindow = JWndInit(NULL, NULL, 0, "The WiNGs Launcher -Greg/DAC-", 0);

  JAppSetMain(Appl, MainWindow);
  JWinSize(MainWindow, 160, 48);

  JWinOveride(MainWindow, MJW_RButton, RightBut);

  TxtArea = JTxtInit(NULL, MainWindow, 0, "");

  JTxtAppend(TxtArea, "Right Click For Options\n");

  JWinGeom(TxtArea, 0, 0, 160, 32, GEOM_TopLeft | GEOM_TopLeft);
  JWinSetBack(TxtArea, COL_LightGreen);

  TxtBar = JTxfInit(NULL, MainWindow, 0, "/");
  JWinGeom(TxtBar, 0, 32, 0, 0, GEOM_TopLeft | GEOM_BotRight2);
  JWinOveride(TxtBar, MJTxf_Entered, puttext);

  JWinShow(MainWindow);
  JAppLoop(Appl);
}

void ShowHelp() {
  JTxtAppend(TxtArea, "-----------\n");
  JTxtAppend(TxtArea, "To Run a shell Command, type it in the textbar and press return.\n");
  JTxtAppend(TxtArea, "To Use a program type the path and filename in the textbar, and choose the program from the right click menu.\n");
  JTxtAppend(TxtArea, "To Browse files, type the path in the textbar, click file browser from the rightclick menu.\n");
  JTxtAppend(TxtArea, "-----------\n");
}

void handlemenu(void *Self, MenuData *item) {
  switch(item->command) {
    case EXIT:
      exit(1);
      break;
    case CMD_HELP:
      ShowHelp();
      break;
    case CREDITS:
      RunCredits();
      break;
    case CMD_BROWSE:
      RunWinapp();
      break;
    case AJIRC:
      RunAjirc();
      break;
    case MINE:
      RunMine();
      break;
    case JPEG:
      RunJpeg();
      break;
    case JOSMOD:
      RunJosmod();
      break;
    case WAVPLAY:
      RunWavplay();
      break;
    case GUNZIP:
      RunGunzip();
      break;
    case GUITEXT:     
      RunGuitext();
      break;
    case PS:
      RunPS();
      break;
    case LS:
      RunLS();
      break;
    case MEM:
      RunMem();
      break;
  }
}

void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs) {
  void * temp=NULL;
  if(Type == EVS_But2Up){
    temp = JMnuInit(NULL, NULL, rootmenu, XAbs, YAbs, handlemenu);
    JWinShow(temp);
  }
}

void puttext(){
  char * buf=NULL;
  JTxtAppend(TxtArea, "Attempting to Run Command\n");
  buf = (char *)malloc(strlen(JTxfGetText(TxtBar)) + 3);
  if(buf == NULL)
    exit(-1);
  sprintf(buf, "%s &", JTxfGetText(TxtBar));
  system(buf);
  free(buf);
}
  
void RunMine() {
  JTxtAppend(TxtArea, "Starting MineSweeper!\n");
  spawnlp(0, "mine", NULL);
}

void RunCredits() {
  if(Credits != 0) {
    JTxtAppend(TxtArea, "Closing Credits.\n");
    sprintf(buf, "%d", Credits);
    spawnlp(0, "kill", buf, NULL);
    Credits = 0;
  } else {
    JTxtAppend(TxtArea, "Showing you the Credits...\n");
    Credits = spawnlp(0, "credits", NULL);
  }
}

void RunJpeg() {
  if(Jpeg != 0) {
    JTxtAppend(TxtArea, "Closing Jpeg!\n");
    sprintf(buf, "%d", Jpeg);
    spawnlp(0, "kill", buf, NULL);
    Jpeg = 0;
  } else {
    JTxtAppend(TxtArea, "Displaying a Jpeg!\n");
    Jpeg = spawnlp(0, "jpeg", JTxfGetText(TxtBar), NULL);
  }
}

void RunMem() {
  system("mem |guitext &");
}

void RunLS() {
  sprintf(buf, "ls %s |guitext &", JTxfGetText(TxtBar));
  system(buf);  
}

void RunPS() {
  system("ps |guitext &");
}

void RunAjirc() {
  JTxtAppend(TxtArea, "Opening AJIRC. Right click it for Options.\n");
  spawnlp(0, "ajirc", NULL);
}

void RunWinapp() {
  if(Winapp !=0){
    sprintf(buf, "%d", Winapp);
    spawnlp(0, "kill", buf, NULL);
    Winapp=0;
  } else {
    Winapp = spawnlp(0, "winapp", JTxfGetText(TxtBar), NULL);
  }
}

void RunJosmod() {
  if(Josmod != 0){
    JTxtAppend(TxtArea, "Stopping Josmod...\n");
    sprintf(buf, "%d", Josmod);
    spawnlp(0, "kill", buf, NULL);
    Josmod=0;
  } else {
    JTxtAppend(TxtArea, "Loading a Music File! It will start playing after it's finished loading. Depending on How large the file is, it may take some time to load.\n");
    Josmod = spawnlp(0, "josmod", "-h", "11000", JTxfGetText(TxtBar), NULL);
  }
}
void RunWavplay() {
  if(Wavplay != 0){
    JTxtAppend(TxtArea, "Stopping Wavplay...\n");
    sprintf(buf, "%d", Wavplay);
    spawnlp(0, "kill", buf, NULL);
    Wavplay=0;
  } else {
    Wavplay = spawnlp(0, "wavplay", JTxfGetText(TxtBar), NULL);
  }
}
void RunGunzip() {
  spawnlp(0, "gunzip", JTxfGetText(TxtBar), NULL);
}

void RunGuitext(){
  spawnlp(0, "guitext", JTxfGetText(TxtBar), NULL);
}
