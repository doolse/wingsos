#include <winlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <wgslib.h>
#include <stdlib.h>

void *txtbar, *display, *mainwin;
char *param;
int total = 0;

void beginsearch();
void search();
void startthread() {
  newThread(beginsearch, 1024, NULL); 
} 

void quitapp() {
  exit(EXIT_SUCCESS);
}

void main(int argc, char * argv[]) {
  void *appl, *searchbut, *exitbut, *inputcnt;
  int ch = 0;

  appl    = JAppInit(NULL,0);
  mainwin = JWndInit(NULL, "Search", JWndF_Resizable);

  ((JCnt*)mainwin)->Orient = JCntF_Vert;
  inputcnt = JCntInit(NULL);
  display  = JTxtInit(NULL);

  JCntAdd(mainwin, inputcnt);
  JCntAdd(mainwin, display);

  txtbar = JTxfInit(NULL);
  JWinCallback(txtbar, JTxf, Entered, startthread);

  searchbut = JButInit(NULL, "Search"); 
  JWinCallback(searchbut, JBut, Clicked, startthread);  

  exitbut = JButInit(NULL, "Exit"); 
  JWinCallback(exitbut, JBut, Clicked, quitapp);  

  JCntAdd(inputcnt, txtbar);
  JCntAdd(inputcnt, searchbut);
  JCntAdd(inputcnt, exitbut);

  JAppSetMain(appl, mainwin);
  JWinShow(mainwin);

  //QUIT READING MY GODDAMN PACKETS GRAHAM. :) THX.

  retexit(1);

  JAppLoop(appl);
} 

void beginsearch() {
  char * matches;

  JWKill(display);

  display = JTxtInit(NULL);
  JCntAdd(mainwin, display);

  param = strdup(JTxfGetText(txtbar));

  if(strlen(param) < 1) {
    JTxtAppend(display, "You must enter a search parameter.\n");
  } else {
    search("/");
    JTxtAppend(display, "Search Complete");
    matches = (char *)malloc(strlen(" (  matches found)"), + 20);
    sprintf(matches, " (%d matches found)\n", total);
    JTxtAppend(display, matches);
    total = 0;
  }
  free(param);
}

void search(char * dirpath) {
  DIR *dir;
  struct dirent *entry;
  struct stat buf;
  char * fullname = NULL;
  char * nextdir  = NULL;

  dir = opendir(dirpath);

  if(dir != NULL) {
    while(entry = readdir(dir)) {
      if(strstr(entry->d_name, param)) {
        total++;
        JTxtAppend(display, dirpath);
        JTxtAppend(display, entry->d_name);
        JTxtAppend(display, "\n");
      }

      if(entry->d_type == 6) {
        nextdir = (char *)malloc(strlen(dirpath) + strlen(entry->d_name) +3);
        sprintf(nextdir, "%s%s/", dirpath, entry->d_name);

        search(nextdir);
        free(nextdir);
      }
    }
    closedir(dir);
  }
}
