#include <winlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <wgslib.h>
#include <stdlib.h>
#include <unistd.h>

#define EXIT 1

void dict(); 
void thes();   
void trans(void * widget);

char * urlencode(char * text);
void cutstr(char * text) {
  *strchr(text, ' ') = 0; 
}

void * TxtBar;

void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs);
void handlemenu(void *Self, MenuData *item);

MenuData mainmenu[] = {
  {"Greg/DAC in 2002", 0, NULL, 0, 0,    NULL, NULL},
  {"Exit",             0, NULL, 0, EXIT, NULL, NULL},
  {NULL,               0, NULL, 0, 0,    NULL, NULL}
};

void main () {
  void *window, *app;
  void *dictbut, *thesbut;
  void *trbut; 

  void *statictext;

  app    = JAppInit(NULL, 0);
  window = JWndInit(NULL, NULL, 0, "WordServices -Greg/DAC-", 0);

  JWinCallBack(window, JWnd, RightClick, RightBut);
  JWinSize(window, 128, 80);

  TxtBar = JTxfInit(NULL, window, 0, "");
  JWinGeom(TxtBar, 0, 0, 0, 16, GEOM_TopLeft | GEOM_TopRight2);

  statictext = JStxInit(NULL, window, 0, "Lookup in:", 1);
  JWinGeom(statictext, 0, 16, 50, 24, GEOM_TopLeft | GEOM_TopLeft2);

  statictext = JStxInit(NULL, window, 0, "Translate:", 1);
  JWinGeom(statictext, 0, 32, 50, 40, GEOM_TopLeft | GEOM_TopLeft2);

  dictbut = JButInit(NULL, window, 0, "Dictionary");
  JWinMove(dictbut, 90, 16, GEOM_TopRight);
  JWinCallBack(dictbut, JBut, Clicked, dict);

  thesbut = JButInit(NULL, window, 0, "Thesaurus");
  JWinMove(thesbut, 50, 16, GEOM_TopRight);
  JWinCallBack(thesbut, JBut, Clicked, thes);

  trbut = JButInit(NULL, window, 0, "English to German");
  JWinMove(trbut, 50, 32, GEOM_TopLeft);
  JWinSetData(trbut, "eng_ger");
  JWinOveride(trbut, JBut, Clicked, trans);

  trbut = JButInit(NULL, window, 0, "English to French");
  JWinMove(trbut, 50, 32, GEOM_TopLeft);
  JWinSetData(trbut, "eng_fre");
  JWinOveride(trbut, JBut, Clicked, trans);

  trbut = JButInit(NULL, window, 0, "English to Italian");
  JWinMove(trbut, 50, 32, GEOM_TopLeft);
  JWinSetData(trbut, "eng_ita");
  JWinOveride(trbut, JBut, Clicked, trans);

  JAppSetMain(app, window);
  JWinShow(window);
  JAppLoop(app);

}

void RightBut(void *Self, int Type, int X, int Y, int XAbs, int YAbs) {
  void *temp = NULL;
  if(Type == EVS_But2Up) {
    temp = JMnuInit(NULL, NULL, mainmenu, XAbs, YAbs, handlemenu);
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

void trans(void * widget){
  FILE *fp, *thepipe;
  char *textstr, *lang, *buf;
  int apipe[2];
  int size;

  buf = NULL;
  size = 0;

  textstr = urlencode(JTxfGetText(TxtBar));
  lang    = JWinGetData(widget);

  fp = fopen("/dev/tcp/babelfish.altavista.com:80", "r+");
  
  if(fp) {

    fflush(fp);
    fprintf(fp, "GET /tr?action=/tr&tt=urltext&lp=%s&urltext=%s&doit=done HTTP/1.0\n", lang, textstr);
    fprintf(fp, "Host: babelfish.altavista.com\n\n");

    fflush(fp);

    pipe(apipe);

    redir(apipe[0], STDIN_FILENO);
      spawnlp(0, "guitext", "-w", "300", "-h", "150", NULL);
    close(apipe[0]);

    thepipe = fdopen(apipe[1], "w");

    while((getline(&buf, &size, fp)) != EOF) 
      fprintf(thepipe, "%s", buf);

    fclose(fp);
    fclose(thepipe);
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
  buf = (char *)malloc(strlen(text)+len+1);
  memset(buf, 0, strlen(text)+len+1);

  ptr = text;

  for(i=0; i<strlen(text); i++) {
    switch (*ptr) {
      case '=':
        sprintf(buf, "%s%3D", buf);
      break;
      case '?':
        sprintf(buf, "%s%3F", buf);
      break;
      case '"':
        sprintf(buf, "%s%22", buf);
      break;
      case '&':
        sprintf(buf, "%s%26", buf);
      break;
      case ' ':
        sprintf(buf, "%s%20", buf);
      break;
      default:
        sprintf(buf, "%s%c", buf, *ptr);
    }
    ptr++;
  }
  return(buf);
}

void dict(){
  char * buf;
  char * text;

  text = strdup(JTxfGetText(TxtBar));
  cutstr(text);

  buf = (char *)malloc(strlen(text) + strlen("dict |guitext &")+1);

  sprintf(buf, "dict %s|guitext &", text);
  system(buf);
}

void thes(){
  char * buf;
  char * text;

  text = strdup(JTxfGetText(TxtBar));
  cutstr(text);

  buf = (char *)malloc(strlen(text) + strlen("thes |guitext &")+1);

  sprintf(buf, "thes %s|guitext &", text);
  system(buf);
}
