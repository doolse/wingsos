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
#define SEARCH     15

static char buf[512];

MenuData mediamenu[]={
  {"JosPEG",  0, NULL, 0, JPEG,    NULL, NULL},
  {"JosMOD",  0, NULL, 0, JOSMOD,  NULL, NULL},
  {"WavPlay", 0, NULL, 0, WAVPLAY, NULL, NULL},
  {NULL,      0, NULL, 0, 0,       NULL, NULL}
};

MenuData progmenu[]={
  {"AJirc",       0, NULL, 0, AJIRC, NULL, NULL},
  {"MineSweeper", 0, NULL, 0, MINE,  NULL, NULL}, 
  {"Search",      0, NULL, 0, SEARCH,NULL, NULL},
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

unsigned char icon[]={0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 
0x0F};

MenuData rootmenu[]={
  {"Programs",         0, NULL, 0, 0,          NULL, progmenu},
  {"Multi-Media",      0, NULL, 0, 0,          NULL, mediamenu},
  {"Tools",            0, NULL, 0, 0,          NULL, toolsmenu},
  {"File Browser",     0, NULL, 0, CMD_BROWSE, NULL, NULL},
  {"Help",             0, NULL, 0, CMD_HELP,   NULL, NULL},
  {"WiNGs Credits",    0, NULL, 0, CREDITS,    NULL, NULL}, 
  {"Exit",             0, NULL, 0, EXIT,       NULL, NULL},
  {NULL,               0, NULL, 0, 0,          NULL, NULL}
};

void * TxtArea, * TxtBar;

void puttext();     //executes as a shell command.
void RunJosmod();
void RunWavplay();
void RunCredits();
void RunWinapp(); 
void gunzip();     //Opens a new thread.
void ShowHelp();

void handlemenu(void *Self, MenuData *item) {
  switch(item->command) {
    case EXIT:
      exit(-1);
      break;

    case AJIRC:
      JTxtAppend(TxtArea, "Opening AJIRC. Right click it for Options.\n");
      system("ajirc &");
      break;
    case MINE:
      JTxtAppend(TxtArea, "Starting MineSweeper!\n");
      system("mine &");
      break;
    case SEARCH: 
      JTxtAppend(TxtArea, "Use Search to Search for files\n");
      system("search &");
      break;
    case JPEG:
      JTxtAppend(TxtArea, "Displaying a Jpeg!\n");
      spawnlp(0, "jpeg", JTxfGetText(TxtBar), NULL);
      break;

    case GUNZIP:
      newThread(gunzip, STACK_DFL, NULL);
      break;
    case GUITEXT:     
      spawnlp(0, "guitext", JTxfGetText(TxtBar), NULL);
      break;

    case PS:
      system("ps |guitext -w 208 -h 80 &");
      break;
    case LS:
      sprintf(buf, "ls %s |guitext -w 70 -h 100 &", JTxfGetText(TxtBar));
      system(buf);  
      break;
    case MEM:
      system("mem |guitext -h 60 &");
      break;

    case JOSMOD:
      RunJosmod();
      break;
    case WAVPLAY:
      RunWavplay();
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
  }
}

void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs) {
  void * temp;
  if(Type == EVS_But2Up){
    temp = JMnuInit(NULL, NULL, rootmenu, XAbs, YAbs, handlemenu);
    JWinShow(temp);
  }
}

int Winapp  = 0;
int Josmod  = 0;
int Wavplay = 0;
int Credits = 0;

void main() {
  void *Appl, *MainWindow;

  Appl       = JAppInit(NULL, 0);
  MainWindow = JWndInit(NULL, NULL,       0, "The WiNGs Launcher -Greg/DAC-", 0);
  TxtArea    = JTxtInit(NULL, MainWindow, 0, "");
  TxtBar     = JTxfInit(NULL, MainWindow, 0, "");

  JWinSize(MainWindow, 160, 48);
  JWinGeom(TxtArea, 0, 0,  160, 32, GEOM_TopLeft | GEOM_TopLeft);
  JWinGeom(TxtBar,  0, 32, 0,   0,  GEOM_TopLeft | GEOM_BotRight2);

  JWinOveride(TxtBar,     MJTxf_Entered, puttext);
  JWinOveride(MainWindow, MJW_RButton,   RightBut);

  JWinShow(MainWindow);

  JWinSetBack(TxtArea, COL_LightGreen);
  JTxtAppend(TxtArea, "Right Click For Options\n");

  JAppSetMain(Appl, MainWindow);
  JAppLoop(Appl);
}

void ShowHelp() {
  JTxtAppend(TxtArea, " - Type commands in TextBar.\n");
  JTxtAppend(TxtArea, " - Right Click for options.\n");
  JTxtAppend(TxtArea, " - Path/Filename in Textbar.\n");
}

void puttext(){
  JTxtAppend(TxtArea, "Attempting to Run Command\n");
  system(JTxfGetText(TxtBar));
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

void gunzip() {
  JTxtAppend(TxtArea, "Unzipping File...\n");
  sprintf(buf, "gunzip %s", JTxfGetText(TxtBar));
  system(buf);
  JTxtAppend(TxtArea, "Unzipping complete.\n");
}
