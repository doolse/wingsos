#include <winlib.h>
#include <stdio.h>
#include <wgslib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define CMD_BROWSE 1
#define JPEG       3
#define JOSMOD     4
#define GUNZIP     5
#define GUITEXT    6
#define MINE       7
#define WAVPLAY    8
#define AJIRC      9
#define LS         11
#define PS         12
#define MEM        13
#define CREDITS    14
#define SEARCH     15
#define LOGTODAY   16
#define LOGYEST    17
#define WORDSERVE  18

//typedef struct 

static char buf[512];

MenuData mediamenu[]={
  {"JosPEG",  0, NULL, 0, JPEG,    NULL, NULL},
  {"JosMOD",  0, NULL, 0, JOSMOD,  NULL, NULL},
  {"WavPlay", 0, NULL, 0, WAVPLAY, NULL, NULL},
  {NULL,      0, NULL, 0, 0,       NULL, NULL}
};

MenuData progmenu[]={
  {"AJirc",        0, NULL, 0, AJIRC,    NULL, NULL},
  {"MineSweeper",  0, NULL, 0, MINE,     NULL, NULL}, 
  {"Search",       0, NULL, 0, SEARCH,   NULL, NULL},
  {"Word Services",0, NULL, 0, WORDSERVE,NULL, NULL},
  {NULL,           0, NULL, 0, 0,        NULL, NULL}
};

MenuData logmenu[]={
  {"Todays Log File",     0, NULL, 0, LOGTODAY, NULL, NULL},
  {"Yesterdays Log File", 0, NULL, 0, LOGYEST,  NULL, NULL},
  {NULL, 0, NULL, 0, 0, NULL, NULL}
};

MenuData toolsmenu[]={
  {"GunZIP",       0, NULL, 0, GUNZIP,  NULL, NULL},
  {"GuiText",      0, NULL, 0, GUITEXT, NULL, NULL},
/*  {"Log Files",    0, NULL, 0, 0,       NULL, logmenu},*/
  {"File List",    0, NULL, 0, LS,      NULL, NULL},
  {"Process List", 0, NULL, 0, PS,      NULL, NULL},
  {"Memory Info",  0, NULL, 0, MEM,     NULL, NULL},
  {NULL,           0, NULL, 0, 0,       NULL, NULL}
};

MenuData rootmenu[]={
  {"Programs",         0, NULL, 0, 0,          NULL, progmenu},
  {"Multi-Media",      0, NULL, 0, 0,          NULL, mediamenu},
  {"Tools",            0, NULL, 0, 0,          NULL, toolsmenu},
  {"File Browser",     0, NULL, 0, CMD_BROWSE, NULL, NULL},
  {"WiNGs Credits",    0, NULL, 0, CREDITS,    NULL, NULL}, 
  {NULL,               0, NULL, 0, 0,          NULL, NULL}
};

void * GoButton, * TxtBar;

void DoCommand(); 
void RunJosmod();
void RunWavplay();
void gunzip();     //Opens in a new thread.

void handlemenu(void *Self, MenuData *item) {
  switch(item->command) {
    case AJIRC:
      spawnlp(0, "ajirc", NULL);
      break;
    case MINE:
      spawnlp(0, "mine", NULL);
      break;
    case SEARCH: 
      spawnlp(0, "search", NULL);
      break;
    case WORDSERVE:
      spawnlp(0, "wordserve", NULL);
      break;
    case JPEG:
      spawnlp(0, "jpeg", JTxfGetText(TxtBar), NULL);
      break;

    case GUNZIP:
      newThread(gunzip, STACK_DFL, NULL);
      break;
    case GUITEXT:     
      spawnlp(0, "guitext", JTxfGetText(TxtBar), NULL);
      break;

    case PS:
      system("ps |guitext -w 208 -h 80");
      break;
    case LS:
      sprintf(buf, "ls %s |guitext -w 70 -h 100", JTxfGetText(TxtBar));
      system(buf);  
      break;
    case MEM:
      system("mem |guitext -h 56 -w 156");
      break;

    case JOSMOD:
      RunJosmod();
      break;
    case WAVPLAY:
      RunWavplay();
      break;

    case CREDITS:
      spawnlp(0, "credits", NULL);
      break;
    case CMD_BROWSE:
      spawnlp(0, "winapp", NULL);
      break;

    case LOGTODAY:
      system("htget starbase.globalpc.net/~vanessa/logs/log.today.txt |guitext -h 150 -w 284");
      break;
    case LOGYEST:
      system("htget starbase.globalpc.net/~vanessa/logs/log.yesterday.txt |guitext -h 150 -w 284");
      break;
  }
}

void ClickHandler(void *Self, int Type, int X, int Y, int XAbs, int YAbs) {
  void * temp;
  int xy[2];

  JWAbs(Self, xy);

  temp = JMnuInit(NULL, rootmenu, xy[0], xy[1]+16, handlemenu);
  JWinShow(temp);
}

int Josmod  = 0;
int Wavplay = 0;

void main() {
  void *Appl, *MainWindow, *temp;
  SizeHints sizes;

  Appl       = JAppInit(NULL, 0);
  MainWindow = JWndInit(NULL, "Launch it!", 0);
  JAppSetMain(Appl, MainWindow);

  ((JCnt *)MainWindow)->Orient = JCntF_TopBottom;

  JWSetBounds(MainWindow, 0,0, 48, 24);

  GoButton = JButInit(NULL, "Go WiNGS!");
  TxtBar   = JTxfInit(NULL);

  JWinCallback(GoButton, JBut, Clicked, ClickHandler);
  JWinCallback(TxtBar, JTxf, Entered, DoCommand);

  JCntAdd(MainWindow, GoButton);
  JCntAdd(MainWindow, TxtBar);

  JWinShow(MainWindow);

  retexit(1);

  JAppLoop(Appl);
}

void DoCommand(){
  system(JTxfGetText(TxtBar));
}

void RunJosmod() {
  if(Josmod != 0){
    spawnlp(0, "kill", Josmod, NULL);
    Josmod=0;
  } else 
    Josmod = spawnlp(0, "josmod", "-h", "11000", JTxfGetText(TxtBar), NULL);
}

void RunWavplay() {
  if(Wavplay != 0){
    spawnlp(0, "kill", Wavplay, NULL);
    Wavplay=0;
  } else
    Wavplay = spawnlp(0, "wavplay", JTxfGetText(TxtBar), NULL);
}

void gunzip() {
  system(buf);
}
