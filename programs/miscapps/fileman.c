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
#include <xmldom.h>
extern char *getappdir();

typedef struct panel_s {
  int firstrow;
  int cursoroffset;
  int totalnumofrows;

  char * path;

  DOMElement * xmldirtree;
  DOMElement * xmltreeptr;
} panel;

DOMElement * tempnode;

panel * toppanel, * botpanel, * activepanel;

char * VERSION = "1.0";
int singleselect,extendedview;

struct termios tio;

void cleanexit(int code) {
  con_end();
  printf("\x1b[0m"); //reset the terminal.
  con_clrscr();
  con_update();
  exit(code);
}

void memerror() {
  fprintf(stderr, "MEMORY ALLOCATION ERROR!! RUN FOR THE HILLS!\n");
  exit(0);
}

void drawmessagebox(char * string1, char * string2) {
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
  
  con_gotoxy(startcolumn, row);
  printf(" |");
  for(i = 0; i < width-4; i++)
    printf(" ");
  printf("| "); 
    
  row++;

  con_gotoxy(startcolumn, row);
  printf(" ");
  for(i = 0; i < width-2; i++) 
    printf("-");
  printf(" ");
      
  con_update();
}
  
void movechardown(int 
x, int y, char c){
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

void lineeditmode() {
  tio.flags |= TF_ICANON;
  settio(STDOUT_FILENO, &tio);
} 

void onecharmode() {
  tio.flags &= ~TF_ICANON;    
  settio(STDOUT_FILENO, &tio);
}

void settioflags(int tioflagsettings) {
  tio.flags = tioflagsettings;
  settio(STDOUT_FILENO, &tio);
}
  
void pressanykey() { 
  int temptioflags;
  temptioflags = tio.flags;   
  onecharmode();
  getchar();
  settioflags(temptioflags);
}

void drawframe(char * message) {
  char * titlestr;
  int i, xpos, ypos;

  titlestr = (char *)malloc(strlen(" File Manager Version  2003 ")+strlen(VERSION)+2);
  if(titlestr == NULL)
    memerror();

  sprintf(titlestr, " File Manager Version %s 2003 ", VERSION);

  xpos = 0;
  ypos = 0;

  for(i = 0;i < (con_xsize - strlen(titlestr))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar('-');
    xpos++;
  }
	
  printf("%s",titlestr);
  xpos = xpos + strlen(titlestr);

  for(i = 0;i <= (con_xsize - strlen(titlestr))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar('-');
    xpos++;
  }

  ypos = botpanel->firstrow - 1;
  xpos = 0;

  for(i = 0;i < (con_xsize - strlen(message))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar('-');
    xpos++;
  }
	
  printf("%s",message);
  xpos = xpos + strlen(message);

  for(i = 0;i <= (con_xsize - strlen(message))/2; i++) {
    con_gotoxy(xpos, ypos);
    putchar('-');
    xpos++;
  }

  con_gotoxy(0,botpanel->firstrow+botpanel->totalnumofrows+1);
  if(extendedview) 
    printf("(+/-), (p)anel, (t)ag, (r)ename, (m)ove, (c)opy, (D)elete, (S)elect and quit");
  else
    printf("(+/-), (p,t,r,m,c,D,S)");
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

  con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
  putchar('>');

  con_setscroll(activepanel->firstrow, activepanel->firstrow+activepanel->totalnumofrows);
}

void drawpanel(panel * thepan) {
  int i;
  DOMElement * dirptr;

  dirptr = XMLgetNode(thepan->xmldirtree, "entry");

    for(i=thepan->firstrow;i<thepan->firstrow+thepan->totalnumofrows;i++) {
      con_gotoxy(0,i);
      if(strcmp(XMLgetAttr(dirptr,"filesize")," "))
        printf(" %s %16s %16s bytes %s", XMLgetAttr(dirptr, "tag"), XMLgetAttr(dirptr, "filename"), XMLgetAttr(dirptr,"filesize"), XMLgetAttr(dirptr, "directory"));
      else
        printf(" %s %16s %16s       %s", XMLgetAttr(dirptr, "tag"), XMLgetAttr(dirptr, "filename"), XMLgetAttr(dirptr,"filesize"), XMLgetAttr(dirptr, "directory"));
      if(dirptr->NextElem->FirstElem)
        break;    
      dirptr = dirptr->NextElem;
    }
}

void builddir(panel * thepan) {
  char * fullname, * filesize;
  int i;
  DIR *dir;
  struct dirent *entry;
  struct stat buf;

  clearpanel(thepan);

  if(thepan->xmldirtree) {
    tempnode = XMLgetNode(thepan->xmldirtree,"entry");
    while(!tempnode->NextElem->FirstElem) {
      XMLremNode(tempnode->NextElem);    
    }
    XMLremNode(tempnode);
  } else 
    thepan->xmldirtree = XMLnewNode(NodeType_Element, "xmldirtree", "");

  if(strcmp(thepan->path,"/")) {
    tempnode = XMLnewNode(NodeType_Element, "entry", "");
    XMLinsert(thepan->xmldirtree, NULL, tempnode);
    XMLsetAttr(tempnode, "filename", "Parent Directory");
    XMLsetAttr(tempnode, "tag", "-");
    XMLsetAttr(tempnode, "directory", "dir");
    XMLsetAttr(tempnode, "filesize", " ");
  }

  thepan->cursoroffset = 0;

  dir = opendir(thepan->path);
  if(!dir) 
    cleanexit(0);
  
  filesize = (char *)malloc(25);
  if(filesize == NULL)
    memerror();

  while(entry = readdir(dir)) {
    tempnode = XMLnewNode(NodeType_Element, "entry", "");
    XMLinsert(thepan->xmldirtree, NULL, tempnode);    
    XMLsetAttr(tempnode, "filename", entry->d_name);
    if(entry->d_type == 6) {
      XMLsetAttr(tempnode, "directory", "dir");
      sprintf(filesize, " ");
    } else {
      XMLsetAttr(tempnode, "directory", "   ");
    
      fullname = fpathname(entry->d_name, thepan->path, 1);
      stat(fullname, &buf);
      sprintf(filesize, "%10ld", buf.st_size);
    }
    XMLsetAttr(tempnode, "filesize", filesize);
    XMLsetAttr(tempnode, "tag", " ");
  }

  thepan->xmltreeptr = tempnode->NextElem;

  free(filesize);

  close(dir);  

  //XMLsaveFile(thepan->xmldirtree, "/test/outfile.xml");

  drawpanel(thepan);
}

void main(int argc, char *argv[]) {
  int ch, input,i;
  char * tempstr, * tempstr2;
  int size;
  char * getbuf;

  getbuf = NULL;
  size = 0;

  while((ch = getopt(argc,argv, "s")) != EOF) {
    switch(ch) {
      case 's':
        singleselect = 1;
      break;
    }
  }

  con_init();

  gettio(STDOUT_FILENO, &tio);
  tio.flags |= TF_ECHO|TF_ICRLF;
  tio.MIN = 1;  
  settio(STDOUT_FILENO, &tio);

  if(con_xsize < 80)
    extendedview = 0;
  else
    extendedview = 1;

  toppanel = (panel *)malloc(sizeof(panel)+1);
  if(toppanel == NULL)
    memerror();
  botpanel = (panel *)malloc(sizeof(panel)+1);
  if(botpanel == NULL)
    memerror();

  toppanel->xmldirtree = NULL;
  botpanel->xmldirtree = NULL;

  toppanel->totalnumofrows = (con_ysize - 3)/2;
  botpanel->totalnumofrows = (con_ysize - 3)/2;

  toppanel->firstrow = 1;
  botpanel->firstrow = toppanel->firstrow+toppanel->totalnumofrows+1;

  toppanel->path = (char *)malloc(256);
  if(toppanel->path == NULL)
    memerror();
  botpanel->path = (char *)malloc(256);
  if(botpanel->path == NULL)
    memerror();

  sprintf(toppanel->path, "/");
  sprintf(botpanel->path, "/");

  builddir(toppanel);
  builddir(botpanel);

  drawframe(" Welcome to the WiNGs File Manager ");

  activepanel = botpanel;
  panelchange();

  con_update();

  input = 'a';
  while(input != 'S') {
    input = getchar();
    switch(input) {
      case '-':
        if(activepanel->xmltreeptr->NextElem->FirstElem)
          break;
        if(activepanel->cursoroffset < (activepanel->totalnumofrows)-1) {
          movechardown(0,activepanel->firstrow+activepanel->cursoroffset, '>');
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
          activepanel->cursoroffset++;
        } else {
          putchar('\n');
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
          printf(" %s %16s %16s bytes %s", XMLgetAttr(activepanel->xmltreeptr, "tag"), XMLgetAttr(activepanel->xmltreeptr, "filename"), XMLgetAttr(activepanel->xmltreeptr,"filesize"), XMLgetAttr(activepanel->xmltreeptr, "directory"));
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset-1);
          putchar(' ');
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
          putchar('>');
        }                
      break;
      case '+':
        if(activepanel->xmltreeptr->FirstElem)
          break;
        if(activepanel->cursoroffset > 0) {
          movecharup(0,activepanel->firstrow+activepanel->cursoroffset, '>');
          activepanel->xmltreeptr = activepanel->xmltreeptr->PrevElem;
          activepanel->cursoroffset--;
        } else {
          printf("\x1b[1L");
          con_gotoxy(0,activepanel->firstrow);
          activepanel->xmltreeptr = activepanel->xmltreeptr->PrevElem;
          printf(" %s %16s %16s bytes %s", XMLgetAttr(activepanel->xmltreeptr, "tag"), XMLgetAttr(activepanel->xmltreeptr, "filename"), XMLgetAttr(activepanel->xmltreeptr,"filesize"), XMLgetAttr(activepanel->xmltreeptr, "directory"));
          con_gotoxy(0,activepanel->firstrow+1);
          putchar(' ');
          con_gotoxy(0,activepanel->firstrow);
          putchar('>');
        }
      break;
      case 'p':
        panelchange();
      break;
      case 't':
        if(strcmp(XMLgetAttr(activepanel->xmltreeptr,"tag"), "-")) {
          con_gotoxy(1, activepanel->firstrow+activepanel->cursoroffset);
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr,"tag"), " ")) {
            putchar('*');
            XMLsetAttr(activepanel->xmltreeptr,"tag","*");
          } else {
            putchar(' ');
            XMLsetAttr(activepanel->xmltreeptr,"tag"," ");
          }
          con_gotoxy(0, activepanel->firstrow+activepanel->cursoroffset);
          putchar('>');
        }
      break;
      case '\n':
        if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "directory"), "dir")) {
          if(strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), "-")) {
            sprintf(activepanel->path, "%s%s/", activepanel->path, XMLgetAttr(activepanel->xmltreeptr, "filename"));
            builddir(activepanel);
          } else {
            for(i=2;i<=strlen(activepanel->path);i++) {
              if(activepanel->path[strlen(activepanel->path)-i] == '/') {
                activepanel->path[strlen(activepanel->path)-i+1] = 0;
                break;
              }
            } 
            builddir(activepanel);
          }
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
          putchar('>');
        }
      break;
      case 'r':
        //con_setscroll(0,0);
        i = 0;
        tempnode = activepanel->xmltreeptr;
        do {
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), "*")) {
            i++;
            tempstr = (char *)malloc(strlen("Rename:   ") + strlen(activepanel->path)+strlen(XMLgetAttr(activepanel->xmltreeptr,"filename"))+1);
            if(tempstr == NULL)
              memerror();
            tempstr2 = (char *)malloc(strlen(activepanel->path)+18);
            if(tempstr2 == NULL)
              memerror();
            sprintf(tempstr,"Rename: %s",XMLgetAttr(activepanel->xmltreeptr,"filename"));
            drawmessagebox(tempstr, "                         ");
            con_gotoxy(27,13);
            con_update();
            lineeditmode();
            getline(&getbuf,&size, stdin);
            onecharmode();
            getbuf[strlen(getbuf)-1] = 0;
            if(strlen(getbuf)>16)
              getbuf[16] = 0;
            sprintf(tempstr,"%s%s",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
            sprintf(tempstr2,"%s%s",activepanel->path,getbuf);
            spawnlp(S_WAIT,"mv","-f",tempstr,tempstr2,NULL);
          }
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
        } while(activepanel->xmltreeptr != tempnode);
        if(i) { 
          drawframe(" Renaming Complete ");
          builddir(toppanel);
          builddir(botpanel);
          panelchange();
          panelchange();
          system("sync");
        } else
          drawframe(" You must tag a file or directory to be renamed ");        
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;
      case 'D':
        i = 0;
        drawframe(" Deleting tagged Files and Directories ");
        tempnode = activepanel->xmltreeptr;
        do {
          if(strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), " ")) {
            i++;
            tempstr = (char *)malloc(strlen(activepanel->path)+strlen(XMLgetAttr(activepanel->xmltreeptr,"filename"))+1);
            if(tempstr == NULL)
              memerror();
            sprintf(tempstr,"%s%s",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
            spawnlp(S_WAIT,"rm","-r","-f",tempstr,NULL);
          }
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
        } while(activepanel->xmltreeptr != tempnode);
        if(i) { 
          drawframe(" Deleting Complete ");
          builddir(activepanel);
          system("sync");
        } else
          drawframe(" You must tag a file or directory to be deleted ");        
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;
      case 'm':
        i = 0;
        drawframe(" Moving tagged Files and Directories ");
        tempnode = activepanel->xmltreeptr;
        do {
          if(strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), " ")) {
            i++;
            tempstr = (char *)malloc(strlen(activepanel->path)+strlen(XMLgetAttr(activepanel->xmltreeptr,"filename"))+1);
            if(tempstr == NULL)
              memerror();
            sprintf(tempstr,"%s%s",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
            if(activepanel == toppanel)
              spawnlp(S_WAIT,"mv","-f",tempstr, botpanel->path,NULL);
            else
              spawnlp(S_WAIT,"mv","-f",tempstr, toppanel->path,NULL);
          }
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
        } while(activepanel->xmltreeptr != tempnode);
        if(i) { 
          drawframe(" Moving Complete ");
          builddir(botpanel);
          builddir(toppanel);
          system("sync");
        } else
          drawframe(" You must tag a file or directory to be moved ");        
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;
      case 'c':
        i = 0;
        drawframe(" Copying tagged Files and Directories ");
        tempnode = activepanel->xmltreeptr;
        do {
          if(strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), " ")) {
            i++;
            tempstr = (char *)malloc(strlen(activepanel->path)+strlen(XMLgetAttr(activepanel->xmltreeptr,"filename"))+1);
            if(tempstr == NULL)
              memerror();
            sprintf(tempstr,"%s%s",activepanel->path,XMLgetAttr(activepanel->xmltreeptr,"filename"));
            if(activepanel == toppanel)
              spawnlp(S_WAIT,"cp","-r","-f",tempstr, botpanel->path,NULL);
            else
              spawnlp(S_WAIT,"cp","-r","-f",tempstr, toppanel->path,NULL);
          }
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
        } while(activepanel->xmltreeptr != tempnode);
        if(i) { 
          drawframe(" Copying Complete ");
          if(activepanel == toppanel)
            builddir(botpanel);
          else
            builddir(toppanel);
          system("sync");
        } else
          drawframe(" You must tag a file or directory to be copied ");        
        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        putchar('>');
      break;
    }
    con_update();
  } 

  con_end();
  con_clrscr();
  printf("%s\n",XMLgetAttr(activepanel->xmltreeptr, "filename"));
  con_update();
}
