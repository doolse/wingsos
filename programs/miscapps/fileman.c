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
  fprintf(stderr, "\x1b[0m"); //reset the terminal.
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
  fputc(' ', stderr);
  for(i = 0; i < width-2; i++)
    fprintf(stderr,"_");
  fputc(' ', stderr);

  row++;

  con_gotoxy(startcolumn, row);
  fprintf(stderr," |");
  for(i = 0; i < width-4; i++)
    fprintf(stderr," ");
  fprintf(stderr,"| ");

  row++;

  con_gotoxy(startcolumn, row);
  fprintf(stderr," | %s", string1);

  for(i=0; i<padding1; i++)
    fputc(' ', stderr);

  fprintf(stderr," | ");
    
  row++;

  if(strlen(string2) > 0) {
    con_gotoxy(startcolumn, row);
    fprintf(stderr," | %s", string2);  
    
    for(i=0; i<padding2; i++)
      fputc(' ', stderr);
  
    fprintf(stderr," | ");
  
    row++;
  }
  
  con_gotoxy(startcolumn, row);
  fprintf(stderr," |");
  for(i = 0; i < width-4; i++)
    fprintf(stderr," ");
  fprintf(stderr,"| "); 
    
  row++;

  con_gotoxy(startcolumn, row);
  fprintf(stderr," ");
  for(i = 0; i < width-2; i++) 
    fprintf(stderr,"-");
  fprintf(stderr," ");
      
  con_update();
}
  
void movechardown(int 
x, int y, char c){
  con_gotoxy(x, y);
  fputc(' ', stderr);
  con_gotoxy(x, y+1);
  fputc(c, stderr);   
  con_update();
}   

void movecharup(int x, int y, char c){
  con_gotoxy(x, y);
  fputc(' ',stderr);
  con_gotoxy(x, y-1);
  fputc(c, stderr);  
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

  con_gotoxy(xpos,ypos);
  if(activepanel == toppanel)
    con_setfgbg(COL_White,COL_Red);
  else
    con_setfgbg(COL_White,COL_Blue);
  con_clrline(LC_End);

  for(i = 0;i < (con_xsize - strlen(titlestr))/2; i++) {
    con_gotoxy(xpos, ypos);
    fputc(' ', stderr);
    xpos++;
  }
	
  fprintf(stderr,"%s",titlestr);
  xpos = xpos + strlen(titlestr);

  for(i = 0;i <= (con_xsize - strlen(titlestr))/2; i++) {
    con_gotoxy(xpos, ypos);
    fputc(' ', stderr);
    xpos++;
  }

  ypos = botpanel->firstrow - 1;
  xpos = 0;

  con_gotoxy(xpos,ypos);
  if(activepanel == botpanel)
    con_setfgbg(COL_White,COL_Red);
  else
    con_setfgbg(COL_White,COL_Blue);
  con_clrline(LC_End);

  for(i = 0;i < (con_xsize - strlen(message))/2; i++) {
    con_gotoxy(xpos, ypos);
    fputc(' ', stderr);
    xpos++;
  }
	
  fprintf(stderr,"%s",message);
  xpos = xpos + strlen(message);

  for(i = 0;i <= (con_xsize - strlen(message))/2; i++) {
    con_gotoxy(xpos, ypos);
    fputc(' ', stderr);
    xpos++;
  }

  con_gotoxy(0,botpanel->firstrow+botpanel->totalnumofrows+1);
  con_setfgbg(COL_Black,COL_White);
  con_clrline(LC_End);

  if(extendedview) 
    fprintf(stderr,"(+/-), TAB, SPACE, (r)ename, (m)ove, (c)opy, (D)elete, (S)elect and quit");
  else
    fprintf(stderr,"(+/-), TAB/SPACE (r,m,c,D,S)");

  con_setfgbg(COL_Cyan,COL_Black);
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
  fputc(' ', stderr);

  if(activepanel == toppanel)
    activepanel = botpanel;    
  else 
    activepanel = toppanel;

  con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
  fputc('>', stderr);

  drawframe(" Welcome to the WiNGs File Manager");

  con_setscroll(activepanel->firstrow, activepanel->firstrow+activepanel->totalnumofrows);
}

void drawpanelline(DOMElement * dirptr) {
  long filesize;
  
  if(strcmp(XMLgetAttr(dirptr, "filesize"), " "))
    filesize = strtol(XMLgetAttr(dirptr, "filesize"), NULL, 10);
  else
    filesize = 0;

  if(filesize > 1048576) {
    filesize /= 1048576;
    fprintf(stderr," %s %16s %6ld mb", XMLgetAttr(dirptr, "tag"), XMLgetAttr(dirptr, "filename"), filesize);
  } else if(filesize > 1024) {
    filesize /= 1024;
    fprintf(stderr," %s %16s %6ld kb", XMLgetAttr(dirptr, "tag"), XMLgetAttr(dirptr, "filename"), filesize);
  } else if(filesize > 0) {
    fprintf(stderr," %s %16s %6ld bytes", XMLgetAttr(dirptr, "tag"), XMLgetAttr(dirptr, "filename"), filesize);
  } else 
    fprintf(stderr," %s %16s             %s", XMLgetAttr(dirptr, "tag"), XMLgetAttr(dirptr, "filename"), XMLgetAttr(dirptr, "directory"));

}

void drawpanel(panel * thepan) {
  int i;
  DOMElement * dirptr;

  dirptr = XMLgetNode(thepan->xmldirtree, "entry");

    for(i=thepan->firstrow;i<thepan->firstrow+thepan->totalnumofrows;i++) {
      con_gotoxy(0,i);
      drawpanelline(dirptr);
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
    XMLsetAttr(tempnode, "filename", "Parent (../)");
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

  closedir(dir);  

  //XMLsaveFile(thepan->xmldirtree, "/test/outfile.xml");

  drawpanel(thepan);
}

void launch(panel * thepan) {
  char * ext, * filename, * tempstr, *tempstr2;

  filename = (char *)malloc(17);
  if(filename == NULL)
    memerror();

  tempstr = (char *)malloc(strlen(thepan->path)+18);
  if(tempstr == NULL)
    memerror();

  tempstr2 = NULL;

  sprintf(filename, "%s", XMLgetAttr(thepan->xmltreeptr, "filename"));
  sprintf(tempstr, "%s%s", thepan->path, filename);

  ext = &filename[strlen(filename)-4];

  if(!strcasecmp(ext, ".txt"))
    spawnlp(S_WAIT, "more", tempstr, NULL);
  if(!strcasecmp(ext, ".wav")) {
    //spawnlp(S_WAIT, "wavplay", tempstr, NULL);
    tempstr2 = (char *)malloc(strlen(tempstr) + strlen("wavplay  >/dev/null"));
    sprintf(tempstr2, "wavplay %s >/dev/null", tempstr);
    system(tempstr2);
  } if((!strcasecmp(ext, ".mod")) || (!strcasecmp(ext, ".s3m"))) {
    //spawnlp(S_WAIT, "josmod", "-h", "11000", tempstr, NULL);
    tempstr2 = (char *)malloc(strlen(tempstr) + strlen("josmod -h 11000   >/dev/null"));
    sprintf(tempstr2, "josmod -h 11000 %s >/dev/null", tempstr);
    system(tempstr2);
  }

  free(filename);
  free(tempstr);
  if(tempstr2)
    free(tempstr2);
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
  con_setout(stderr);

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
          fputc('\n',stderr);
          activepanel->xmltreeptr = activepanel->xmltreeptr->NextElem;
          drawpanelline(activepanel->xmltreeptr);
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset-1);
          fputc(' ',stderr);
          con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
          fputc('>',stderr);
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
          fprintf(stderr,"\x1b[1L");
          con_gotoxy(0,activepanel->firstrow);
          activepanel->xmltreeptr = activepanel->xmltreeptr->PrevElem;
          drawpanelline(activepanel->xmltreeptr);
          con_gotoxy(0,activepanel->firstrow+1);
          fputc(' ',stderr);
          con_gotoxy(0,activepanel->firstrow);
          fputc('>',stderr);
        }
      break;

      case 9:  
        // 9 is ASCII for TAB
        panelchange();
      break;

      case ' ':
        if(strcmp(XMLgetAttr(activepanel->xmltreeptr,"tag"), "-")) {
          con_gotoxy(1, activepanel->firstrow+activepanel->cursoroffset);
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr,"tag"), " ")) {
            fputc('*',stderr);
            XMLsetAttr(activepanel->xmltreeptr,"tag","*");
          } else {
            fputc(' ',stderr);
            XMLsetAttr(activepanel->xmltreeptr,"tag"," ");
          }
          con_gotoxy(0, activepanel->firstrow+activepanel->cursoroffset);
          fputc('>',stderr);
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
        } else 
          launch(activepanel);

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        fputc('>',stderr);
      break;

      case 'r':
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
          drawframe(" Press SPACE to tag Files and Directories ");        

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        fputc('>',stderr);
      break;

      case 'D':
        i = 0;
        drawframe(" Deleting tagged Files and Directories ");
        tempnode = activepanel->xmltreeptr;

        do {
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), "*")) {
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
          drawframe(" Press SPACE to tag Files and Directories ");        

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        fputc('>',stderr);
      break;

      case 'm':
        i = 0;
        drawframe(" Moving tagged Files and Directories ");
        tempnode = activepanel->xmltreeptr;

        do {
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), "*")) {
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
          drawframe(" Press SPACE to tag Files and Directories ");        

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        fputc('>',stderr);
      break;
      case 'c':
        i = 0;
        drawframe(" Copying tagged Files and Directories ");
        tempnode = activepanel->xmltreeptr;

        do {
          if(!strcmp(XMLgetAttr(activepanel->xmltreeptr, "tag"), "*")) {
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
          drawframe(" Press SPACE to tag Files and Directories ");        

        con_gotoxy(0,activepanel->firstrow+activepanel->cursoroffset);
        fputc('>',stderr);
      break;
    }
    con_update();
  } 

  con_setout(stdout);
  con_end();
  con_clrscr();
  printf("%s\n",XMLgetAttr(activepanel->xmltreeptr, "filename"));
  con_update();
}
