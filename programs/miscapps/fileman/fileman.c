#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <wgslib.h>
#include <fcntl.h>
#include <console.h>
#include <termio.h>

#define sid ((uchar *)0xd400)

typedef struct direntry_s {
  struct direntry_s * next;
  struct direntry_s * prev;

  char * filename;
  int tag;
  int parent;
  char * filetype;
  long filesize;  
} direntry;

typedef struct panel_s {
  int firstrow;
  int cursoroffset;
  int totalnumofrows;

  char * path;

  char * prevpath;
  int inimage;

  direntry * headdirent;
  direntry * direntptr;
} panel;

direntry * tempdirent, * tempdirent2;

panel * toppanel, * botpanel, * activepanel;

int cbmfsloaded;

char * VERSION = "1.3";
int singleselect,extendedview;

int MainFG = COL_White;
int MainBG = COL_Blue;

int globaltioflags;

struct termios tio;

void prepconsole();
void drawpanel(panel * thepan);

void resetsid() {
  int i;
  for(i = 0; i<0x20; i++) 
    sid[i] = 0;
}

void drawmessagebox(char * string1, char * string2, int pressakey) {
  int width, startcolumn, row, i, padding1, padding2;

  if(strlen(string1) < strlen(string2)) {
    width = strlen(string2);
    padding1 = width - strlen(string1);
    padding2 = 0;
  } else {
    width = strlen(string1);
    padding1 = 0;
    padding2 = width - strlen(string2);
  }
  width = width+6;

  row         = 10;
  startcolumn = (con_xsize - width)/2;

  con_gotoxy(startcolumn, row);
  putchar(' ');
  for(i = 0; i < width-2; i++)
    printf("_");
  putchar(' ');

  row++;

  con_gotoxy(startcolumn, row);
  printf(" |");
  for(i = 0; i < width-4; i++)
    printf(" ");
  printf("| ");

  row++;

  con_gotoxy(startcolumn, row);
  printf(" | %s", string1);

  for(i=0; i<padding1; i++)
    putchar(' ');

  printf(" | ");
    
  row++;

  if(strlen(string2) > 0) {
    con_gotoxy(startcolumn, row);
    printf(" | %s", string2);  
    
    for(i=0; i<padding2; i++)
      putchar(' ');
  
    printf(" | ");
  
    row++;
  }
  
  //Extra row... 
  /*
  con_gotoxy(startcolumn, row);
  printf(" |");
  for(i = 0; i < width-4; i++)
    printf(" ");
  printf("| "); 
    
  row++;
  */
  con_gotoxy(startcolumn, row);
  printf(" |");
  for(i = 0; i < width-4; i++) 
    printf("_");
  printf("| ");
      
  con_update();

  if(pressakey)
    con_getkey();
}
  
void movechardown(int x, int y, char c){
  con_gotoxy(x, y);
  putchar(' ');
  con_gotoxy(x, y+1);
  putchar(c);   
  con_update();
}   

void movecharup(int x, int y, char c){
  con_gotoxy(x, y);
  putchar(' ');
  con_gotoxy(x, y-1);
  putchar(c);  
  con_update();
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

  titlestr = (char *)malloc(strlen(" File Manager Version  2003 ")+strlen(VERSION)+2);

  sprintf(titlestr, " File Manager Version %s 2003/04 ", VERSION);

  xpos = 0;
  ypos = 0;

  con_gotoxy(xpos,ypos);
  if(activepanel == toppanel)
    con_setfgbg(COL_Black,COL_Red);
  else
    con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  for(i = 0;i < (con_xsize - strlen(titlestr))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar(' ');
    xpos++;
  }
	
  printf("%s",titlestr);
  xpos = xpos + strlen(titlestr);

  for(i = 0;i <= (con_xsize - strlen(titlestr))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar(' ');
    xpos++;
  }

  ypos = botpanel->firstrow - 1;
  xpos = 0;

  con_gotoxy(xpos,ypos);
  if(activepanel == botpanel)
    con_setfgbg(COL_Black,COL_Red);
  else
    con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  for(i = 0;i < (con_xsize - strlen(message))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar(' ');
    xpos++;
  }
	
  printf("%s",message);
  xpos = xpos + strlen(message);

  for(i = 0;i <= (con_xsize - strlen(message))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar(' ');
    xpos++;
  }

  con_gotoxy(0,botpanel->firstrow+botpanel->totalnumofrows+1);
  con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  if(extendedview) 
    printf(" TAB, SPACE, (n)ew dir, (r)ename, (m)ove, (c)opy, (D)elete, (S)elect and quit");
  else
    printf(" TAB/SPACE (n,r,m,c,D,S)");

  con_setfgbg(MainFG,MainBG);
}

void clearpanel(panel * thepan) {
  int i;

  for(i = thepan->firstrow; i<thepan->firstrow+thepan->totalnumofrows; i++) {
    con_gotoxy(0,i);
    con_clrline(LC_End);   
  }
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

  con_setscroll(activepanel->firstrow, activepanel->firstrow+activepanel->totalnumofrows);
}

void drawpanelline(direntry * direntptr, int active) {

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

  printf("%s", direntptr->filetype);
}

void drawpanel(panel * thepan) {
  int i, active;
  direntry * dirptr;

  dirptr = thepan->headdirent;

  if(thepan == activepanel)
    active = 1;
  else 
    active = 0;

  for(i=thepan->firstrow;i<thepan->firstrow+thepan->totalnumofrows;i++) {
    con_gotoxy(0,i);
    drawpanelline(dirptr, active);
    if(dirptr->next == thepan->headdirent)
      break;    
    dirptr = dirptr->next;
  }
}

void builddir(panel * thepan) {
  char * fullname, *ext;
  int i, root;
  long filesize;
  DIR *dir;
  direntry * tempnode;
  struct dirent *entry;
  struct stat buf;

  clearpanel(thepan);

  while(thepan->headdirent) {
    free(thepan->headdirent->filename);
    if(thepan->headdirent->filetype)
      free(thepan->headdirent->filetype);
    thepan->headdirent = remQueue(thepan->headdirent, thepan->headdirent);
  }

  if(strcmp(thepan->path,"/")) {
    tempnode = malloc(sizeof(direntry));
    tempnode->filename = strdup("Parent (../)");
    tempnode->tag      = ' ';
    tempnode->parent   = 1;
    tempnode->filetype = strdup("Directory");
    tempnode->filesize = 0;

    thepan->headdirent = addQueueB(thepan->headdirent,thepan->headdirent,tempnode);

    root = 0;
  } else
    root = 1;

  thepan->cursoroffset = 0;

  dir = opendir(thepan->path);
  if(!dir) { 
    con_end();
    con_clrscr();
    exit(EXIT_FAILURE);
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

    if(entry->d_name[0] == '.')
      continue;

    filesize = 10;
    tempnode = malloc(sizeof(direntry));
    thepan->headdirent = addQueueB(thepan->headdirent, thepan->headdirent, tempnode);
    tempnode->filename = strdup(entry->d_name);
    tempnode->parent = 0;

    ext = &entry->d_name[strlen(entry->d_name)-4];

    if(!strcmp(ext,".app")) {
      tempnode->filetype = strdup("Application");
      tempnode->filename[strlen(entry->d_name)-4] = 0; 
      filesize = 0;
    } else if(entry->d_type == 6) {
      tempnode->filetype = strdup("Directory");
      tempnode->filesize = 0;
    } else if(!strcmp(ext,".txt")) {
      tempnode->filetype= strdup("TEXT Document");
    } else if(!strcmp(ext,".mod") ||
              !strcmp(ext,".s3m") ||
              !strcmp(&ext[1],".xm")) {
      tempnode->filetype = strdup("Module Music");
    } else if(!strcmp(ext,".sid") ||
              !strcmp(ext,".dat")) {
      tempnode->filetype = strdup("SID Music");
    } else if(!strcmp(ext,".tmp")) {
      tempnode->filetype = strdup("Temporary File");
    } else if(!strcmp(ext,".jpg")) {
      tempnode->filetype = strdup("JPEG Image");
    } else if(!strcmp(ext,".hbm")) {
      tempnode->filetype = strdup("Highres BitMap");
    } else if(!strcmp(ext,".wav")) {
      tempnode->filetype = strdup("WAVE Audio");
    } else if(!strcmp(ext,".mov")) {
      tempnode->filetype = strdup("WiNGs Movie");
    } else if(!strcmp(ext,".rvd")) {
      tempnode->filetype = strdup("Raw Video Data");
    } else if(!strcmp(ext,".drv")) {
      tempnode->filetype = strdup("Driver");
    } else if(!strcmp(ext,"font")) {
      tempnode->filetype = strdup("GUI Font");
    } else if(!strcmp(ext,"cfnt")) {
      tempnode->filetype = strdup("Console Font");
    } else if(!strcmp(&ext[1],".so")) {
      tempnode->filetype = strdup("Shared Object");
    } else if(!strcmp(ext,".bak")) {
      tempnode->filetype = strdup("Backup File");
    } else if(!strcmp(ext,".zip") ||
              !strcmp(&ext[1], ".gz")) {
      tempnode->filetype = strdup("ZIP Archive");
    } else if(!strcasecmp(ext,".prg")) {
      tempnode->filetype = strdup("C64 Binary");
    } else if(!strcasecmp(ext,".d64")) {
      tempnode->filetype = strdup("Disk Image");
    } else if(!strcmp(ext,"html") ||
              !strcmp(ext,".htm")) {
      tempnode->filetype = strdup("HTML Document");
    } else if(!strcmp(ext,".vcf")) {
      tempnode->filetype = strdup("Addressbook Data");
    } else {
      tempnode->filetype = strdup(" ");
    }

    if(filesize == 10) {
      fullname = fpathname(entry->d_name, thepan->path, 1);
      stat(fullname, &buf);
      filesize = buf.st_size;
    }
    tempnode->filesize = filesize;
    tempnode->tag = ' ';
  }

  thepan->direntptr = tempnode->next;

  closedir(dir);  

  drawpanel(thepan);
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
    spawnlp(S_WAIT, "ned", tempstr, NULL);
    prepconsole();
    con_clrscr();
    drawframe("Welcome to the WiNGs Filemanager");
    clearpanel(toppanel);
    clearpanel(botpanel);
    drawpanel(toppanel);
    drawpanel(botpanel);
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
      drawpanel(toppanel);
      clearpanel(botpanel);
      drawpanel(botpanel);
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
}

char * getmyline(int size, int x, int y) {
  int i,count = 0;
  char * linebuf;

  linebuf = (char *)malloc(size+1);

  con_gotoxy(x,y);
  con_update();

  /*  ASCII Codes

    32 is SPACE
    126 is ~
    47 is /
    8 is DEL

  */

  while(1) {
    i = con_getkey();
    if(i > 31 && i < 127 && i != 47  && count < size) {
      linebuf[count] = i;
      con_gotoxy(x+count,y);
      putchar(i);
      linebuf[++count] = 0;
      con_update();
    } else if(i == 8 && count > 0) {
      count--;
      con_gotoxy(x+count,y);
      putchar(' ');
      con_gotoxy(x+count,y);
      linebuf[count] = 0;
      con_update();
    } else if(i == '\n' || i == '\r') 
      break;
  }
  return(linebuf);
}

void prepconsole() {
  con_init();

  gettio(STDOUT_FILENO, &tio);
  tio.flags |= TF_ECHO|TF_ICRLF;
  tio.MIN = 1;  
  settio(STDOUT_FILENO, &tio);
}

void main() {
  FILE * tempout;
  int input,i,size = 0;
  char *tempstr, *tempstr2, *mylinebuf, *getbuf, *prevdir = NULL;
  direntry * tempnode;

  prepconsole();

  if(con_xsize < 80) {
    extendedview = 0;

    //Until the 40 column scrolling is implemented properly.

    printf("Fileman requires an 80 column console.\n");
    con_update();
    con_end();
    exit(1);

  } else
    extendedview = 1;

  toppanel = (panel *)malloc(sizeof(panel)+1);
  botpanel = (panel *)malloc(sizeof(panel)+1);

  toppanel->headdirent = NULL;
  botpanel->headdirent = NULL;

  toppanel->totalnumofrows = (con_ysize - 3)/2;
  botpanel->totalnumofrows = (con_ysize - 3)/2;

  toppanel->firstrow = 1;
  botpanel->firstrow = toppanel->firstrow+toppanel->totalnumofrows+1;

  toppanel->path = (char *)malloc(1024);
  botpanel->path = (char *)malloc(1024);

  sprintf(toppanel->path, "/");
  sprintf(botpanel->path, "/");
   
  toppanel->inimage = 0;
  botpanel->inimage = 0;

  con_setfgbg(MainFG,MainBG);
  con_clrscr();

  builddir(botpanel);
  builddir(toppanel);

  activepanel = toppanel;  

  drawframe("Welcome to the WiNGs File Manager");

  panelchange();
  panelchange();

  con_update();

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
          drawpanelline(activepanel->direntptr, 1);
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
          drawpanelline(activepanel->direntptr, 1);
          con_gotoxy(0,activepanel->firstrow+1);
          putchar(' ');
          con_gotoxy(0,activepanel->firstrow);
          putchar('>');
        }
      break;

      case TAB:  
        panelchange();
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
              drawpanelline(activepanel->direntptr, 1);
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

        tempstr  = (char *)malloc(strlen("Rename:   ") + strlen(activepanel->path)+18);
        tempstr2 = (char *)malloc(strlen(activepanel->path)+17);

        do {
          if(activepanel->direntptr->tag == '*') {
            sprintf(tempstr,"Rename: %s",activepanel->direntptr->filename);

            drawmessagebox(tempstr, "                         ",0);

            if(!strcmp(activepanel->direntptr->filetype,"Application")) {
              mylinebuf = getmyline(12,27,13);
              if(strlen(mylinebuf) > 0) {

                sprintf(tempstr,"%s%s.app",activepanel->path,activepanel->direntptr->filename);
                sprintf(tempstr2,"%s%s.app",activepanel->path,mylinebuf);

                spawnlp(S_WAIT,"mv","-f",tempstr,tempstr2,NULL);
              }
            } else {
              mylinebuf = getmyline(16,27,13);
              if(strlen(mylinebuf) > 0) {

                sprintf(tempstr,"%s%s",activepanel->path,activepanel->direntptr->filename);
                sprintf(tempstr2,"%s%s",activepanel->path,mylinebuf);

                spawnlp(S_WAIT,"mv","-f",tempstr,tempstr2,NULL);
              }
            }
            free(mylinebuf);
            i++;
          }
          activepanel->direntptr = activepanel->direntptr->next;
        } while(activepanel->direntptr != tempnode);


        free(tempstr);
        free(tempstr2);

        if(i) { 
          con_clrscr();

          if(!strcmp(toppanel->path, botpanel->path)) {
            builddir(toppanel);
            builddir(botpanel);
          } else {
            builddir(activepanel);
            if(activepanel == toppanel)
              drawpanel(botpanel);
            else
              drawpanel(toppanel);
          }

          drawframe(" Renaming Complete ");
          system("sync");
        } else {
          if(extendedview)
            drawframe(" Press SPACE to tag Files and Directories ");
          else
            drawframe(" Press SPACE to tag Files");
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'D':
        drawmessagebox("Are you sure you want to delete all flagged items?","               (Y)es, (n)o",0);
        i = 'z';
        while(i != 'Y' && i != 'n') 
          i = con_getkey();

        if(i == 'Y') {
          drawframe(" Deleting tagged Files and Directories ");
          tempnode = activepanel->direntptr;

          i = 0;
          tempstr = (char *)malloc(strlen(activepanel->path)+18);
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
          free(tempstr);

          if(i) { 
            builddir(toppanel);
            builddir(botpanel);
            drawframe(" Deleting Complete ");
          } else {
            drawframe("Welcome to the WiNGs File Manager");
          
            clearpanel(toppanel); 
            drawpanel(toppanel);
            clearpanel(botpanel);
            drawpanel(botpanel);
          }
        } else {
          drawframe("Welcome to the WiNGs File Manager");
          
          clearpanel(toppanel); 
          drawpanel(toppanel);
          clearpanel(botpanel);
          drawpanel(botpanel);
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'm':
        i = 0;
        drawframe(" Moving tagged Files and Directories ");
        tempnode = activepanel->direntptr;

        do {
          if(activepanel->direntptr->tag == '*') {
            i++;
            tempstr = (char *)malloc(strlen(activepanel->path)+strlen(activepanel->direntptr->filename)+1);

            sprintf(tempstr,"%s%s",activepanel->path,activepanel->direntptr->filename);
            if(activepanel == toppanel)
              spawnlp(S_WAIT,"mv","-f",tempstr, botpanel->path,NULL);
            else
              spawnlp(S_WAIT,"mv","-f",tempstr, toppanel->path,NULL);
          }
          activepanel->direntptr = activepanel->direntptr->next;
        } while(activepanel->direntptr != tempnode);

        if(i) { 
          drawframe(" Moving Complete ");
          builddir(botpanel);
          builddir(toppanel);
          system("sync");
        } else {
          if(extendedview)
            drawframe(" Press SPACE to tag Files and Directories ");
          else
            drawframe(" Press SPACE to tag Files");
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'c':
        i = 0;
        drawframe(" Copying tagged Files and Directories ");
        tempnode = activepanel->direntptr;

        do {
          if(activepanel->direntptr->tag == '*') {
            i++;
            tempstr = (char *)malloc(strlen(activepanel->path)+strlen(activepanel->direntptr->filename)+1);

            sprintf(tempstr,"%s%s",activepanel->path,activepanel->direntptr->filename);
            if(activepanel == toppanel)
              spawnlp(S_WAIT,"cp","-r","-f",tempstr, botpanel->path,NULL);
            else
              spawnlp(S_WAIT,"cp","-r","-f",tempstr, toppanel->path,NULL);
          }
          activepanel->direntptr = activepanel->direntptr->next;
        } while(activepanel->direntptr != tempnode);

        if(i) { 
          drawframe(" Copying Complete ");
          if(activepanel == toppanel)
            builddir(botpanel);
          else
            builddir(toppanel);
          system("sync");
        } else {
          if(extendedview)
            drawframe(" Press SPACE to tag Files and Directories ");
          else
            drawframe(" Press SPACE to tag Files");
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;

      case 'n':
        tempstr = (char *)malloc(strlen("New Directory Name:") +1);
        tempstr2 = (char *)malloc(strlen("mkdir  2>/dev/null >/dev/null") + strlen(activepanel->path)+18);

        sprintf(tempstr,"New Directory Name:");
        drawmessagebox(tempstr, "                         ",0);
        mylinebuf = getmyline(16,27,13);
        if(strlen(mylinebuf) > 0) {
          sprintf(tempstr2,"mkdir \"%s%s\" 2>/dev/null >/dev/null",activepanel->path,mylinebuf);
          system(tempstr2);
          con_clrscr();
          drawframe("New directory created");
          builddir(toppanel);
          builddir(botpanel);
        }

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');

      break;
    }
    con_update();
  } 

  con_end();
  printf("\x1b[0m"); //reset the term colors
  con_clrscr();
  tempout = fopen("/wings/.fm.filepath.tmp", "w");
  fprintf(tempout,"%s%s",activepanel->path,activepanel->direntptr->filename);
  fclose(tempout);
  con_update();
}
