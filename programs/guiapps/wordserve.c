#include <winlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <wgslib.h>
#include <stdlib.h>
#include <unistd.h>

#define EXIT 1
#define NORESIZE 0

unsigned char app_icon[] = {
31,32,82,90,158,158,136,142,
128,66,37,50,112,48,16,16,
70,36,31,2,4,8,7,0,
32,76,158,18,63,243,51,0,
0x01,0x01,0x01,0x01
};

int pipe1[2];
int pipe2[2];
char * transcode;
JLModel * trans_m;

typedef struct trans_s {
  TNode tnode; 
  char * label;
  char * transcode;
} trans_s;

unsigned char clearx[] = {
60,66,165,153,153,165,66,60,
0xf0
};

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
  buf = calloc(strlen(text)+len+2,1);
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
  return(buf);
}

void translate(void * button) {
  void * textarea, * textbar;
  FILE *fp,*fp2;
  char *text, *buf = NULL;
  int size = 0, times = 0;

  textarea = JWGetData(button);
  textbar  = JWGetData(textarea);

  JTxtClear(textarea);

  if(!strcmp(JTxfGetText(textbar), ""))
    return;
  if(!transcode)
    return;

  text = urlencode(JTxfGetText(textbar));
  
  if(!(fp = fopen("/dev/tcp/dictionary.reference.com:80", "r+"))) {
    JTxtAppend(textarea,"Unable to connect to dictionary.com's transalation server.\n");
    return;
  }

  fflush(fp);
  fprintf(fp, "POST /translate/text.html HTTP/1.0\n");
  fprintf(fp, "Content-Type: application/x-www-form-urlencoded\n");
  fprintf(fp, "Content-Length: %d\n\n", strlen("lp=&text=")+strlen(transcode)+strlen(text)+1);

  fprintf(fp, "lp=%s&text=%s\n", transcode, text);
  fflush(fp);

  while((getline(&buf, &size, fp)) != EOF) {
    if(!strncasecmp(buf, "<h2>in ", strlen("<h2>in "))) {
      times++;
      if(times == 2) {
        while(times != 4) {
          getline(&buf, &size, fp);
          if((!strncasecmp(buf, "<p>", strlen("<p>"))) && (strstr(buf, "</p>"))) {
            times = 4;
            buf[strlen(buf)-5] = 0;
            JTxtAppend(textarea,&buf[3]);
          } else if(!strncasecmp(buf, "<p>", strlen("<p>"))) {
            buf[strlen(buf)-1] = 0;
            JTxtAppend(textarea,&buf[3]);
          } else if(strstr(buf, "</p>")) {
            times = 4;
            buf[strlen(buf)-5] = 0;
            JTxtAppend(textarea,buf);
          } else {
            buf[strlen(buf)-1] = 0;
            JTxtAppend(textarea,buf);
          }
        }
        break;
      }
    }
  }
  fclose(fp);
}

void changetranscode(TNode *tnode) {
  trans_s * trans = (trans_s *)tnode;
  transcode = trans->transcode;
}

void clearbar(void *clearbut) {
  void * textbar = JWGetData(clearbut);
  JTxfSetText(textbar, "");
  JWReqFocus(textbar);
}

void writetopipe(FILE * fpin) {
  FILE *fpout;
  int ch;

  fpout = fdopen(pipe1[1], "w");
  while ((ch = fgetc(fpin)) != EOF) 
    fputc(ch, fpout);
  fclose(fpout);
}

void dict(void * button){
  int ch, header = 3, size = 0;
  FILE *fp,*fp2;
  void * textarea, * textbar;
  char * text, * buf = NULL;

  textarea = JWGetData(button);
  textbar  = JWGetData(textarea);

  JTxtClear(textarea);

  if(!strcmp(JTxfGetText(textbar), ""))
    return;
  if(!strcmp( ((JBut*)button)->Label,"Thes") ) {
    if(!(fp = fopen("/dev/tcp/thesaurus.reference.com:80","r+"))) {
      JTxtAppend(textarea,"Unable to open connection to thesaurus.com\n");
      return;
    }
  } else {
    if(!(fp = fopen("/dev/tcp/dictionary.reference.com:80","r+"))) {
      JTxtAppend(textarea,"Unable to open connection to dictionary.com\n");
      return;
    }
  }

  text = urlencode(JTxfGetText(textbar));

  fprintf(fp, "GET /search?q=%s HTTP/1.0\n\n", text);
  fflush(fp);

  pipe(pipe1);
  pipe(pipe2);
  redir(pipe1[0], STDIN_FILENO);
  redir(pipe2[1], STDOUT_FILENO);

  spawnlp(0, "web", NULL);

  close(pipe1[0]);
  close(pipe2[1]);

  newThread(writetopipe,STACK_DFL,fp);

  fp2 = fdopen(pipe2[0], "r");
  while((getline(&buf, &size, fp2)) != EOF) {
    if(header) {
      if(!strcmp(buf,"\n"))
        header--;
    } else {
      if(!strncmp(buf,"Perform a new search,",strlen("Perform a new search,")))
        break;
      if(strncmp(buf,"ADVERTISEMENT",strlen("ADVERTISEMENT")))
        JTxtAppend(textarea,buf); 
    }
  }
  fclose(fp2);
  fclose(fp);
}

void lyrics(void * button){
  int ch, open, i,j, header = 2, size = 0;
  FILE *fp,*fp2;
  void * textarea, * textbar;
  char * text, *lcode = NULL, * buf = NULL;

  textarea = JWGetData(button);
  textbar  = JWGetData(textarea);

  JTxtClear(textarea);

  if(!strcmp(JTxfGetText(textbar), ""))
    return;
  if(!(fp = fopen("/dev/tcp/www.purelyrics.com:80","r+"))) {
    JTxtAppend(textarea,"Unable to open connection to purelyrics.com\n");
    return;
  }

  text = urlencode(JTxfGetText(textbar));

  fprintf(fp, "GET /index.php?search_string2=%s HTTP/1.0\n\n", text);
  free(text);
  fflush(fp);

  pipe(pipe1);
  pipe(pipe2);
  redir(pipe1[0], STDIN_FILENO);
  redir(pipe2[1], STDOUT_FILENO);

  spawnlp(0, "web", NULL);

  close(pipe1[0]);
  close(pipe2[1]);

  newThread(writetopipe,STACK_DFL,fp);

  fp2 = fdopen(pipe2[0], "r");
  while((getline(&buf, &size, fp2)) != EOF) {
    if(header) {
      if(!strcmp(buf,"\n"))
        header--;
    } else {
      if(!strcmp(buf,"\n"))
        break;
      if(lcode = strstr(buf,"Location:")) {
        lcode = strdup(lcode+strlen("Location: "));
        lcode[strlen(lcode)-1] = 0;
        break;
      }
    }
  }
  header = 2;

  fclose(fp2);
  fclose(fp);

  if(!lcode || strstr(lcode,"search")) {
    JTxtAppend(textarea,"A unique song matching your search criteria could not be found.  Try changing the search terms to include the name of the band.\n");
    return;
  }

  if(!(fp = fopen("/dev/tcp/www.purelyrics.com:80","r+"))) {
    JTxtAppend(textarea,"Unable to open connection to purelyrics.com\n");
    return;
  }

  fprintf(fp, "GET /%s HTTP/1.0\n\n", lcode);
  free(lcode);
  fflush(fp);

  open = 0;
  while((getline(&buf, &size, fp)) != EOF) {
    if(header) {
      if(strstr(buf,"http://www.purelyrics.com/img/lyricsbg.gif"))
        header = 0;
    } else {
      if(strstr(buf,"if (typeof topbar_banner"))
        break;
      text = calloc(strlen(buf),1);
      lcode = buf;
      j=0;
      for(i=0;i<strlen(buf);i++) {
        if(!open) {
          if(*lcode == '<') {
            open = 1;
            if(lcode[1] == 'b' && lcode[2] == 'r')
              text[j++] = '\n';
          } else if(*lcode != '\n' && *lcode != '\r') 
            text[j++] = *lcode;
        } else {
          if(*lcode == '>')
            open = 0;
        }
        lcode++;
      }
      JTxtAppend(textarea,text);
      free(text);
    }
  }
  fclose(fp2);
  fclose(fp);
}

void createtranscodes() {
  trans_s * transitem;
  int i;
  char * textlabels[] = {"English to German",
                         "English to French",
                         "English to Italian",
                         "English to Dutch",
                         "English to Portuguese",
                         "English to Spanish",
                         "German to English",
                         "French to English",
                         "Italian to English",
                         "Dutch to English",
                         "Portuguese to English",
                         "Spanish to English"};

  char * transcodes[] = {"en_ge","en_fr","en_it","en_nl","en_pt","en_es",
                         "ge_en","fr_en","it_en","nl_en","pt_en","es_en"};

  trans_m = JLModelInit(NULL);

  for(i=0;i<12;i++) {
    transitem = calloc(sizeof(trans_s),1);
    transitem->label     = strdup(textlabels[i]);
    transitem->transcode = strdup(transcodes[i]);
    JLModelAppend(trans_m,(TNode *)transitem);
  }
}

void saveas(void * button) {
  FILE * outfp;
  void * textbar  = JWGetData(button);
  void * textarea = JWGetData(textbar);
  Piece * pptr = ((JTxt*)textarea)->LineTab->PiecePtr;
  char * filename = JTxfGetText(textbar);

  if(strlen(filename) == 0)
    filename = strdup("wordserve.txt");
  else if(strlen(filename) > 16)
    filename[16] = 0;

  outfp = fopen(filename,"w");
  if(outfp) {
    fprintf(outfp,"%s",pptr->buffer);
    while(!pptr->Last) {
      pptr = pptr->Next;
      fprintf(outfp,"%s",pptr->buffer);
    }
    fclose(outfp);
  }
}

void main(int argc, char * argv[]) {
  void *app, *mainwin;
  void *textbar, *textbar2, *textarea, *tempbut, *tempcnt, *scrarea;
  void *ibutclass;
  JCombo * selbox;
  JMeta * metadata = malloc(sizeof(JMeta));

  metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
  metadata->title = "WordServices v2.0";
  metadata->icon = app_icon;
  metadata->showicon = 1;
  metadata->parentreg = -1;

  transcode = NULL;

  app     = JAppInit(NULL, 0);
  mainwin = JWndInit(NULL, metadata->title, JWndF_Resizable,metadata);
  ((JCnt *)mainwin)->Orient = JCntF_TopBottom;
  JWSetBounds(mainwin, 50, 50, 184, 80);
  JWSetMin(mainwin,184,80);
  JWSetMax(mainwin,184,160);
  JWndSetProp(mainwin);

  tempcnt = JCntInit(NULL);
  ((JCnt *)tempcnt)->Orient = JCntF_LeftRight;
  JCntAdd(mainwin, tempcnt);

  textbar = JTxfInit(NULL);
  JCntAdd(tempcnt, textbar);
  JWSetPen(textbar,COL_White);
  JWSetBack(textbar,COL_Black);

  ibutclass = JSubclass(&JBmpClass,-1,
                          METHOD(MJW, Button), clearbar,
                          -1);  

  tempbut = JNew(ibutclass);
  JBmpInit(tempbut,8,8,clearx);
  ((JW*)tempbut)->Sense |= WEV_Button;

  JWSetData(tempbut,textbar);
  JCntAdd(tempcnt,tempbut);

  tempcnt = JCntInit(NULL);
  ((JCnt *)tempcnt)->Orient = JCntF_LeftRight;
  JCntAdd(mainwin,tempcnt);

  createtranscodes();

  selbox = JComboInit(NULL,(TModel*)trans_m,OFFSET32(trans_s,label),JColF_STRING);
  selbox->Changed = changetranscode;
  JCntAdd(tempcnt,selbox);

  textarea = JTxtInit(NULL);
  JWSetPen(textarea,COL_White);
  JWSetBack(textarea,COL_Blue);

  tempbut = JButInit(NULL,"Trans");
  JWSetData(tempbut,textarea);
  JCntAdd(tempcnt,tempbut);
  JWinCallback(tempbut,JBut,Clicked,translate);

  tempbut = JButInit(NULL,"Dict");
  JWSetData(tempbut,textarea);
  JCntAdd(tempcnt,tempbut);
  JWinCallback(tempbut,JBut,Clicked,dict);

  tempbut = JButInit(NULL,"Thes");
  JWSetData(tempbut,textarea);
  JCntAdd(tempcnt,tempbut);
  JWinCallback(tempbut,JBut,Clicked,dict);

  tempbut = JButInit(NULL,"Lyrics");
  JWSetData(tempbut,textarea);
  JCntAdd(tempcnt,tempbut);
  JWinCallback(tempbut,JBut,Clicked,lyrics);

  tempcnt = JCntInit(NULL);
  ((JCnt *)tempcnt)->Orient = JCntF_LeftRight;
  JCntAdd(mainwin,tempcnt);

  scrarea = JScrInit(NULL,textarea,JScrF_VNotEnd|JScrF_HNotEnd);

  JCntAdd(tempcnt,scrarea);
  JWSetData(textarea,textbar);

  tempcnt = JCntInit(NULL);
  ((JCnt *)tempcnt)->Orient = JCntF_LeftRight;
  JCntAdd(mainwin, tempcnt);

  textbar2 = JTxfInit(NULL);
  JWSetAll(textbar2,320,16);
  JWSetPen(textbar2,COL_White);
  JWSetBack(textbar2,COL_Black);
  JWSetData(textbar2,textarea);

  tempbut = JButInit(NULL,"Save As...");
  JWSetData(tempbut,textbar2);
  JWinCallback(tempbut,JBut,Clicked,saveas);

  JCntAdd(tempcnt,tempbut);
  JCntAdd(tempcnt,textbar2);

  JWReqFocus(textbar);

  JAppSetMain(app, mainwin);
  JWinShow(mainwin);

  retexit(1);

  JAppLoop(app);

}
