#include <console.h>
#include <dirent.h>
#include <exception.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termio.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <xmldom.h>

#define sid ((uchar *)0xd400)
#define FALSE 0
#define TRUE 1

#define FILENAME 0
#define FILESIZE 1
#define FILETYPE 2

typedef struct direntry_s {
  struct direntry_s * next;
  struct direntry_s * prev;

  char * filename;
  int tag;
  int parent;
  char * filetype;
  long filesize;  
  char * datestr;
} direntry;

typedef struct panel_s {
  int firstrow;
  int cursoroffset;
  int totalnumofrows;

  char * path;

  char * prevpath;
  int inimage;

  int sortby;

  direntry * headdirent;
  direntry * direntptr;
} panel;

direntry * tempdirent, * tempdirent2;

panel * toppanel, * botpanel, * activepanel;

int cbmfsloaded;

char * VERSION = "1.4";
int showhidden = 0;
struct tm g_time;

int MainFG = COL_White;
int MainBG = COL_Blue;

int globaltioflags;
struct wmutex pause = {-1,-1};

struct termios tio;

DOMElement * filetypes_rootnode, * filetypes;

void prepconsole();
void drawpanel(panel * thepan,int redraw);

void con_reset() {
  printf("\x1b[0m");
}

void resetsid() {
  int i;
  for(i = 0; i<0x20; i++) 
    sid[i] = 0;
}

char * getdate(time_t date) {
  struct tm * tmtime;
  char * tmprint = "%b %d %Y";
  static char datestr[32];

  tmtime = gmtime(&date);
  if(tmtime->tm_year == g_time.tm_year) 
    tmprint = "%b %d %H:%M";

  strftime(datestr, sizeof(datestr), tmprint, tmtime);

  return(datestr);
}

void showhelp() {
  FILE * helpfp;
  int x,y,size = 0;
  char * buf = NULL;
  
  y = 1;
  x = 0;
  
  helpfp = fopen(fpathname("help.txt",getappdir(),1),"r");
  if(!helpfp) {
    drawmessagebox("Error: Help file missing.","Press a key.",1);
    return;
  }
  con_setfgbg(COL_Blue,COL_Blue);
  while(getline(&buf,&size,helpfp) != EOF) {
    buf[strlen(buf)-1] = 0;
    if(!x) 
      x = (con_xsize - strlen(buf))/2;
    con_gotoxy(x,y++);
    printf("%s",buf);
  }
  fclose(helpfp);
  con_update();
  con_getkey();
}
  
void movechardown(int x, int y, char c){
  con_gotoxy(x, y);
  putchar(' ');
  con_gotoxy(x, y+1);
  putchar(c);   
}   

void movecharup(int x, int y, char c){
  con_gotoxy(x, y);
  putchar(' ');
  con_gotoxy(x, y-1);
  putchar(c);  
}

int checkcbmfsys() {
  struct PSInfo APS;
  int prevpid = 1;
  
  do {
    prevpid =  getPSInfo(prevpid, &APS);
    if(!strcmp(APS.Name, "cbmfsys.drv     "))
      return(1);
  } while(prevpid);
  return(0);
}

void loadcbmfsys() {
  if(!cbmfsloaded) {
    spawnlp(S_WAIT, "cbmfsys.drv", NULL);
    cbmfsloaded++;
  }
}

void drawframe(char * message) {
  char * titlestr;
  int i, xpos, ypos;

  titlestr = malloc(81);
  sprintf(titlestr, " Console File Manager v%s - 2004 ", VERSION);


  //Draw title of top panel

  xpos = 0;
  ypos = 0;

  con_gotoxy(xpos,ypos);
  if(activepanel == toppanel)
    con_setfgbg(COL_Black,COL_Red);
  else
    con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  con_gotoxy((con_xsize - strlen(titlestr))/2,ypos);	
  printf("%s",titlestr);


  //Draw title of bottom panel

  ypos = botpanel->firstrow - 1;
  xpos = 0;

  con_gotoxy(xpos,ypos);
  if(activepanel == botpanel)
    con_setfgbg(COL_Black,COL_Red);
  else
    con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  con_gotoxy((con_xsize - strlen(message))/2, ypos);
  printf("%s",message);


  //Draw bottom menu

  con_gotoxy(0,botpanel->firstrow+botpanel->totalnumofrows+1);
  con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  printf(" (Q)uit, (c)opy, (m)ove, (n)ew dir, (r)ename, (?) Help");


  //reset draw cursor to standard colours

  con_setfgbg(MainFG,MainBG);
}

void clearpanel(panel * thepan) {
  int i;
  for(i = thepan->firstrow; i<thepan->firstrow+thepan->totalnumofrows; i++) {
    con_gotoxy(0,i);
    con_clrline(LC_End);   
  }
}

void setactivescrollregion() {
  con_setscroll(activepanel->firstrow, activepanel->firstrow+activepanel->totalnumofrows);
}

void panelchange() {
  con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
  putchar(' ');

  if(activepanel == toppanel)
    activepanel = botpanel;    
  else 
    activepanel = toppanel;

  drawframe("Welcome to the WiNGs File Manager");

  con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
  putchar('>');

  setactivescrollregion();
}

void drawpanelline(direntry * direntptr) {

  printf(" %c %16s ", direntptr->tag, direntptr->filename);

  if(direntptr->filesize > 1048576) 
    printf("%6ld mb    ", direntptr->filesize/1048576);
  else if(direntptr->filesize > 1024)
    printf("%6ld kb    ", direntptr->filesize/1024);
  else if(direntptr->filesize > 1)
    printf("%6ld bytes ", direntptr->filesize);
  else if(direntptr->filesize == 1)
    printf("%6ld byte  ", 1);
  else
    printf("             ");

  printf("%12s %s", direntptr->datestr,direntptr->filetype);
}

void drawpanel(panel * thepan, int redraw) {
  int i, active;
  direntry * dirptr;

  //redraw is used when refreshing the panel, when the panel
  //is scrolled down some from the top. 

  if(redraw) {
    dirptr = thepan->direntptr;
    for(i=thepan->cursoroffset;i>0;i--)
      dirptr = dirptr->prev;
  } else
    dirptr = thepan->headdirent;

  for(i=thepan->firstrow;i<thepan->firstrow+thepan->totalnumofrows;i++) {
    con_gotoxy(0,i);
    drawpanelline(dirptr);
    if(dirptr->next == thepan->headdirent)
      break;    
    dirptr = dirptr->next;
  }
}

int cmp(direntry *a, direntry *b,int type) {
  if(type == FILESIZE) {
    if(a->filesize > b->filesize)
      return(1);
    else if(a->filesize < b->filesize)
      return(-1);
    else
      return(0);

  } else if(type == FILETYPE)
    return(strcasecmp(a->filetype,b->filetype));

  else //FILENAME
    return(strcasecmp(a->filename,b->filename));
}

direntry *listsort(direntry *list, int sortby) {
  direntry *p, *q, *e, *tail, *oldhead;
  int insize, nmerges, psize, qsize, i;

  if (!list)
    return NULL;

  insize = 1;

  while (1) {
    p = list;
    oldhead = list;		       /* only used for circular linkage */
    list = NULL;
    tail = NULL;

    nmerges = 0;  /* count number of merges we do in this pass */

    while (p) {
      nmerges++;  /* there exists a merge to be done */

      /* step `insize' places along from p */

      q = p;
      psize = 0;

      for (i = 0; i < insize; i++) {
        psize++;
        q = (q->next == oldhead ? NULL : q->next);
        if (!q) 
          break;
      }

      /* if q hasn't fallen off end, we have two lists to merge */
      qsize = insize;

      /* now we have two lists; merge them */

      while (psize > 0 || (qsize > 0 && q)) {

        /* decide whether next element of merge comes from p or q */
        if (psize == 0) {
          /* p is empty; e must come from q. */
          e = q; 
          q = q->next; 
          qsize--;

          if (q == oldhead) 
            q = NULL;

        } else if (qsize == 0 || !q) {
          /* q is empty; e must come from p. */
	  e = p; 
          p = p->next; 
          psize--;

	  if (p == oldhead) 
            p = NULL;

        } else if (cmp(p,q,sortby) <= 0) {
          /* First element of p is lower (or same);
	   * e must come from p. */
	   e = p; 
           p = p->next; 
           psize--;

           if (p == oldhead) 
             p = NULL;

         } else {
           /* First element of q is lower; e must come from q. */
           e = q; 
           q = q->next; 
           qsize--;

           if (q == oldhead) 
             q = NULL;
         }

         /* add the next element to the merged list */
         if (tail) {
           tail->next = e;
         } else {
           list = e;
         }

         /* Maintain reverse pointers in a doubly linked list. */
         e->prev = tail;
         tail = e;
       }

       /* now p has stepped `insize' places along, and q has too */
       p = q;
    }
    tail->next = list;
    list->prev = tail;

    /* If we have done only one merge, we're finished. */
    if (nmerges <= 1)   /* allow for nmerges==0, the empty list case */
      return list;

    /* Otherwise repeat, merging lists twice the size */
    insize *= 2;
  }
  return list;
}

void builddir(panel * thepan) {
  char * fullname;
  int i, root;
  long filesize;
  DIR *dir;
  direntry * tempnode;
  struct dirent *entry;
  struct stat buf;
  DOMElement * filetypeptr;

  clearpanel(thepan);

  while(thepan->headdirent) {
    free(thepan->headdirent->filename);
    if(thepan->headdirent->filetype)
      free(thepan->headdirent->filetype);
    thepan->headdirent = remQueue(thepan->headdirent, thepan->headdirent);
  }

  if(strcmp(thepan->path,"/"))
    root = 0;
  else
    root = 1;

  thepan->cursoroffset = 0;

  dir = opendir(thepan->path);
  if(!dir) { 
    sprintf(thepan->path,"/");
    dir = opendir(thepan->path);
    if(!dir) {
      con_end();
      con_reset();
      con_clrscr();
      exit(EXIT_FAILURE);
    }
  }
  
  while(entry = readdir(dir)) {
    //Hide virtual directories
    if(root) {
      if(!strcmp(entry->d_name, "boot"))
        continue;
      if(!strcmp(entry->d_name, "dev"))
        continue;
      if(!strcmp(entry->d_name, "sys"))
        continue;
    } 

    if(entry->d_name[0] == '.' && showhidden == 0)
      continue;

    filesize = 10;
    tempnode = malloc(sizeof(direntry));
    thepan->headdirent = addQueueB(thepan->headdirent, thepan->headdirent, tempnode);
    tempnode->filename = strdup(entry->d_name);
    tempnode->parent = 0;

    if(!strcmp(&entry->d_name[strlen(entry->d_name)-4],".app")) {
      tempnode->filetype = strdup("Application");
      tempnode->filename[strlen(entry->d_name)-4] = 0; 
      filesize = 0;
    } else if(entry->d_type == 6) {
      tempnode->filetype = strdup("Directory");
      tempnode->filesize = 0;
    } else {
      tempnode->filetype = strdup(" ");
      filetypeptr = filetypes;
      do {
        if(!strcasecmp(&entry->d_name[strlen(entry->d_name)-strlen(XMLgetAttr(filetypeptr,"ext"))],XMLgetAttr(filetypeptr,"ext"))) {
          free(tempnode->filetype);
          tempnode->filetype = strdup(XMLgetAttr(filetypeptr,"type"));
          break;
        }
        filetypeptr = filetypeptr->NextElem;
      } while(filetypeptr != filetypes);
    }

    fullname = fpathname(entry->d_name, thepan->path, 1);
    stat(fullname, &buf);

    if(filesize == 10) {
      filesize = buf.st_size;
    }
    tempnode->filesize = filesize;
    tempnode->tag = ' ';

    tempnode->datestr = strdup(getdate(buf.st_mtime));
  }

  closedir(dir);  

  thepan->direntptr = thepan->headdirent = listsort(thepan->headdirent, thepan->sortby);

  if(!root) {
    tempnode = malloc(sizeof(direntry));
    tempnode->filename = strdup("Parent (../)");
    tempnode->tag      = ' ';
    tempnode->parent   = 1;
    tempnode->filetype = strdup("Directory");
    tempnode->filesize = 0;
    tempnode->datestr  = strdup("           ");

    thepan->headdirent = addQueueB(thepan->headdirent,thepan->headdirent,tempnode);
    thepan->direntptr = thepan->headdirent = thepan->headdirent->prev;
  }

  drawpanel(thepan,0);
}

void launch(panel * thepan, int text) {
  char * ext, * filename, * tempstr, *mountname, *tempstr2;
  char input;

  filename = (char *)malloc(17);
  tempstr  = (char *)malloc(strlen(thepan->path)+25);
  tempstr2 = NULL;

  sprintf(filename, "%s", thepan->direntptr->filename);
  sprintf(tempstr, "\"%s%s\"", thepan->path, filename);

  ext = &filename[strlen(filename)-4];

  if(!strcasecmp(ext, ".txt") || text) {
    con_end();
    //Rewrite the string sans outside quotes
    sprintf(tempstr, "%s%s", thepan->path, filename);
    getMutex(&pause);
    spawnlp(S_WAIT, "ned", tempstr, NULL);
    prepconsole();
    con_clrscr();
    drawframe("Welcome to the WiNGs Filemanager");
    clearpanel(toppanel);
    clearpanel(botpanel);
    drawpanel(toppanel,1);
    drawpanel(botpanel,1);
    relMutex(&pause);
  } else if(!strcasecmp(ext, ".wav")) {
    tempstr2 = (char *)malloc(strlen(tempstr) + strlen("wavplay  >/dev/null "));
    sprintf(tempstr2, "wavplay %s >/dev/null", tempstr);
    system(tempstr2);
  } else if(!strcasecmp(ext, ".mod") || !strcasecmp(ext, ".s3m") || !strcasecmp(&ext[1], ".xm")) {
    tempstr2 = (char *)malloc(strlen(tempstr) + strlen("josmod -h 11000  >/dev/null "));
    sprintf(tempstr2, "josmod -h 11000 %s >/dev/null", tempstr);
    system(tempstr2);
  } else if((!strcasecmp(ext, ".dat")) || (!strcasecmp(ext, ".sid"))) {
    tempstr2 = (char *)malloc(strlen(tempstr) + strlen("sidplay  >/dev/null "));
    sprintf(tempstr2, "sidplay %s >/dev/null", tempstr);
    system(tempstr2);
    resetsid();
  } else if(!strcasecmp(ext, ".zip") || !strcasecmp(&ext[1], ".gz")) {
    drawmessagebox("Are you sure you want to unzip this archive?","              (Y)es  or (n)o", 0);
    input = 0;
    while(input != 'Y' && input != 'n')
      input = con_getkey();
    if(input == 'Y') {
      drawmessagebox("Unzipping archive. Please wait.", "", 0);
      con_modeoff(TF_ECHO);
      tempstr2 = (char *)malloc(strlen(tempstr) + strlen("gunzip  2>/dev/null >/dev/null "));
      chdir(thepan->path);
      sprintf(tempstr2, "gunzip %s 2>/dev/null >/dev/null", tempstr);
      system(tempstr2);
      con_clrscr();
      con_modeon(TF_ECHO);
      builddir(toppanel);
      builddir(botpanel);
    } else {
      clearpanel(toppanel);
      drawpanel(toppanel,1);
      clearpanel(botpanel);
      drawpanel(botpanel,1);
    }
    drawframe("Welcome to the WiNGs Filemanager");
  } else if(!strcasecmp(ext, ".d64")) {
    loadcbmfsys();

    sprintf(tempstr, "%s%s", thepan->path, filename);

    mountname = malloc(25);
    sprintf(mountname, "/mount/%s/",filename);

    mount("/sys/fsys.1541", tempstr, mountname);

    thepan->prevpath = thepan->path;
    thepan->path = mountname;
    thepan->inimage = 1;
    builddir(thepan);

  } else if(!strcasecmp(ext, ".d81")) {
    loadcbmfsys();

    sprintf(tempstr, "%s%s", thepan->path, filename);

    mountname = malloc(25);
    sprintf(mountname, "/mount/%s/",filename);

    mount("/sys/fsys.1581", tempstr, mountname);

    thepan->prevpath = thepan->path;
    thepan->path = mountname;
    thepan->inimage = 1;
    builddir(thepan);
  }

  free(filename);
  free(tempstr);
  if(tempstr2)
    free(tempstr2);

  setactivescrollregion();
}

void prepconsole() {
  con_init();

  gettio(STDOUT_FILENO, &tio);
  tio.flags |= TF_ECHO|TF_ICRLF;
  tio.MIN = 1;  
  settio(STDOUT_FILENO, &tio);
}

void updatetime() {
  char date[64];
  char *MsgP;
  int Channel,RcvID;
  time_t curtime;

  Channel = makeChan();
  setTimer(-1,1000,0,Channel,PMSG_Alarm);
  while(1) {
    RcvID = recvMsg(Channel, (void *)&MsgP);
    getMutex(&pause);
    curtime = time(NULL);
    strftime(date, sizeof(date), "%c", localtime(&curtime));
    con_gotoxy(con_xsize-26,0);
    puts(date);
    con_gotoxy(1,activepanel->firstrow+activepanel->cursoroffset);
    con_update();
    replyMsg(RcvID,-1);
    relMutex(&pause);
    setTimer(-1,1000,0,Channel,PMSG_Alarm);    
  }
}

void renamefile() {
  int size;
  char * source, * dest, * mylinebuf;

  source = malloc(strlen(activepanel->path) + 26);
  dest   = malloc(strlen(activepanel->path) + 18);
  sprintf(source,"Rename: %s",activepanel->direntptr->filename);

  drawmessagebox(source, "                         ",0);

  if(!strcmp(activepanel->direntptr->filetype,"Application"))
    size = 12;
  else
    size = 16;

  mylinebuf = getmylinerestrict(strdup(activepanel->direntptr->filename),size,size,27,13,"/",0);

  if(strlen(mylinebuf)) {
    if(size == 12) {
      sprintf(source,"%s%s.app",activepanel->path,activepanel->direntptr->filename);
      sprintf(dest,"%s%s.app",activepanel->path,mylinebuf);
    } else {
      sprintf(source,"%s%s",activepanel->path,activepanel->direntptr->filename);
      sprintf(dest,"%s%s",activepanel->path,mylinebuf);
    }
    rename(source,dest);
  }

  free(source);
  free(dest);
  free(mylinebuf);
}

void copyfile() {
  char * source;
  panel * targetpan;

  source = malloc(strlen(activepanel->path) + strlen(activepanel->direntptr->filename)+1);

  sprintf(source,"%s%s",activepanel->path,activepanel->direntptr->filename);
  if(activepanel == toppanel)
    targetpan = botpanel;
  else 
    targetpan = toppanel;

  spawnlp(S_WAIT,"cp","-r","-f",source,targetpan->path,NULL);
}

void main() {
  FILE * fp, * laststatefp;
  int ex,input,i,j,size = 0;
  void * exp;
  char *tempstr, *tempstr2, *mylinebuf, *getbuf, *prevdir = NULL;
  direntry * tempnode;
  time_t thetime;
  DOMElement * laststatexmlroot, * xmlpanelstate;

  thetime = time(NULL);
  localtime_r(&thetime,&g_time);

  prepconsole();

  if(con_xsize < 80) {

    //Until the 40 column scrolling is implemented properly.

    printf("Fileman requires an 80 column console.\n");
    con_update();
    con_end();
    exit(1);

  }

  toppanel = (panel *)malloc(sizeof(panel)+1);
  botpanel = (panel *)malloc(sizeof(panel)+1);

  toppanel->path = malloc(1024);
  botpanel->path = malloc(1024);

  toppanel->totalnumofrows = (con_ysize - 3)/2;
  botpanel->totalnumofrows = (con_ysize - 3)/2;

  toppanel->firstrow = 1;
  botpanel->firstrow = toppanel->firstrow+toppanel->totalnumofrows+1;

  toppanel->headdirent = NULL;
  botpanel->headdirent = NULL;

  toppanel->inimage = 0;
  botpanel->inimage = 0;

  toppanel->sortby = FILENAME;
  botpanel->sortby = FILENAME;

  con_setfgbg(MainFG,MainBG);
  con_clrscr();
  
  filetypes_rootnode = XMLloadFile(fpathname("filetypes.xml",getappdir(), 1));
  filetypes = XMLgetNode(filetypes_rootnode, "/xml/filetype");

  Try {
    laststatexmlroot = XMLloadFile(fpathname("laststate.xml", getappdir(),1));
  }
  Catch2(ex,exp) {
    laststatefp = fopen(fpathname("laststate.xml", getappdir(),1), "w");
    fprintf(laststatefp,"<xml><toppanel path=%c/%c sortby=%c0%c numofrows=%c%d%c firstrow=%c1%c /><botpanel path=%c/%c sortby=%c0%c firstrow=%c%d%c numofrows=%c%d%c/></xml>", '"','"', '"','"', '"',(con_ysize-3)/2,'"','"','"','"','"','"','"', '"',toppanel->firstrow+toppanel->totalnumofrows+1,'"','"', (con_ysize-3)/2,'"');
    fclose(laststatefp);
    laststatexmlroot = XMLloadFile(fpathname("laststate.xml", getappdir(),1));
  }

  //printf("before: top: %s | %d | %d\nbot: %s | %d | %d\n", toppanel->path, toppanel->firstrow, toppanel->totalnumofrows, botpanel->path, botpanel->firstrow, botpanel->totalnumofrows);

  //Restore last state...
  
  xmlpanelstate = XMLgetNode(laststatexmlroot, "/xml/toppanel");
  sprintf(toppanel->path, XMLgetAttr(xmlpanelstate,"path"));
  toppanel->totalnumofrows = atoi(XMLgetAttr(xmlpanelstate,"numofrows"));
  toppanel->firstrow       = atoi(XMLgetAttr(xmlpanelstate,"firstrow"));
  toppanel->sortby         = atoi(XMLgetAttr(xmlpanelstate,"sortby"));

  xmlpanelstate = XMLgetNode(laststatexmlroot, "/xml/botpanel");
  sprintf(botpanel->path, XMLgetAttr(xmlpanelstate,"path"));
  botpanel->totalnumofrows = atoi(XMLgetAttr(xmlpanelstate,"numofrows"));  
  botpanel->firstrow       = atoi(XMLgetAttr(xmlpanelstate,"firstrow"));
  botpanel->sortby         = atoi(XMLgetAttr(xmlpanelstate,"sortby"));

  builddir(botpanel);
  builddir(toppanel);

  activepanel = toppanel;  

  drawframe("Welcome to the WiNGs File Manager");
  setactivescrollregion();
  con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
  putchar('>');
  con_update();

  //newThread(updatetime,STACK_DFL,NULL);

  cbmfsloaded = checkcbmfsys();

  input = 0;
  while(input != 'S' && input != 'Q') {
    input = con_getkey();

    forcekeypress:

    switch(input) {
      case CURD:
        if(activepanel->direntptr->next == activepanel->headdirent)
          break;
        if(activepanel->cursoroffset < (activepanel->totalnumofrows)-1) {
          movechardown(0,activepanel->firstrow+activepanel->cursoroffset, '>');
          activepanel->direntptr = activepanel->direntptr->next;
          activepanel->cursoroffset++;
        } else {
          putchar('\n');
          activepanel->direntptr = activepanel->direntptr->next;
          drawpanelline(activepanel->direntptr);
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset-1);
          putchar(' ');
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
          putchar('>');
        }                
      break;
      case CURU:
        if(activepanel->direntptr == activepanel->headdirent)
          break;
        if(activepanel->cursoroffset > 0) {
          movecharup(0,activepanel->firstrow+activepanel->cursoroffset, '>');
          activepanel->direntptr = activepanel->direntptr->prev;
          activepanel->cursoroffset--;
        } else {
          printf("\x1b[1L");
          con_gotoxy(0,activepanel->firstrow);
          activepanel->direntptr = activepanel->direntptr->prev;
          drawpanelline(activepanel->direntptr);
          con_gotoxy(0,activepanel->firstrow+1);
          putchar(' ');
          con_gotoxy(0,activepanel->firstrow);
          putchar('>');
        }
      break;

      case '1': 
        activepanel->sortby = FILENAME;
        builddir(activepanel);
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;
      case '2': 
        activepanel->sortby = FILESIZE;
        builddir(activepanel);
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;
      case '3': 
        activepanel->sortby = FILETYPE;
        builddir(activepanel);
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'D':
        drawmessagebox("Name for new D64 image:"," ",0);
        mylinebuf = getmylinerestrict(strdup(".d64"),16,16,28,13,"/",0);
        if(strlen(mylinebuf) > 0) {
          tempstr = malloc(strlen(activepanel->path) + 18);
          sprintf(tempstr, "%s%s", activepanel->path, mylinebuf);
          spawnlp(S_WAIT,"cp",fpathname("blank.d64", getappdir(),1), tempstr, NULL);
          free(tempstr);
          clearpanel(toppanel);
          clearpanel(botpanel);
          builddir(activepanel);
          if(activepanel == toppanel)
            drawpanel(botpanel,1);
          else
            drawpanel(toppanel,1);
        } else {
          clearpanel(toppanel); 
          drawpanel(toppanel,1);
          clearpanel(botpanel);
          drawpanel(botpanel,1);
        }
        drawframe("The WiNGs File Manager");
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case '?':
        showhelp();
        drawframe("Welcome to the WiNGs File Manager");
          
        clearpanel(toppanel); 
        drawpanel(toppanel,1);
        clearpanel(botpanel);
        drawpanel(botpanel,1);
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case '+':
        if(botpanel->totalnumofrows > 2) {
          toppanel->totalnumofrows += 1;
          botpanel->totalnumofrows -= 1;
    
          if(botpanel->cursoroffset > botpanel->totalnumofrows-1) {
            botpanel->direntptr = botpanel->direntptr->prev;
            botpanel->cursoroffset--;
          }

          botpanel->firstrow = toppanel->firstrow+toppanel->totalnumofrows+1;
 
          drawframe("Welcome to the WiNGs File Manager");
          
          clearpanel(toppanel); 
          drawpanel(toppanel,1);
          clearpanel(botpanel);
          drawpanel(botpanel,1);
          setactivescrollregion();
        }
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;
      case '-':
        if(toppanel->totalnumofrows > 2) {
          toppanel->totalnumofrows -= 1;
          botpanel->totalnumofrows += 1;

          if(toppanel->cursoroffset > toppanel->totalnumofrows-1) {
            toppanel->direntptr = toppanel->direntptr->prev;
            toppanel->cursoroffset--;
          }

          botpanel->firstrow = toppanel->firstrow+toppanel->totalnumofrows+1;

          drawframe("Welcome to the WiNGs File Manager");
          
          clearpanel(toppanel); 
          drawpanel(toppanel,1);
          clearpanel(botpanel);
          drawpanel(botpanel,1);
          setactivescrollregion();
        }
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case TAB:  
        panelchange();
      break;

      case 'h':
        if(showhidden) {
          showhidden = 0;
          drawframe("Conceal hidden files");
        } else {
          showhidden = 1;
          drawframe("Show hidden files");
        }
        builddir(toppanel);
        builddir(botpanel);
      break;

      case ' ':
        if(!activepanel->direntptr->parent) {
          con_gotoxy(1, activepanel->firstrow+activepanel->cursoroffset);
          if(activepanel->direntptr->tag == ' ') {
            putchar('*');
            activepanel->direntptr->tag = '*';
          } else {
            putchar(' ');
            activepanel->direntptr->tag = ' ';
          }

          con_gotoxy(0, activepanel->firstrow+activepanel->cursoroffset);
          putchar('>');
        }
        input = CURD;
        goto forcekeypress;
      break;

      case 'T':
        launch(activepanel, 1);
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      //C= O standing for Open. 
      case 15:
        if(!strcmp(activepanel->direntptr->filetype, "Application")) {
          if(activepanel->inimage)
            activepanel->inimage++;
          sprintf(activepanel->path, "%s%s.app/", activepanel->path, activepanel->direntptr->filename);
          builddir(activepanel);
        }
      break;

      case '\n':
      case '\r':
        if(!strcmp(activepanel->direntptr->filetype, "Directory")) {
          if(!activepanel->direntptr->parent) {
            if(activepanel->inimage)
              activepanel->inimage++;
            sprintf(activepanel->path, "%s%s/", activepanel->path, activepanel->direntptr->filename);
            builddir(activepanel);
          } else {
            if(activepanel->inimage) {
              activepanel->inimage--;
              if(!activepanel->inimage) {
                umount(activepanel->path);
                activepanel->path = activepanel->prevpath;
                goto afterimageumount;
              }
            }
            for(i=2;i<=strlen(activepanel->path);i++) {
              if(activepanel->path[strlen(activepanel->path)-i] == '/') {
                prevdir = strdup(&activepanel->path[strlen(activepanel->path)-i+1]);
                prevdir[strlen(prevdir)-1] = 0;

                activepanel->path[strlen(activepanel->path)-i+1] = 0;
                break;
              }
            } 
            afterimageumount:
            builddir(activepanel);
          }
        } else 
          launch(activepanel, 0);

        if(prevdir && 0) {
          if(!strcmp(activepanel->direntptr->filename,prevdir)) {
            con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
            putchar('>');
          }
          while(strcmp(activepanel->direntptr->filename,prevdir)) {
            if(activepanel->cursoroffset < (activepanel->totalnumofrows)-1) {
              movechardown(0,activepanel->firstrow+activepanel->cursoroffset, '>');
              activepanel->direntptr = activepanel->direntptr->next;
              activepanel->cursoroffset++;
            } else {
              putchar('\n');
              activepanel->direntptr = activepanel->direntptr->next;
              drawpanelline(activepanel->direntptr);
              con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset-1);
              putchar(' ');
              con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
              putchar('>');
            }                
          }
          free(prevdir);
          prevdir = NULL;
        } else {
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
          putchar('>');
        }
      break;

      case 'r':
        i = 0;
        tempnode = activepanel->direntptr;
        do {
          if(activepanel->direntptr->tag == '*') {
            renamefile();
            i++;
          }
          activepanel->direntptr = activepanel->direntptr->next;
        } while(activepanel->direntptr != tempnode);

        if(!i)
          renamefile();

        con_clrscr();

        if(!strcmp(toppanel->path, botpanel->path)) {
          builddir(toppanel);
          builddir(botpanel);
        } else {
          builddir(activepanel);
          if(activepanel == toppanel)
            drawpanel(botpanel,1);
          else
            drawpanel(toppanel,1);
        }

        drawframe(" Renaming Complete ");
        system("sync");

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'p':
        drawmessagebox("Current Path:",activepanel->path,1);
        drawframe("Welcome to the WiNGs File Manager");
          
        clearpanel(toppanel); 
        drawpanel(toppanel,1);
        clearpanel(botpanel);
        drawpanel(botpanel,1);
      break;

      case DEL:
        drawmessagebox("Are you sure you want to DELETE items?","         (Y)es     or     (n)o",0);
        i = 0;
        while(i != 'Y' && i != 'n') 
          i = con_getkey();

        if(i == 'Y') {
          drawframe(" Deleting tagged Files and Directories ");
          tempnode = activepanel->direntptr;

          i = 0;
          tempstr = malloc(strlen(activepanel->path)+18);
          do {
            if(activepanel->direntptr->tag == '*') {

              if(!strcmp(activepanel->direntptr->filetype,"Application"))
                sprintf(tempstr,"%s%s.app",activepanel->path,activepanel->direntptr->filename);
              else
                sprintf(tempstr,"%s%s",activepanel->path,activepanel->direntptr->filename);
 
             spawnlp(S_WAIT,"rm","-r","-f",tempstr,NULL);
              i++;
            }
            activepanel->direntptr = activepanel->direntptr->next;
          } while(activepanel->direntptr != tempnode);

          if(!i) {
            if(!strcmp(activepanel->direntptr->filetype,"Application"))
              sprintf(tempstr,"%s%s.app",activepanel->path,activepanel->direntptr->filename);
            else
              sprintf(tempstr,"%s%s",activepanel->path,activepanel->direntptr->filename);
 
            spawnlp(S_WAIT,"rm","-r","-f",tempstr,NULL);
          }

          free(tempstr);

          builddir(toppanel);
          builddir(botpanel);
          drawframe(" Deleting Complete ");

        } else {
          drawframe("Welcome to the WiNGs File Manager");
          
          clearpanel(toppanel); 
          drawpanel(toppanel,1);
          clearpanel(botpanel);
          drawpanel(botpanel,1);
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'Z':
        drawmessagebox("Add the selected files to a ZIP archive?", "         (y)es         (n)o",0);
        while(input != 'y' && input != 'n')
          input = con_getkey();
        
        i = 0;
        if(input == 'y') {
          tempnode = activepanel->direntptr;
          do {
            if(activepanel->direntptr->tag == '*')
              i++;
            activepanel->direntptr = activepanel->direntptr->next;
          } while(tempnode != activepanel->direntptr);
          if(!i)
            drawmessagebox("No files flagged.","Press any key.",1);
        }
        
        if(input == 'y' && i) {
          drawmessagebox("Name for new ZIP archive:"," ",0);
          mylinebuf = getmylinerestrict(NULL,16,16,27,13,"/",0);
          if(strlen(mylinebuf) > 0) {
            drawmessagebox("ZZzzipping...","Please Wait",0);
            tempstr = malloc(strlen("puzip > 2>/dev/null") + strlen(mylinebuf) + (18 * i) + 1);
            sprintf(tempstr,"puzip");
            do {
              if(activepanel->direntptr->tag == '*')
                sprintf(tempstr,"%s %s",tempstr,activepanel->direntptr->filename);
              activepanel->direntptr = activepanel->direntptr->next;
            } while(tempnode != activepanel->direntptr);
            sprintf(tempstr,"%s >%s 2>/dev/null", tempstr,mylinebuf);
            chdir(activepanel->path);            
            //drawmessagebox(tempstr,"",1);
            system(tempstr);
            free(tempstr);
          }
          free(mylinebuf);
          con_clrscr();
          drawframe("The WiNGs File Manager");
          builddir(activepanel);
          if(activepanel == toppanel)
            drawpanel(botpanel,1);
          else
            drawpanel(toppanel,1);
        } else {
          con_clrscr();
          drawframe("The WiNGs File Manager");
          drawpanel(toppanel,1);
          drawpanel(botpanel,1);
        }
      break;

      case 'W':
        drawmessagebox("Enqueue selected files to WaveStream?","        (y)es         (n)o",0);
        while(input != 'y' && input != 'n')
          input = con_getkey();
        
        i = 0;
        if(input == 'y') {
          tempnode = activepanel->direntptr;
          do {
            if(activepanel->direntptr->tag == '*')
              i++;
            activepanel->direntptr = activepanel->direntptr->next;
          } while(tempnode != activepanel->direntptr);
        }
        
        if(input == 'y' && i) {
          tempstr = malloc(strlen("wavestream   >/dev/null 2>/dev/null") + (20 * i) + 1);
          sprintf(tempstr,"wavestream");
          do {
            if(activepanel->direntptr->tag == '*') {
              sprintf(tempstr,"%s %c%s%c",tempstr,'"',activepanel->direntptr->filename,'"');
              activepanel->direntptr->tag = ' ';
            }
            activepanel->direntptr = activepanel->direntptr->next;
          } while(tempnode != activepanel->direntptr);

          sprintf(tempstr,"%s >/dev/null 2>/dev/null", tempstr);
          chdir(activepanel->path);
          system(tempstr);
          free(tempstr);
        } else {
          tempstr = malloc(strlen("wavestream     >/dev/null 2>/dev/null") + 16 + 1);
          sprintf(tempstr,"wavestream %c%s%c >/dev/null 2>/dev/null",'"',activepanel->direntptr->filename,'"');
          chdir(activepanel->path);
          system(tempstr);
          free(tempstr);
        }
        con_clrscr();
        drawframe("The WiNGs File Manager");
        drawpanel(toppanel,1);
        drawpanel(botpanel,1);

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'm':
        drawframe(" Moving tagged Files and Directories ");
        tempnode = activepanel->direntptr;

        tempstr = malloc(strlen(activepanel->path)+18);

        i = 0;
        do {
          if(activepanel->direntptr->tag == '*') {
            i++;

            sprintf(tempstr,"%s%s",activepanel->path,activepanel->direntptr->filename);
            if(activepanel == toppanel)
              spawnlp(S_WAIT,"mv","-f",tempstr, botpanel->path,NULL);
            else
              spawnlp(S_WAIT,"mv","-f",tempstr, toppanel->path,NULL);
          }
          activepanel->direntptr = activepanel->direntptr->next;
        } while(activepanel->direntptr != tempnode);

        if(!i) {
          sprintf(tempstr,"%s%s",activepanel->path,activepanel->direntptr->filename);
          if(activepanel == toppanel)
            spawnlp(S_WAIT,"mv","-f",tempstr, botpanel->path,NULL);
          else
            spawnlp(S_WAIT,"mv","-f",tempstr, toppanel->path,NULL);
        }
        
        free(tempstr);

        drawframe(" Moving Complete ");
        builddir(botpanel);
        builddir(toppanel);
        system("sync");

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'c':
        j = i = 0;
        tempnode = activepanel->direntptr;

        tempstr = malloc(strlen("Copying:     of    ")+1);

        do {
          if(activepanel->direntptr->tag == '*')
            j++;
          activepanel->direntptr = activepanel->direntptr->next;
        } while(activepanel->direntptr != tempnode);

        do {
          if(activepanel->direntptr->tag == '*') {
            sprintf(tempstr, "Copying: %3d of %3d", i+1, j);
            drawmessagebox(tempstr,"",0);
            copyfile();
            i++;
          }
          activepanel->direntptr = activepanel->direntptr->next;
        } while(activepanel->direntptr != tempnode);

        if(!i) {
          drawmessagebox("Copying:   1 of   1","",0);
          copyfile();
        }

        free(tempstr);

        con_clrscr();

        if(activepanel == toppanel)
          builddir(botpanel);
        else 
          builddir(toppanel);

        clearpanel(activepanel);
        drawpanel(activepanel,1);
        
        drawframe(" Copying Complete ");
        system("sync");

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'n':
        tempstr = malloc(strlen(activepanel->path)+18);
        drawmessagebox("New Directory Name:", "                         ",0);

        mylinebuf = getmylinerestrict(NULL,16,16,27,13,"/",0);
        if(strlen(mylinebuf) > 0) {
          sprintf(tempstr, "%s%s", activepanel->path, mylinebuf);
          mkdir(tempstr,0);
          free(tempstr);
          con_clrscr();
          drawframe("New directory created");
          builddir(toppanel);
          builddir(botpanel);
        } else {
          con_clrscr();
          drawframe("New directory Aborted");
          drawpanel(toppanel,1);
          drawpanel(botpanel,1);
        }

        free(mylinebuf);

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');

      break;
    }
    con_update();
  } 

  con_end();
  con_reset();
  con_clrscr();

  fp = fopen("/wings/.fm.filepath.tmp", "w");
  fprintf(fp,"%s%s",activepanel->path,activepanel->direntptr->filename);
  fclose(fp);

  //Save last state...
  tempstr = malloc(10);

  xmlpanelstate = XMLgetNode(laststatexmlroot, "/xml/toppanel");
  XMLsetAttr(xmlpanelstate,"path", toppanel->path);
  sprintf(tempstr, "%d", toppanel->totalnumofrows);
  XMLsetAttr(xmlpanelstate, "numofrows", tempstr);
  sprintf(tempstr, "%d", toppanel->firstrow);
  XMLsetAttr(xmlpanelstate, "firstrow", tempstr);
  sprintf(tempstr, "%d", toppanel->sortby);
  XMLsetAttr(xmlpanelstate, "sortby", tempstr);

  xmlpanelstate = XMLgetNode(laststatexmlroot, "/xml/botpanel");
  XMLsetAttr(xmlpanelstate,"path", botpanel->path);
  sprintf(tempstr, "%d", botpanel->totalnumofrows);
  XMLsetAttr(xmlpanelstate, "numofrows", tempstr);
  sprintf(tempstr, "%d", botpanel->firstrow);
  XMLsetAttr(xmlpanelstate, "firstrow", tempstr);
  sprintf(tempstr, "%d", botpanel->firstrow);
  XMLsetAttr(xmlpanelstate, "sortby", tempstr);

  XMLsaveFile(laststatexmlroot,fpathname("laststate.xml", getappdir(),1));

  free(tempstr);
}
