#include <winlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <wgslib.h>
#include <stdlib.h>
#include <unistd.h>

#define EXIT 1
#define NORESIZE 0

void dict(); 
void thes();   
void clear();
void trans(void * widget);
void resize(void *widget);

char * urlencode(char * text);
void cutstr(char * text) {
  *strchr(text, ' ') = 0; 
}

void * TxtBar, * mainwin;

/*
void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs);
void handlemenu(void *Self, MenuData *item);

MenuData mainmenu[] = {
  {"WordServices v1.1", 0, NULL, 0, 0,    NULL, NULL},
  {"Exit",              0, NULL, 0, EXIT, NULL, NULL},
  {NULL,                0, NULL, 0, 0,    NULL, NULL}
};
*/

void main () {
  void *app;
  void *dictbut, *thesbut, *tranbut; 

  void *statictext, *checkbox;

  void *topcnt, *botcnt, *sttxtcnt, *butcnt, *dictthescnt;

  app     = JAppInit(NULL, 0);
  mainwin = JWndInit(NULL, "WordServices v1.1", 0);
  ((JCnt *)mainwin)->Orient = JCntF_Vert;

  topcnt = JCntInit(NULL);
  botcnt = JCntInit(NULL);

  JCntAdd(mainwin, topcnt);
  JCntAdd(mainwin, botcnt);

  sttxtcnt = JCntInit(NULL);
  ((JCnt *)sttxtcnt)->Orient = JCntF_Vert;

  butcnt = JCntInit(NULL);
  ((JCnt *)butcnt)->Orient = JCntF_Vert;

  JCntAdd(botcnt, sttxtcnt);
  JCntAdd(botcnt, butcnt);

  //JWinCallback(mainwin, JWnd, RightClick, RightBut);
  JWSetBounds(mainwin, 50, 50, 120, 80);

  TxtBar = JTxfInit(NULL);
  JCntAdd(topcnt, TxtBar);

  statictext = JStxInit(NULL, "Lookup in:");
  JCntAdd(sttxtcnt, statictext);

  statictext = JFilInit(NULL, 0);
  JCntAdd(sttxtcnt, statictext);

  statictext = JStxInit(NULL, "Translate:");
  JCntAdd(sttxtcnt, statictext);

  dictthescnt = JCntInit(NULL);
  JCntAdd(butcnt, dictthescnt);

  dictbut = JButInit(NULL, "Dictionary");
  JWinCallback(dictbut, JBut, Clicked, dict);
  JCntAdd(dictthescnt, dictbut);

  thesbut = JButInit(NULL, "Thesaurus");
  JWinCallback(thesbut, JBut, Clicked, thes);
  JCntAdd(dictthescnt, thesbut);

  tranbut = JButInit(NULL, "English to German");
  JWSetData(tranbut, "en_ge");
  JWinCallback(tranbut, JBut, Clicked, trans);

  JCntAdd(butcnt, tranbut);

  tranbut = JButInit(NULL, "English to French ");
  JWSetData(tranbut, "en_fr");
  JWinCallback(tranbut, JBut, Clicked, trans);

  JCntAdd(butcnt, tranbut);

  tranbut = JButInit(NULL, "English to Italian ");
  JWSetData(tranbut, "en_it");
  JWinCallback(tranbut, JBut, Clicked, trans);

  JCntAdd(butcnt, tranbut);

  tranbut = JButInit(NULL, "X");
  JWinCallback(tranbut, JBut, Clicked, clear);

  JCntAdd(topcnt, tranbut);

  JAppSetMain(app, mainwin);
  JWinShow(mainwin);

  retexit(1);

  JAppLoop(app);

}

/*
void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs) {
  void *temp = NULL;
  if(Type == EVS_But2Up) {
    temp = JMnuInit(NULL, mainmenu, XAbs, YAbs, handlemenu);
    JWinShow(temp);
  }
}

void handlemenu(void *Self, MenuData *item) {
  switch(item->command) {
    case EXIT:
      exit(1);
    break;
  }
}
*/

void clear() {
  JTxfSetText(TxtBar, "");
}

void trans(void * widget){
  FILE *fp, *thepipe;
  char *textstr, *lang, *buf;
  int apipe[2];
  int size, times;

  buf = NULL;
  size = 0;
  times = 0;

  if(strcmp(JTxfGetText(TxtBar), "")) {

  textstr = urlencode(JTxfGetText(TxtBar));
  lang    = (char *)JWGetData(widget);

  fp = fopen("/dev/tcp/dictionary.reference.com:80", "r+");
  
  if(fp) {

    fflush(fp);
    fprintf(fp, "POST /translate/text.html HTTP/1.0\n");

    fprintf(fp, "Content-Type: application/x-www-form-urlencoded\n");
    fprintf(fp, "Content-Length: %d\n\n", strlen("lp=&text=")+strlen(lang)+strlen(textstr)+1);

    fprintf(fp, "lp=%s&text=%s\n", lang, textstr);

    fflush(fp);

    pipe(apipe);

    redir(apipe[0], STDIN_FILENO);
      spawnlp(0, "guitext", "-w", "300", "-h", "150", NULL);
    close(apipe[0]);

    thepipe = fdopen(apipe[1], "w");

    fprintf(thepipe, "You Wrote: \"%s\"\n\n", JTxfGetText(TxtBar));
    fprintf(thepipe, "The Translation is: \""); 

    while((getline(&buf, &size, fp)) != EOF) {
      if(!strncasecmp(buf, "<h2>in ", 7)) {
        times++;
        if(times == 2) {
          while(times != 4) {
            getline(&buf, &size, fp);
            if((!strncasecmp(buf, "<p>", 3)) && (strstr(buf, "</p>"))) {
              times = 4;
              buf[strlen(buf)-5] = 0;
              fprintf(thepipe, "%s", &buf[3]);
            } else if(!strncasecmp(buf, "<p>", 3)) {
              buf[strlen(buf)-1] = 0;
              fprintf(thepipe, "%s", &buf[3]);
            } else if(strstr(buf, "</p>")) {
              times = 4;
              buf[strlen(buf)-5] = 0;
              fprintf(thepipe, "%s", buf);
            } else {
              buf[strlen(buf)-1] = 0;
              fprintf(thepipe, "%s", buf);
            }
          }
          break;
        }
      }
    }

    fprintf(thepipe, "\"");

    fclose(fp);
    fclose(thepipe);
  }
  }
}

char * urlencode(char * text) {
  char * buf, * ptr;
  int i, len;
  
  len = 0;
  ptr = text;

  for(i=0; i<strlen(text); i++) {
    if(*ptr == ' '||*ptr == '"'||*ptr == '&'||*ptr == '?'||*ptr == '=')
      len += 2;
    ptr++;
  }
  buf = (char *)malloc(strlen(text)+len+2);
  memset(buf, 0, strlen(text)+len+2);

  ptr = text;

  for(i=0; i<strlen(text); i++) {
    switch (*ptr) {
      case '=':
        sprintf(buf, "%s%%3D", buf);
      break;
      case '?':
        sprintf(buf, "%s%%3F", buf);
      break;
      case '"':
        sprintf(buf, "%s%%22", buf);
      break;
      case '&':
        sprintf(buf, "%s%%26", buf);
      break;
      case ' ':
        sprintf(buf, "%s%%20", buf);
      break;
      default:
        sprintf(buf, "%s%c", buf, *ptr);
    }
    ptr++;
  }
  return(strdup(buf));
}

void dict(){
  char * buf;
  char * text;

  if(strcmp(JTxfGetText(TxtBar), "")) {
  text = strdup(JTxfGetText(TxtBar));
  cutstr(text);

  buf = (char *)malloc(strlen(text) + strlen("dict |guitext &")+1);

  sprintf(buf, "dict %s|guitext &", text);
  system(buf);
  }
}

void thes(){
  char * buf;
  char * text;

  if(strcmp(JTxfGetText(TxtBar), "")) {
  text = strdup(JTxfGetText(TxtBar));
  cutstr(text);

  buf = (char *)malloc(strlen(text) + strlen("thes |guitext &")+1);

  sprintf(buf, "thes %s|guitext &", text);
  system(buf);
  }
}
