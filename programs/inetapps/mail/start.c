//Mail V2.x for Wings
#include "mailheader.h"

extern int editserver(DOMElement * server, soundsprofile * soundfiles);

// ***** GLOBAL Variables ***** 

extern DOMElement * configxml; //the root element of the config xml element.
extern soundsprofile * soundsettings;

int  size = 0;
char *buf = NULL;

char *server;           // Server name as text
FILE *fp;               // Main Server connection.
FILE *msgfile;          // global to be piped through web or base64 in a thread
int abookfd;            // addressbook filedescriptor
namelist *abook = NULL; // ptr to array of AddressBook info
char *abookbuf  = NULL; // ptr to Raw AddressBook data buffer

accountprofile * currentaccount; //temp account info for auto mail checker

// Pipes

int writetowebpipe[2];
int readfromwebpipe[2];

int writetobase64pipe[2];
int readfrombase64pipe[2];

int eom = 0;
int pq  = 0;     //printed-quotable encoding
char * pqhexbuf;

char * VERSION     = "2.2";
char   PROGBARCHAR = '*'; 

char * boundary  = NULL;
char * boundary2 = NULL;

int logofg_col,     logobg_col,     serverselectfg_col, serverselectbg_col;
int listfg_col,     listbg_col,     listheadfg_col,     listheadbg_col;
int listmenufg_col, listmenubg_col, messagefg_col,      messagebg_col;

/* msgboxobj functions */

msgboxobj * initmsgboxobj(char * msgline1, char * msgline2, char * msgline3, int showprogress, ulong numofitems) {
  msgboxobj * mb;  
  int numoflines = 0;

  int height;
  int width1, width2, width3;
  int width;

  mb = (msgboxobj *)malloc(sizeof(msgboxobj));

  mb->msgline[0] = NULL;
  mb->msgline[1] = NULL;
  mb->msgline[2] = NULL;

  width1 = strlen(msgline1);
  width2 = strlen(msgline2);
  width3 = strlen(msgline3);

  if(width1) 
    mb->msgline[numoflines++] = msgline1;
  if(width2) 
    mb->msgline[numoflines++] = msgline2;
  if(width3) 
    mb->msgline[numoflines++] = msgline3;

  mb->numoflines = numoflines;

  mb->showprogress     = showprogress;
  mb->numofitems       = numofitems;
  mb->progressposition = 0;

  height = numoflines;
  if(showprogress)
    height += 2;

  height += 2;

  mb->top = (con_ysize - height)/2;
  mb->bottom = mb->top+height;

  width = width1;
  if(width < width2)
    width = width2;
  if(width < width3)
    width = width3;

  mb->left  = (con_xsize - width)/2;
  mb->right = mb->left + (width-1);

  mb->left  -=2;
  mb->right +=2;

  if(showprogress) {
    if(numofitems < width-2)
      mb->progresswidth = numofitems;
    else
      mb->progresswidth = width-2;
  }

  return(mb);
}

void drawmsgboxobj(msgboxobj * mb) {
  int x,y,i, width;
  char * blankline;

  mb->linelength = 0;

  /* Clear the rectangle */

  width = (mb->right - mb->left) +1;

  //allocate maximum width line
  blankline = (char *)malloc(width);
  
  //set string blank
  memset(blankline, ' ',width);
  blankline[width] = '\0';
  
  //clear the rectangle
  for(y=mb->top;y<mb->bottom;y++) {
    con_gotoxy(mb->left,y);
    printf("%s",blankline);
  }

  free(blankline);

  //Draw the frame
  //topline
  for(x=mb->left;x<mb->right;x++) {
    con_gotoxy(x,mb->top);
    putchar('_');
  }
  //botline
  for(x=mb->left+1;x<mb->right;x++) {
    con_gotoxy(x,mb->bottom);
    putchar('_');
  }
  //leftline
  for(y=mb->top+1;y<mb->bottom+1;y++) {
    con_gotoxy(mb->left,y);
    putchar('|');
  }
  //rightline
  for(y=mb->top+1;y<mb->bottom+1;y++) {
    con_gotoxy(mb->right,y);
    putchar('|');
  }

  //Draw Message Lines
  for(i=0;i<mb->numoflines;i++) {
    con_gotoxy(mb->left+2,mb->top+2+i);
    printf("%s",mb->msgline[i]);
  }

  //Draw Blank Progress Bar
  if(mb->showprogress) {
    con_gotoxy(mb->left+2, mb->bottom-1);
    putchar('|');
    con_gotoxy(mb->left+2+mb->progresswidth+1, mb->bottom-1);    
    putchar('|');
  }

  con_update();
}

void updatemsgboxprogress(msgboxobj * mb) {
  int i, x, y;
  int linesize;

  if(mb->progressposition <= mb->numofitems) {
    linesize = (mb->progresswidth * mb->progressposition) / mb->numofitems;

    if(linesize > mb->linelength) {    
      mb->linelength = linesize;

      x = mb->left+3;
      y = mb->bottom-1;
 
      linesize += x;

      for(i=x;i<linesize;i++) {
        con_gotoxy(i,y);
        putchar(PROGBARCHAR);
      }
      con_update();
    }
  } //else
    //printf("error. progposition > numofitems!! %ld > %ld\n", mb->progressposition, mb->numofitems);

    //Ok, that was stupid. Don't print this error. Sometimes the accuracy 
    //is slightly off. It'll only be a few bytes at most, and should be 
    //completely unnoticeable.
}

void incrementprogress(msgboxobj * mb) {
  mb->progressposition++;
  updatemsgboxprogress(mb);
}

void setprogress(msgboxobj * mb,ulong progress) {
  mb->progressposition = progress;
  updatemsgboxprogress(mb);
}

/* END of msgboxobj Functions */

/* Simple Message box call */

void drawmessagebox(char * string1, char * string2, int wait) {
  int width, startcolumn, row, i, padding1, padding2;

  if(strlen(string1) < strlen(string2)) {
    width = strlen(string2);

    if(width > (con_xsize - 6)) {
      width = con_xsize - 6;
      string2[width] = 0;
    }

    padding1 = width - strlen(string1);
    padding2 = 0;
  } else {
    width = strlen(string1);

    if(width > (con_xsize - 6)) {
      width = con_xsize - 6;
      string1[width] = 0;
    }

    padding1 = 0;
    padding2 = width - strlen(string2);
  }
  width = width+6;

  row         = 10;
  startcolumn = (con_xsize - width)/2;

  con_gotoxy(startcolumn, row);

  putchar(' ');
  for(i = 0; i < width-2; i++) 
    putchar('_');
  putchar(' ');

  row++;

  con_gotoxy(startcolumn, row);

  putchar(' ');
  putchar('|');
  for(i = 0; i < width-4; i++)
    putchar(' ');
  putchar('|');
  putchar(' ');
 
  row++;

  con_gotoxy(startcolumn, row);

  putchar(' ');
  printf("| %s", string1);

  for(i=0; i<padding1; i++)
    putchar(' ');

  putchar(' ');
  putchar('|');
  putchar(' ');

  row++;

  if(strlen(string2) > 0) {
    con_gotoxy(startcolumn, row);

    putchar(' ');
    printf("| %s", string2);
 
    for(i=0; i<padding2; i++)
      putchar(' ');

    putchar(' ');
    putchar('|');
    putchar(' ');

    row++;
  }

  con_gotoxy(startcolumn, row);

  putchar(' ');
  putchar('|');
  for(i = 0; i < width-4; i++) 
    putchar('_');
  putchar('|');
  putchar(' ');
  
  row++;

  con_gotoxy(startcolumn, row);

  for(i = 0; i < width; i++)
    putchar(' ');

  con_update();

  if(wait)
    con_getkey();
}

/* END simple messagebox call */

void curleft(int num) {
  printf("\x1b[%dD", num);
  con_update();
}
void curright(int num) {
  printf("\x1b[%dC", num);
  con_update();
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

void prepconsole() {
  struct termios tio;

  con_init();

  gettio(STDOUT_FILENO, &tio);
  tio.flags |= TF_ECHO|TF_ICRLF;
  tio.MIN = 1;
  settio(STDOUT_FILENO, &tio);

  con_clrscr();
  con_update();
}

char * getmyline(int size, int x, int y, int password) {
  int i,count = 0;
  char * linebuf;

  linebuf = (char *)malloc(size+1);

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
      if(password)
        putchar('*');
      else
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

char * getmylinen(int size, int x, int y) {
  int i,count = 0;
  char * linebuf;

  linebuf = (char *)malloc(size+1);

  /*  ASCII Codes

    32 is SPACE
    126 is ~
    47 is /
    8 is DEL

  */

  while(1) {
    i = con_getkey();
    if(i > 47 && i < 58 && count < size) {
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

void setcolors(int fg_col, int bg_col) {
  con_setfg(fg_col);
  con_setbg(bg_col);
  con_update();
}

void drawaddressbookselector(int width, int total, int start) {
  int row, col, i, j;

  col = 30;
  row = 1;

  if(total > 20)
    total = 20;

  con_gotoxy(col,row);
  for(i=0;i<width+4;i++)
    putchar('_');

  for(i=0;i<total;i++) {
    row++;
    con_gotoxy(col,row);

    putchar('|');
    for(j=0;j<width+2;j++) {
      putchar(' ');
    }
    putchar('|');

    con_gotoxy(col+2,row);
    printf("%s %s", abook[start+i].firstname,abook[start+i].lastname);
  }
  row++;
  con_gotoxy(col,row);
  putchar('|');
  for(i=0;i<width+2;i++)
    putchar('_');
  putchar('|');
}

char * selectfromaddressbook() {
  int buflen = 100;
  namelist * abookptr;
  char * ptr, * returnbuf;
  int total, i, returncode, maxwidth, current, start, input;
  int arrowxpos, arrowypos;

  if(abookbuf != NULL)
    free(abookbuf);

  if(abook != NULL)
    free(abook);

  abookbuf = (char *)malloc(buflen);
  if(abookbuf == NULL)
    exit(1);
  
  returncode = sendCon(abookfd, GET_ALL_LIST, NULL, NULL, NULL, abookbuf, buflen);
    
  //The addressbook returns an ERROR code if the buffer was too small,
  //and puts the minimum buffer size needed as ascii in the buffer.
  
  if(returncode == ERROR) {
    buflen = atoi(abookbuf);
    free(abookbuf);
    abookbuf = (char *)malloc(buflen);
    if(abookbuf == NULL)
      exit(1);
    returncode = sendCon(abookfd, GET_ALL_LIST, NULL, NULL, NULL, abookbuf, buflen);
  }

  if(returncode == ERROR) {
    drawmessagebox("An error occurred retrieveing data from the Address Book.", "Press any key.",1);
    return("");
  } 

  //there will be one less comma then there are entries.
  
  total = 0;
  ptr = strstr(abookbuf, ",");
  while(ptr != NULL) {
    total++;
    ptr++;
    ptr = strstr(ptr, ",");
  }
  if(strlen(abookbuf) > 0)
    total++;
    
  abook = (namelist *)malloc(sizeof(namelist) * (total +1));
  ptr = abookbuf;
  abookptr = abook;

  abook[total].use = -1;

  maxwidth = 0;

  while(ptr != NULL && ptr != 0) {
    abookptr->use = 0;
    abookptr->firstname = ptr;
    ptr = strstr(ptr, " ");
    *ptr = 0;
    ptr++;
    abookptr->lastname = ptr;

    ptr = strstr(ptr, ",");
    if(ptr != NULL) {
      *ptr = 0;
      ptr++;
      if(strlen(abookptr->lastname) + strlen(abookptr->firstname) > maxwidth)
        maxwidth = strlen(abook->lastname) + strlen(abookptr->firstname);
      abookptr++;
    } else {
      if(strlen(abookptr->lastname) + strlen(abookptr->firstname) > maxwidth)
        maxwidth = strlen(abook->lastname) + strlen(abookptr->firstname);
    }

  }

  current = 0;
  start = current;
  maxwidth = maxwidth+2;

  arrowxpos = 31;
  arrowypos = 2;

  drawaddressbookselector(maxwidth, total, start);

  con_gotoxy(arrowxpos,arrowypos);
  putchar('>');  

  con_update();
  input = 0;  

  while(input != '\n' && input != '\r') {

    input = con_getkey();

    switch(input) {
      case CURD:
        if(current < total-1) {
          if(arrowypos < 21) {
            movechardown(arrowxpos, arrowypos, '>');
            arrowypos++;
          } else {
            start++;
            drawaddressbookselector(maxwidth, total, start);
            con_gotoxy(arrowxpos, arrowypos);
            putchar('>');
            con_update();
          }
          current++;
        }
      break;
      case CURU:
        if(current > 0) {
          if(arrowypos > 2) {
            movecharup(arrowxpos,arrowypos,'>');
            arrowypos--;
          } else {
            start--;
            drawaddressbookselector(maxwidth, total, start);
            con_gotoxy(arrowxpos, arrowypos);
            putchar('>');
            con_update();
          }
          current--;
        }
      break;
    }   
  }

  returnbuf = (char *)malloc(50);

  returncode = sendCon(abookfd,GET_ATTRIB,abook[current].lastname,abook[current].firstname,"email",returnbuf,50);  

  if(returncode == NOENTRY || returncode == ERROR)
    return("");
  else
    return(returnbuf);
}

void composescreendraw(char * to, char * subject, msgline * cc, int cccount, msgline * bcc, int bcccount, msgline * attach, int attachcount, int typeofcompose) {
  int i, bodylines;
  int row = 0;  
  char * ptr;
  msgline * ccptr, *bccptr, *attachptr;
  char * headerstr, * footerstr;

  switch(typeofcompose) {
    case COMPOSENEW:
      headerstr = strdup("COMPOSE NEW");
      footerstr = strdup("inbox");
    break;
    case REPLY:
      headerstr = strdup("REPLY");
      footerstr = strdup("inbox");
    break;
    case REPLYCONTINUED:
      headerstr = strdup("REPLY CONT");
      footerstr = strdup("drafts");
    break;
    case COMPOSECONTINUED:
      headerstr = strdup("COMPOSE CONT");
      footerstr = strdup("drafts");
    break;
    case RESEND:
      headerstr = strdup("RESEND");
      footerstr = strdup("sent box");
    break;    
  } 

  ccptr = cc;
  bccptr = bcc;
  attachptr = attach;

  con_clrscr();
  con_gotoxy(0,row);
  printf("___|Modify Options:|_____________________________/ Mail v%s - %s", VERSION, headerstr);
  row++;
  con_gotoxy(0,row);
  putchar('|');
  row++;
  con_gotoxy(0,row);

  if(strlen(to))
    printf("|     (a/e)       To: [ %s ]", to);
  else
    printf("|     (a/e)       To: [ ]");

  row++;
  con_gotoxy(0,row);

  if(strlen(subject))
    printf("|     (e)    Subject: [ %s ]",subject);  
  else
    printf("|     (e)    Subject: [ ]");  

  //deal with multiple CC's. 
  row++;
  con_gotoxy(0,row);
  if(cccount < 1) {
    printf("|     (a/e/r)     CC: [ ]");
  } else {
    printf("|     (a/e/r)     CC: [ %s ]", ccptr->line);    
  }
  if(cccount > 1) {
    for(i=1; i<cccount; i++) {
      ccptr = ccptr->nextline;
      row++;
      con_gotoxy(0,row);
      printf("|     (a/e/r)         [ %s ]", ccptr->line);
    }
  }

  //deal with multiple BCC's. (handled the EXACT same way as CC's)
  row++;
  con_gotoxy(0,row);
  if(bcccount < 1) {
    printf("|     (a/e/r)    BCC: [ ]");
  } else {
    printf("|     (a/e/r)    BCC: [ %s ]", bccptr->line);    
  }
  if(bcccount > 1) {
    for(i=1; i<bcccount; i++) {
      bccptr = bccptr->nextline;
      row++;
      con_gotoxy(0,row);
      printf("|     (a/e/r)         [ %s ]", bccptr->line);
    }
  }

  //deal with multiple attachs's. (handled the EXACT same way as CC's)
  row++;
  con_gotoxy(0,row);
  if(attachcount < 1) {
    printf("|     (a/r)   Attach: [ ]");
  } else {
    printf("|     (a/r)   Attach: [ %s ]", attachptr->line);    
  }
  if(attachcount > 1) {
    for(i=1; i<attachcount; i++) {
      attachptr = attachptr->nextline;
      row++;
      con_gotoxy(0,row);
      printf("|     (a/r)           [ %s ]", attachptr->line);
    }
  }

  //finally draw the header closing line. 

  row++;
  con_gotoxy(0,row);
  printf("|_______________________________________________________________________________");

  row++;
  con_gotoxy(2,row);
  printf(" (return) Edit Body Contents");

  //and the commands help line at the bottom.
  con_gotoxy(1,24);
  printf(" (Q)uit to %s, (a)ddressbook OR (a)ttach, (e)dit, (r)emove", footerstr);
}

void freemsgpreview(msgline * currentline) {
  msgline * templine;

  //first go back to start of list, if possible.

  while(currentline->prevline)
    currentline = currentline->prevline;

  //then systematically free all parts of the list to the end. 

  while(currentline != NULL) {
    templine = currentline->nextline;
    free(currentline->line);
    free(currentline);
    currentline = templine;
  }
}

msgline * buildmsgpreview(FILE * msgfile) {
  char * line, * lineptr;
  char c;
  int charcount, eom;
  msgline * thisline, * templine, * firstline;

  line = (char *)malloc(81);
  eom = 0;
  thisline = (msgline *)malloc(sizeof(msgline));

  thisline->prevline = NULL;
  thisline->nextline = NULL;
  firstline = thisline;

  while(!eom) {
    lineptr = line;
    charcount = 0;

    while(charcount < 80) {
      c = fgetc(msgfile);

      switch(c) {
        case '\n':
          charcount = 80;
        break;
        case EOF:
          eom = 1;
          charcount = 80;
        break;
        default:
          charcount++;
          *lineptr = c;
          lineptr++;
      }
    }
    *lineptr = 0;
    thisline->line = strdup(line);
    if(!eom) {
      templine = (msgline *)malloc(sizeof(msgline));
      templine->prevline = thisline;
      templine->nextline = NULL;
      thisline->nextline = templine;
      thisline = templine;
    }
  }
  return(firstline);
}

msgline * drawmsgpreview(msgline * firstline, int cccount, int bcccount, int attachcount) {
  msgline * lastline;
  int upperscrollrow, i;

  if(firstline != NULL) {

    // starting position without duplicates of cc, bcc, or attachments

    upperscrollrow = 9; 
    if(bcccount)
      upperscrollrow += (bcccount - 1);
    if(cccount)
      upperscrollrow += (cccount -1);
    if(attachcount)
      upperscrollrow += (attachcount -1);
 
    con_setscroll(upperscrollrow,24);
    lastline = firstline;

    for(i=0;i<(24-upperscrollrow);i++) {
      con_gotoxy(0,(upperscrollrow+i));
      printf("%s", lastline->line);
      if(!lastline->nextline)
         break;
      lastline = lastline->nextline;
    }            
  } else 
    lastline = NULL;

  return(lastline);
}

void sendmail(msgline * firstcc, int cccount, msgline * firstbcc, int bcccount, msgline * firstattach, int attachcount, char * to, char * subject, char * messagefile, char * fromname, char * returnaddress) {
  char * argarray[21];
  int tempstrlen, resultcode, input;
  char * ccstring, * bccstring, * attachstring, * smtpserver;
  msgline * ccptr, * bccptr, *attachptr;
  char * buf = NULL;
  int size = 0;
  int i;

  tempstrlen = 1;
  ccstring   = NULL;
  ccptr      = firstcc;

  if(cccount) {
    do {
      tempstrlen += strlen(ccptr->line) + 1;
      ccptr = ccptr->nextline;
    } while(ccptr);
  }

  if(tempstrlen > 1) {
    ccstring = (char *)malloc(tempstrlen+1);

    ccptr = firstcc;
    *ccstring = 0;
    do {
      sprintf(ccstring, "%s%s,",ccstring, ccptr->line);
      ccptr = ccptr->nextline;
    } while(ccptr);

    ccstring[strlen(ccstring)-1] = 0;
  }

  tempstrlen = 1;
  bccstring  = NULL;
  bccptr     = firstbcc;
        
  if(bcccount) {
    do {
      tempstrlen += strlen(bccptr->line) + 1;
      bccptr = bccptr->nextline;
    } while(bccptr);
  }

  if(tempstrlen > 1) {
    bccstring = (char *)malloc(tempstrlen+1);

    bccptr = firstbcc;
    *bccstring = 0;
    do {
      sprintf(bccstring, "%s%s,",bccstring, bccptr->line);
      bccptr = bccptr->nextline;
    } while(bccptr);

    bccstring[strlen(bccstring)-1] = 0;
  }

  tempstrlen    = 1;
  attachstring  = NULL;
  attachptr     = firstattach;
        
  if(attachcount) {
    do {
      tempstrlen += strlen(attachptr->line) + 1;
      attachptr = attachptr->nextline;
    } while(attachptr);
  }

  if(tempstrlen > 1) {
    attachstring = (char *)malloc(tempstrlen+1);

    attachptr = firstattach;
    *attachstring = 0;
    do {
      sprintf(attachstring, "%s%s,",attachstring, attachptr->line);
      attachptr = attachptr->nextline;
    } while(attachptr);

    attachstring[strlen(attachstring)-1] = 0;
  }

  //spawnvp our saviour. Takes an array of args.

  i = 0;
  
  argarray[i++] = "qsend";
  argarray[i++] = "-q"; 
  argarray[i++] = "-s";
  argarray[i++] = subject;
  argarray[i++] = "-t";
  argarray[i++] = to;
  argarray[i++] = "-m";
  argarray[i++] = messagefile;
  argarray[i++] = "-f";
  argarray[i++] = fromname;
  argarray[i++] = "-F";
  argarray[i++] = returnaddress;  

  if(ccstring) {
    argarray[i++] = "-C";
    argarray[i++] = ccstring;
  }

  if(bccstring) {
    argarray[i++] = "-B";
    argarray[i++] = bccstring;
  }

  if(attachstring) {
    argarray[i++] = "-a";
    argarray[i++] = attachstring;
  }
  
  argarray[i] = NULL;

  resultcode = spawnvp(S_WAIT, argarray);

  if(resultcode == EXIT_SUCCESS)
    drawmessagebox("Message Delivered! And stored in sent box.","Press any key to continue.",1);

  else if(resultcode == EXIT_NOCONFIG) {
    drawmessagebox("Qsend has not been configured.","Configure it now? (y/n)",0);
    input = 's';
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'y') {
      spawnlp(S_WAIT, "qsend","-c",NULL);
      resultcode = spawnvp(S_WAIT, argarray);
    }
  }

  input = 's';
  while(resultcode == EXIT_BADSERVER) {
    drawmessagebox("SMTP Server is inaccessable.","Use an alternative SMTP server? (y/n)",0);
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'n') 
      break;
    drawmessagebox("SMTP Server address:"," ",0);
    if(smtpserver)
      free(smtpserver);
    smtpserver = getmyline(20,30,13,0);

    argarray[i] = strdup("-S");
    argarray[i+1] = smtpserver;
    argarray[i+2] = NULL;

    resultcode = spawnvp(S_WAIT, argarray);
    if(resultcode == EXIT_SUCCESS)
      drawmessagebox("Message Delivered.","Press a key to continue.",1);
  }

  input = 's';
  while(resultcode == EXIT_NORELAY) {
    drawmessagebox("Relaying on this server denied.","Use an alternative SMTP server? (y/n)",0);
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'n') 
      break;
    drawmessagebox("SMTP Server address:"," ",0);
    if(smtpserver)
      free(smtpserver);
    smtpserver = getmyline(20,30,13,0);

    argarray[i] = strdup("-S");
    argarray[i+1] = smtpserver;
    argarray[i+2] = NULL;

    resultcode = spawnvp(S_WAIT, argarray);
    if(resultcode == EXIT_SUCCESS)
      drawmessagebox("Message Delivered.","Press a key to continue.",1);
  }

  if(resultcode == EXIT_BADADDRESS)
    drawmessagebox("\"To\" field contains an invalide email address.","Message transfered to sent box undelivered.",1);

  if(resultcode == EXIT_FAILURE) 
    drawmessagebox("An error has occurred. Message was not delivered.","Check to make sure you have Qsend, and that it is configured properly.",1);

  if(ccstring)
    free(ccstring);
  if(bccstring)
    free(bccstring);
  if(attachstring)
    free(attachstring);
}

void savetosent(DOMElement * sentxml, char * serverpath, char * to, char * subject, msgline * firstcc, int cccount, msgline * firstbcc, int bcccount, msgline * firstattach, int attachcount, int typeofcompose) {
  char * indexfilepath = NULL;
  char * tempfilestr, * destfilestr, *tempstr;
  DOMElement * activeelemptr, * tempelemptr;
  int tempint;
  msgline * msglineptr;

  if(typeofcompose != RESEND) { 
    indexfilepath = (char *)malloc(strlen(serverpath) + strlen("sent/index.xml") +1);
    sprintf(indexfilepath, "%ssent/index.xml", serverpath);
    sentxml = XMLloadFile(indexfilepath);
  }

  activeelemptr = XMLgetNode(sentxml, "xml/messages");

  tempint = atoi(XMLgetAttr(activeelemptr, "refnum"));
  tempint++;

  destfilestr = (char *)malloc(strlen(serverpath) + strlen("sent/")+20);
  tempfilestr = (char *)malloc(strlen(serverpath) + strlen("drafts/temporary.txt")+1);

  sprintf(destfilestr, "%ssent/%d", serverpath, tempint);
  sprintf(tempfilestr, "%sdrafts/temporary.txt", serverpath);
            
  spawnlp(S_WAIT,"mv", "-f", tempfilestr, destfilestr, NULL);            
    
  free(tempfilestr);
  free(destfilestr);

  tempstr = (char *)malloc(17);
  sprintf(tempstr, "%d", tempint);

  XMLsetAttr(activeelemptr, "refnum", tempstr);

  tempelemptr = XMLnewNode(NodeType_Element, "message", "");
  
  XMLsetAttr(tempelemptr, "to", to);
  XMLsetAttr(tempelemptr, "subject", subject);
  XMLsetAttr(tempelemptr, "fileref", tempstr);

  free(tempstr);

  XMLsetAttr(tempelemptr, "status", " ");
        
  XMLinsert(activeelemptr, NULL, tempelemptr); 

  //insert the cc, bcc, and attach's as child nodes. 
       
  msglineptr = firstcc;
  for(tempint = 0; tempint < cccount; tempint++) {
    activeelemptr = XMLnewNode(NodeType_Element, "cc", "");
    XMLinsert(tempelemptr, NULL, activeelemptr);
    XMLsetAttr(activeelemptr, "address", msglineptr->line);
    msglineptr = msglineptr->nextline;
  }

  msglineptr = firstbcc;
  for(tempint = 0; tempint < bcccount; tempint++) {
    activeelemptr = XMLnewNode(NodeType_Element, "bcc", "");
    XMLinsert(tempelemptr, NULL, activeelemptr);
    XMLsetAttr(activeelemptr, "address", msglineptr->line);
    msglineptr = msglineptr->nextline;
  }

  msglineptr = firstattach;
  for(tempint = 0; tempint < attachcount; tempint++) {
    activeelemptr = XMLnewNode(NodeType_Element, "attach", "");
    XMLinsert(tempelemptr, NULL, activeelemptr);
    XMLsetAttr(activeelemptr, "file", msglineptr->line);
    msglineptr = msglineptr->nextline;
  }

  if(indexfilepath) {
    XMLsaveFile(sentxml, indexfilepath);
    free(indexfilepath);
  }
}

void savetodrafts(DOMElement * draftxml, char * serverpath, char * to, char * subject, msgline * firstcc, int cccount, msgline * firstbcc, int bcccount, msgline * firstattach, int attachcount, int typeofcompose) {
  char * indexfilepath = NULL;
  char * tempfilestr, * destfilestr, *tempstr;
  DOMElement * activeelemptr, * tempelemptr;
  int tempint;
  msgline * msglineptr;

  if(typeofcompose == REPLY || 
     typeofcompose == COMPOSENEW ||
     typeofcompose == RESEND) {

    indexfilepath = (char *)malloc(strlen(serverpath) + strlen("drafts/index.xml") +1);
    sprintf(indexfilepath, "%sdrafts/index.xml", serverpath);
    draftxml = XMLloadFile(indexfilepath);
  }

  activeelemptr = XMLgetNode(draftxml, "xml/messages");

  tempint = atoi(XMLgetAttr(activeelemptr, "refnum"));
  tempint++;

  tempfilestr = (char *)malloc(strlen(serverpath) + strlen("drafts/temporary.txt")+1);
  destfilestr = (char *)malloc(strlen(serverpath) + strlen("drafts/")+20);

  sprintf(tempfilestr, "%sdrafts/temporary.txt",serverpath);
  sprintf(destfilestr, "%sdrafts/%d", serverpath, tempint);
            
  spawnlp(S_WAIT,"mv","-f",tempfilestr,destfilestr,NULL);            
    
  free(tempfilestr);
  free(destfilestr);

  tempstr = (char *)malloc(17);
  sprintf(tempstr, "%d", tempint);

  XMLsetAttr(activeelemptr, "refnum", tempstr);

  tempelemptr = XMLnewNode(NodeType_Element, "message", "");

  XMLsetAttr(tempelemptr, "to", to);
  XMLsetAttr(tempelemptr, "subject", subject);
  XMLsetAttr(tempelemptr, "fileref", tempstr);

  free(tempstr);

  if(typeofcompose == REPLY || typeofcompose == REPLYCONTINUED)
    XMLsetAttr(tempelemptr, "status", "R");
  else if(typeofcompose == COMPOSENEW || typeofcompose == COMPOSECONTINUED)
    XMLsetAttr(tempelemptr, "status", "C");
           
  XMLinsert(activeelemptr, NULL, tempelemptr); 

  //if any cc's bcc's or attachment's add save them to the xml index

  msglineptr = firstcc;
  for(tempint = 0; tempint < cccount; tempint++) {
    activeelemptr = XMLnewNode(NodeType_Element, "cc", "");
    XMLinsert(tempelemptr, NULL, activeelemptr);
    XMLsetAttr(activeelemptr, "address", msglineptr->line);
    msglineptr = msglineptr->nextline;
  }

  msglineptr = firstbcc;
  for(tempint = 0; tempint < bcccount; tempint++) {
    activeelemptr = XMLnewNode(NodeType_Element, "bcc", "");
    XMLinsert(tempelemptr, NULL, activeelemptr);
    XMLsetAttr(activeelemptr, "address", msglineptr->line);
    msglineptr = msglineptr->nextline;
  }

  msglineptr = firstattach;
  for(tempint = 0; tempint < attachcount; tempint++) {
    activeelemptr = XMLnewNode(NodeType_Element, "attach", "");
    XMLinsert(tempelemptr, NULL, activeelemptr);
    XMLsetAttr(activeelemptr, "file", msglineptr->line);
    msglineptr = msglineptr->nextline;
  }

  if(indexfilepath) {
    XMLsaveFile(draftxml, indexfilepath);
    free(indexfilepath);
  }
}

void compose(DOMElement * server,DOMElement * indexxml, char * serverpath, char * to, char * subject, msgline * firstcc, int cccount, msgline * firstbcc, int bcccount, msgline * firstattach, int attachcount, int typeofcompose) {
  FILE * composefile, * incoming;
  char * tempfilestr;
  msgline * curcc, * curbcc, * curattach, * msglineptr;
  msgline * firstline, * lastline;
  int section, arrowxpos, arrowypos, refresh, upperscrollrow, tempint, input;

  //For "section" see DEFINEs. 

  firstline = NULL;
  section   = 0;

  curcc     = firstcc;
  curbcc    = firstbcc;
  curattach = firstattach;

  tempfilestr = (char *)malloc(strlen(serverpath) + strlen("drafts/temporary.txt") + 1);
  sprintf(tempfilestr,"%sdrafts/temporary.txt", serverpath);

  composefile = fopen(tempfilestr, "r");
  if(composefile) {
    firstline = buildmsgpreview(composefile);          
    fclose(composefile);     
  }
  
  composescreendraw(to, subject,firstcc,cccount,firstbcc,bcccount,firstattach,attachcount,typeofcompose);
  lastline = drawmsgpreview(firstline, cccount, bcccount, attachcount);

  arrowxpos = 1;
  arrowypos = 2;

  upperscrollrow = 9;
  if(bcccount)
    upperscrollrow += (bcccount - 1);
  if(cccount)
    upperscrollrow += (cccount -1);
  if(attachcount)
    upperscrollrow += (attachcount -1);

  con_gotoxy(arrowxpos,arrowypos);
  putchar('>');

  input = 0;
  while(input != 'Q') {
    con_update();
    refresh = 1;

    input = con_getkey();

    switch(input) {
      case CURD:
        switch(section) {
          case TO:
            movechardown(arrowxpos,arrowypos, '>');
            arrowypos++;
            section++;
          break;
          case SUBJECT:
            movechardown(arrowxpos,arrowypos, '>');
            arrowypos++;
            section++;
          break;
          case CC:
            movechardown(arrowxpos,arrowypos, '>');
            arrowypos++;
            if(curcc == NULL || curcc->nextline == NULL)
              section++;
            else {
              curcc = curcc->nextline;
            }
          break;
          case BCC:
            movechardown(arrowxpos,arrowypos, '>');
            arrowypos++;
            if(curbcc == NULL || curbcc->nextline == NULL)
              section++;
            else {
              curbcc = curbcc->nextline;
            }
          break;
          case ATTACH:
            if(curattach == NULL || curattach->nextline == NULL) {
              section++;
              con_gotoxy(arrowxpos, arrowypos);
              putchar(' ');
              arrowypos += 2;
              con_gotoxy(arrowxpos, arrowypos);
              putchar('>');
              con_update();
            } else {
              movechardown(arrowxpos,arrowypos, '>');
              arrowypos++;
              curattach = curattach->nextline;
            }
          break;
          case BODY:
            if(lastline && lastline->nextline) {
              firstline = firstline->nextline;
              lastline = lastline->nextline;
              con_gotoxy(0, 23);
              putchar('\n');
              con_gotoxy(0, 23);
              printf("%s", lastline->line);
              con_update();
            }
          break;
        }
        refresh = 0;
      break;
      case CURU:
        switch(section) {
          case SUBJECT:
            movecharup(arrowxpos,arrowypos, '>');
            arrowypos--;
            section--;
          break;
          case CC:
            movecharup(arrowxpos,arrowypos, '>');
            arrowypos--;
            if(curcc == NULL || curcc->prevline == NULL)
              section--;
            else
              curcc = curcc->prevline;
          break;
          case BCC:
            movecharup(arrowxpos,arrowypos, '>');
            arrowypos--;
            if(curbcc == NULL || curbcc->prevline == NULL)
              section--;
            else
              curbcc = curbcc->prevline;
          break;
          case ATTACH:
            movecharup(arrowxpos,arrowypos, '>');
            arrowypos--;
            if(curattach == NULL || curattach->prevline == NULL)
              section--;
            else
              curattach = curattach->prevline;
          break;
          case BODY:
            if(firstline && firstline->prevline) {
              firstline = firstline->prevline;
              lastline = lastline->prevline;
              con_gotoxy(0,upperscrollrow);
              printf("\x1b[1L");
              printf("%s", firstline->line);
              con_update();
            } else {
              con_gotoxy(arrowxpos,arrowypos);
              putchar(' ');
              arrowypos -= 2;
              con_gotoxy(arrowxpos,arrowypos);
              putchar('>');
              con_update();
              section--;
            }
          break;
        }
        refresh = 0;
      break;

      //addressbook or attachment
      case 'a':
        switch(section) {
          case TO:
            to = strdup(selectfromaddressbook());
          break;

          case CC:
            msglineptr = (msgline *)malloc(sizeof(msgline));

            if(curcc == NULL) {
              firstcc = msglineptr;
              msglineptr->prevline = NULL;
              msglineptr->nextline = NULL;
            } else {
              if(curcc->nextline != NULL) {
                curcc->nextline->prevline = msglineptr;
              }
              msglineptr->nextline = curcc->nextline;
              curcc->nextline = msglineptr;
              msglineptr->prevline = curcc;
              arrowypos++;
            }            

            curcc = msglineptr;              
            curcc->line = strdup(selectfromaddressbook());
            cccount++;
          break;

          case BCC:
            msglineptr = (msgline *)malloc(sizeof(msgline));

            if(curbcc == NULL) {
              firstbcc = msglineptr;
              msglineptr->prevline = NULL;
              msglineptr->nextline = NULL;
            } else {
              if(curbcc->nextline != NULL) {
                curbcc->nextline->prevline = msglineptr;
              }
              msglineptr->nextline = curbcc->nextline;
              curbcc->nextline = msglineptr;
              msglineptr->prevline = curbcc;
              arrowypos++;
            }            

            curbcc = msglineptr;              
            curbcc->line = strdup(selectfromaddressbook());
            bcccount++;
          break;

          case ATTACH:
            msglineptr = (msgline *)malloc(sizeof(msgline));

            if(curattach == NULL) {
              firstattach = msglineptr;
              msglineptr->prevline = NULL;
              msglineptr->nextline = NULL;
            } else {
              if(curattach->nextline != NULL) {
                curattach->nextline->prevline = msglineptr;
              }
              msglineptr->nextline = curattach->nextline;
              curattach->nextline = msglineptr;
              msglineptr->prevline = curattach;
              arrowypos++;
            }            


            con_end();    

            //add attachment from fileman. 
            spawnlp(S_WAIT, "fileman", NULL);

            prepconsole();

            //the temp file is heinous and bad. but until I figure out
            //how to do it with pipes, a temp file it shall remain.

            incoming = fopen("/wings/attach.tmp", "r");
            getline(&buf, &size, incoming);
            fclose(incoming);

            //may as well get rid of the temp file so we don't cause a mess

            unlink("/wings/attach.tmp");

            curattach = msglineptr;              
            curattach->line = strdup(buf);
            attachcount++;
          break;
          default:
            refresh = 0;
        }
      break;

      //edit field manually
      case 'e':
        switch(section) {
          case TO:
            drawmessagebox("To:","                                ",0);
            if(to)
              free(to);
            to = getmyline(32,24,13,0);
          break;
          case SUBJECT:
            drawmessagebox("Subject:","                                        ",0);
            if(subject)
              free(subject);
            subject = getmyline(40,20,13,0); 
          break;
          case CC:
            if(curcc != NULL) {
              drawmessagebox("Edit this CC:","                                ",0);
              if(curcc->line)
                free(curcc->line);
              curcc->line = getmyline(32,24,13,0);
            } else {
              msglineptr = (msgline *)malloc(sizeof(msgline));

              firstcc = msglineptr;
              msglineptr->prevline = NULL;
              msglineptr->nextline = NULL;

              curcc = msglineptr;              
              drawmessagebox("Edit new CC:","                                ",0);
              curcc->line = getmyline(32,24,13,0);
              cccount++;
            }
          break;
          case BCC:
            if(curbcc != NULL) {
              drawmessagebox("Edit this BCC:","                                ",0);
              if(curbcc->line)
                free(curbcc->line);
              curbcc->line = getmyline(32,24,13,0);
            } else {
              msglineptr = (msgline *)malloc(sizeof(msgline));

              firstbcc = msglineptr;
              msglineptr->prevline = NULL;
              msglineptr->nextline = NULL;

              curbcc = msglineptr;              
              drawmessagebox("Edit new BCC:","                                ",0);
              curbcc->line = getmyline(32,24,13,0);
              bcccount++;
            }
          break;
          default:
            refresh = 0;
        }        
      break;

      //remove field ... if applicable. only for CC and BCC
      case 'r':
        switch(section) {
          case CC:
            if(curcc != NULL) {
              if(curcc->prevline == NULL && curcc->nextline == NULL) {
                msglineptr = curcc;
                curcc = NULL;
                firstcc = NULL;
                //Arrowposition does not move. 
              } else if(curcc->prevline != NULL && curcc->nextline == NULL) {
                msglineptr = curcc;
                curcc = msglineptr->prevline;
                curcc->nextline = NULL;
                //arrowposition moves up one row.
                arrowypos--;
              } else if(curcc->prevline != NULL && curcc->nextline != NULL) {
                msglineptr = curcc;
                curcc->prevline->nextline = curcc->nextline;
                curcc->nextline->prevline = curcc->prevline;
                curcc = curcc->nextline;
                //arrowposition does not move.
              } else if(curcc->prevline == NULL && curcc->nextline != NULL) {
                msglineptr = curcc;
                curcc = curcc->nextline;
                curcc->prevline = NULL;
                firstcc = curcc;
                //arrowposition does not move.
              }
              free(msglineptr);
              cccount--;
            }
          break;
          case BCC:
            if(curbcc != NULL) {
              if(curbcc->prevline == NULL && curbcc->nextline == NULL) {
                msglineptr = curbcc;
                curbcc = NULL;
                firstbcc = NULL;
                //Arrowposition does not move. 
              } else if(curbcc->prevline != NULL && curbcc->nextline == NULL) {
                msglineptr = curbcc;
                curbcc = msglineptr->prevline;
                curbcc->nextline = NULL;
                //arrowposition moves up one row.
                arrowypos--;
              } else if(curbcc->prevline != NULL && curbcc->nextline != NULL) {
                msglineptr = curbcc;
                curbcc->prevline->nextline = curbcc->nextline;
                curbcc->nextline->prevline = curbcc->prevline;
                curbcc = curbcc->nextline;
                //arrowposition does not move.
              } else if(curbcc->prevline == NULL && curbcc->nextline != NULL) {
                msglineptr = curbcc;
                curbcc = curbcc->nextline;
                curbcc->prevline = NULL;
                firstbcc = curbcc;
                //arrowposition does not move.
              }
              free(msglineptr);
              bcccount--;
            }
          break;

          case ATTACH:
            if(curattach != NULL) {
              if(curattach->prevline == NULL && curattach->nextline == NULL) {
                msglineptr   = curattach;
                curattach   = NULL;
                firstattach = NULL;
                //Arrowposition does not move. 
              } else if(curattach->prevline != NULL && curattach->nextline == NULL) {
                msglineptr = curattach;
                curattach = msglineptr->prevline;
                curattach->nextline = NULL;
                //arrowposition moves up one row.
                arrowypos--;
              } else if(curattach->prevline != NULL && curattach->nextline != NULL) {
                msglineptr = curattach;
                curattach->prevline->nextline = curattach->nextline;
                curattach->nextline->prevline = curattach->prevline;
                curattach = curattach->nextline;
                //arrowposition does not move.
              } else if(curattach->prevline == NULL && curattach->nextline != NULL) {
                msglineptr = curattach;
                curattach = curattach->nextline;
                curattach->prevline = NULL;
                firstattach = curattach;
                //arrowposition does not move.
              }
              free(msglineptr);
              attachcount--;
            }
          break;
          default:
            refresh = 0;
        }
      break;

      //Edit in ned... press return while section == BODY;

      case '\n':
        if(section == BODY) {
          con_end();

          //open the tempfile with ned.
          spawnlp(S_WAIT, "ned", tempfilestr, NULL);

          prepconsole();

          if(firstline)
            freemsgpreview(firstline);

          composefile = fopen(tempfilestr, "r");

          firstline = buildmsgpreview(composefile);

          fclose(composefile);     
        }
      break;

      //If they push a key that does nothing, don't refresh.
      
      default:
        refresh = 0;      
    }
    if(refresh) {
 
      composescreendraw(to, subject, firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, typeofcompose);
      lastline = drawmsgpreview(firstline, cccount, bcccount, attachcount);

      upperscrollrow = 9;
      if(bcccount)
        upperscrollrow += (bcccount - 1);
      if(cccount)
        upperscrollrow += (cccount -1);
      if(attachcount)
        upperscrollrow += (attachcount -1);

      con_gotoxy(arrowxpos,arrowypos);
      putchar('>');
    }

    refresh = 1;
    con_update();
  }

  drawmessagebox("Options:", "(d)eliver message, (s)ave to send later, (A)bandon message",0);

  input = 0;
  while(input != 'd' && input != 's' && input != 'A') {
    input = con_getkey();  

    switch(input) {
      case 'd':
        sendmail(firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, to, subject, tempfilestr, XMLgetAttr(server, "fromname"), XMLgetAttr(server, "returnaddress"));
        savetosent(indexxml, serverpath, to, subject, firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, typeofcompose);
        break;

      case 's':
        savetodrafts(indexxml, serverpath, to, subject, firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, typeofcompose);
        break;

      case 'A':
        //just delete the file serverpath/drafts/temporary.txt
        spawnlp(0,"rm",tempfilestr,NULL);
        break;
    }
  } 

  free(tempfilestr);
}

int makeserverinbox(char * server) {
  char * path;
  char * tempstr;
  int mkdirresult; 

  if(strlen(server) > 16)
    server[16] = 0;
 
  path    = fpathname("data/servers/", getappdir(), 1);
  tempstr = (char *)malloc(strlen(path)+strlen(server)+strlen("/drafts")+2);

  sprintf(tempstr, "%s%s", path, server);
  mkdirresult = mkdir(tempstr, 0);

  if(mkdirresult != -1) {
    sprintf(tempstr, "%s%s/drafts", path, server);
    mkdir(tempstr, 0);
    sprintf(tempstr, "%s%s/sent", path, server);
    mkdir(tempstr, 0);
  } 

  free(tempstr);

  return(mkdirresult);
}

void makenewmessageindex(char * server) {
  FILE * messageindex;
  char * tempstr;

  if(strlen(server) > 16)
    server[16] = 0;
 
  tempstr = (char *)malloc(strlen("data/servers//drafts/index.xml")+strlen(server)+2);

  //create inbox xml index file
  sprintf(tempstr, "data/servers/%s/index.xml", server);
  messageindex = fopen(fpathname(tempstr, getappdir(), 1), "w");
  fprintf(messageindex, "<xml><messages firstnum=\"1\" refnum=\"0\"></messages></xml>");
  fclose(messageindex);

  //create drafts xml index file
  sprintf(tempstr, "data/servers/%s/drafts/index.xml", server);
  messageindex = fopen(fpathname(tempstr, getappdir(), 1), "w");
  fprintf(messageindex, "<xml><messages refnum=\"0\"></messages></xml>");
  fclose(messageindex);

  //create sent mail xml index file
  sprintf(tempstr, "data/servers/%s/sent/index.xml", server);
  messageindex = fopen(fpathname(tempstr, getappdir(), 1), "w");
  fprintf(messageindex, "<xml><messages refnum=\"0\"></messages></xml>");
  fclose(messageindex);

  free(tempstr);
}

int addserver(DOMElement * servers) {
  DOMElement * newserver;
  char * name,* address,* username,* password, *fromname, *returnaddress, *tempstr;
  int i,input = 'n';

  name = address = username = password = NULL;
  fromname = returnaddress = tempstr = NULL;

  while(input == 'n') {
    con_clrscr();
    con_gotoxy(0,2);
    printf("             - Email account setup assistant -");

    con_gotoxy(0,4);
    printf("            Display Name for the server: ");
    con_update();
    name = getmyline(36,41,4,0);

    con_gotoxy(0,5);
    printf(" Incoming (POP3) address of this server: ");
    con_update();
    address = getmyline(36,41,5,0);

    con_gotoxy(0,6);
    printf("               Username for this server: ");
    con_update();
    username = getmyline(36,41,6,0);

    con_gotoxy(0,7);
    printf("               Password for this server: ");
    con_update();
    password = getmyline(36,41,7,1);

    con_gotoxy(0,9);
    printf("                       Personal Info");

    con_gotoxy(0,11);
    printf("                   Your name: ");
    con_update();
    fromname = getmyline(45,30,11,0);

    con_gotoxy(0,12);
    printf("        Return email address: ");
    con_update();
    returnaddress = getmyline(45,30,12,0);

    con_clrscr();

    putchar('\n');
    putchar('\n');
    printf("             - Information Overview -\n\n");

    printf("                            Server Name: %s\n", name);
    printf("                           POP3 Address: %s\n", address);
    printf("                               Username: %s\n", username);
    printf("                               Password: ");
    for(i=0;i<strlen(password);i++)
      putchar('*');
    printf("\n\n");
    
    printf("                     Personal Info\n\n");
   
    printf("                   Your name: %s\n", fromname);
    printf("        Return email address: %s\n", returnaddress);

    con_gotoxy(0,24);
    printf(" Correct? (y/n), (A)bort");
    con_update();

    input = 0;
    while(input != 'y' && input != 'n' && input != 'A')
      input = con_getkey();

    if(input == 'A')
      return(0);
    if(input == 'n') {
      free(name);
      free(address);
      free(username);
      free(password);
      free(fromname);
      free(returnaddress);
      con_clrscr();
    }
  }

  if(makeserverinbox(strdup(address)) == -1) {
    tempstr = (char *)malloc(strlen("The inbox may  already exist.") + strlen(address) +1);
    
    sprintf(tempstr, "The inbox %s may already exist.", address);
    drawmessagebox("An error occurred while trying to create the inbox.", tempstr,1);
    return(0);
  } 

  makenewmessageindex(strdup(address));
  newserver = XMLnewNode(NodeType_Element, "server", "");

  XMLsetAttr(newserver, "name", name);
  XMLsetAttr(newserver, "address", address);
  XMLsetAttr(newserver, "username", username);
  XMLsetAttr(newserver, "password", password);

  XMLsetAttr(newserver,"fromname", fromname);
  XMLsetAttr(newserver,"returnaddress", returnaddress);

  //default advanced options
  XMLsetAttr(newserver, "unread", "0");
  XMLsetAttr(newserver, "deletemsgs", "0");
  XMLsetAttr(newserver, "skipsize", "0");

  //insert the new element as a child of "servers"
  XMLinsert(servers, NULL, newserver);
  
  return(1);
}

void drawmsglistboxheader(int type, int howmanymessages) {
  char * boxtitle;

  switch(type) {
    case INBOX:
      boxtitle = strdup("INBOX ");
    break;
    case DRAFTSBOX:
      boxtitle = strdup("DRAFTS");
    break;
    case SENTBOX:
      boxtitle = strdup("SENT  ");
    break;
  }

  con_gotoxy(1,0);
  con_setfgbg(listheadfg_col,listheadbg_col);
  con_clrline(LC_Full);

  if(howmanymessages != 1)
    printf("Mail v%s for WiNGs    %s    (%d Messages Total)         By Greg in 2003", VERSION, boxtitle, howmanymessages);
  else
    printf("Mail v%s for WiNGs    %s    (%d Message)                By Greg in 2003", VERSION, boxtitle, howmanymessages);

  con_gotoxy(0,1);
  con_clrline(LC_Full);

  printf("< S >--< FROM >--------------------< SUBJECT >-----------------------------< A >");
}

void drawmsglistboxmenu(int type) {
  con_gotoxy(1,24);
  con_setfgbg(listmenufg_col,listmenubg_col);
  con_clrline(LC_Full);

  switch(type) {
    case INBOX:
      if(abookfd != -1)
        printf(" (Q)uit, (N)ew Mail, (c)ompose,(a)ttached,(d)elete,(o)ther boxes,(t)ake address");
      else
        printf(" (Q)uit, (N)ew Mail, (c)ompose,(a)ttached,(d)elete,(o)ther boxes");
    break;
    case DRAFTSBOX:
      printf(" (Q)uit to inbox, (c)ontinue, (d)elete");
    break;
    case SENTBOX:
      printf(" (Q)uit to inbox, (r)e-send, (d)elete");
    break;
  }
}

int drawmailboxlist(int boxtype, DOMElement * message, int direction, int first, int howmanymessages) {
  //direction 0 = down, 1 = up

  char * subject, * mailaddress, * status, * attachments;
  int i;

  con_setfgbg(listfg_col,listbg_col);
  con_clrscr();

  drawmsglistboxheader(boxtype,howmanymessages);
  drawmsglistboxmenu(boxtype);

  con_setfgbg(listfg_col,listbg_col);

  if(direction == 0) {

    for(i = 2; i<23; i++) {
    
      if(message->FirstElem && (!first)) {
        return(i-1);
      } else {
        first = 0;
      }        

      con_gotoxy(2, i);
      status = XMLgetAttr(message, "status");
      printf("%s", status);   

      if(XMLfindAttr(message, "delete")) {
        con_gotoxy(2,i);
        putchar('D');
      }

      con_gotoxy(4, i);
      switch(boxtype) {
        case INBOX:
          mailaddress = strdup(XMLgetAttr(message, "from"));
        break;
        case DRAFTSBOX:
        case SENTBOX:
          mailaddress = strdup(XMLgetAttr(message, "to"));
        break;
      }

      if(strlen(mailaddress) > 20)
        mailaddress[20] = 0;
      printf("%s", mailaddress);

      con_gotoxy(25, i);
      subject = strdup(XMLgetAttr(message, "subject"));
      if(strlen(subject) > 50)
        subject[51] = 0;
      printf("%s", subject);        

      con_gotoxy(77, i);

      attachments = XMLgetAttr(message, "attachments"); 
      if(atoi(attachments) != 0)
        printf("%s", attachments);
     
      message = message->NextElem;
    } 
  } else {
    for(i = 22; i>1; i--) {
    
      con_gotoxy(2, i);
      status = XMLgetAttr(message, "status");
      printf("%s", status);   

      if(XMLfindAttr(message, "delete")) {
        con_gotoxy(2,i);
        putchar('D');
      }

      con_gotoxy(4, i);
      switch(boxtype) {
        case INBOX:
          mailaddress = strdup(XMLgetAttr(message, "from"));
        break;
        case DRAFTSBOX:
        case SENTBOX:
          mailaddress = strdup(XMLgetAttr(message, "to"));
        break;
      }

      if(strlen(mailaddress) > 20)
        mailaddress[20] = 0;
      printf("%s", mailaddress);

      con_gotoxy(25, i);
      subject = strdup(XMLgetAttr(message, "subject"));
      if(strlen(subject) > 50)
        subject[51] = 0;
      printf("%s", subject);        

      con_gotoxy(77, i);
      attachments = XMLgetAttr(message, "attachments"); 
      if(atoi(attachments) != 0)
        printf("%s", attachments);

      if(message->FirstElem)
        return(22);

      message = message->PrevElem;
    }
  }
  return(22);
}

//Use this line to rebuild the list after throwing anything that would disrupt it
//lastline = rebuildlist(type, reference, direction, first, arrowpos);

int rebuildlist(int type, DOMElement *reference, int direction, int first, int arrowpos, int howmanymessages) {
  int lastline;

  lastline = drawmailboxlist(type, reference, direction, first, howmanymessages);

  con_gotoxy(0, arrowpos);
  putchar('>');
  con_update();
  return(lastline);
}

void opendrafts(DOMElement * server,char * serverpath) {
  DOMElement * draftsboxindex, * messages, * message, * reference, * msgptr;
  DOMElement * msgelement;

  msgline * firstcc, * firstbcc, * firstattach, * msglineptr;
  int cccount, bcccount, attachcount;

  char * tempstr, * tempstr2, * indexfilestr;

  int lastline, more, howmanymessages, arrowpos, fileref, lastmsgpos;
  int direction, first, newmessages, i, input;
  int nomessages = 0;
  int elementcount;

  indexfilestr = (char *)malloc(strlen(serverpath) + strlen("drafts/index.xml") +2);

  sprintf(indexfilestr, "%sdrafts/index.xml", serverpath);

  draftsboxindex = XMLloadFile(indexfilestr);

  messages = XMLgetNode(draftsboxindex, "xml/messages");
  message  = XMLgetNode(messages, "message");
  
  setnomessages:
  
  howmanymessages = messages->NumElements;

  //If no messages in the XML index, make a mock one as below.

  if(message == NULL) {
    nomessages = 1;
    message = XMLnewNode(NodeType_Element, "message", "");
    XMLsetAttr(message, "to", "");
    XMLsetAttr(message, "status", "");
    XMLsetAttr(message, "attachments", "");
    XMLsetAttr(message, "subject", "Drafts box is currently empty.");
    message->FirstElem = 1;
    message->PrevElem = message;
    message->NextElem = message;
  }

  con_clrscr();

  reference = message;
  first = 1;
  direction = 0;
  arrowpos = 2;

  lastmsgpos = atoi(XMLgetAttr(messages, "lastmsgpos"));
  if(lastmsgpos != 0) {
    while(1) {
      if(lastmsgpos == atoi(XMLgetAttr(message, "fileref"))) {
        if(message->FirstElem)
          first = 1;
        break;
      }
      arrowpos++;
      message = message->NextElem;
      if(arrowpos > 22) {
        first = 0;
        arrowpos = 2;
        reference = message;
      }
      if(message->FirstElem) {
        reference = message;
        arrowpos = 2;
        first = 1;
        break;
      }
    }
  }

  //build the list, it should be in the position left off.

  lastline = rebuildlist(DRAFTSBOX,reference, direction, first, arrowpos, howmanymessages);

  input = 0;
  while(input != 'Q') {
    input = con_getkey();

    switch(input) {
      case CURD:
        if(arrowpos < lastline) {
          movechardown(0, arrowpos, '>');
          arrowpos++;
          message = message->NextElem;
        } else {
          message = message->NextElem;
          if(message->FirstElem) {
            message = message->PrevElem;
          } else {
            reference = message;
            direction = 0;
            first = 0;
            lastline = drawmailboxlist(DRAFTSBOX, reference, direction, first, howmanymessages);
            con_update();
            arrowpos = 2;
            con_gotoxy(0,arrowpos);
            putchar('>');
            con_update();
          }
        }
      break;
      case CURU:
        if(arrowpos > 2) {
            movecharup(0, arrowpos, '>');
            arrowpos--;
            message = message->PrevElem;
        } else if(!message->FirstElem) {
          message = message->PrevElem;
          reference = message;
          direction = 1;
          first = 0;
          lastline = drawmailboxlist(DRAFTSBOX, reference, direction, first, howmanymessages);
          con_update();
          arrowpos = 22;
          con_gotoxy(0, arrowpos);
          putchar('>');
          con_update();
        }
      break;

      case '\n':
      case 'c':
        if(nomessages)
          break;

        //move the fileref to temporary.txt
        tempstr = (char *)malloc(strlen(serverpath)+strlen("drafts/")+20);

        tempstr2 = (char *)malloc(strlen(serverpath)+strlen("drafts/")+20);

        sprintf(tempstr, "%sdrafts/%s", serverpath, XMLgetAttr(message, "fileref"));
        sprintf(tempstr2, "%sdrafts/temporary.txt", serverpath);           

        spawnlp(S_WAIT, "mv","-f",tempstr, tempstr2, NULL);
     
        //get cc, bcc, and attachment nodes...

        firstcc = firstbcc = firstattach = NULL;
        cccount = bcccount = attachcount = 0;

        elementcount = message->NumElements;

        if(elementcount) {
          msgelement = message->Elements;

          for(i = 0; i<elementcount; i++) {
            msglineptr = (msgline *)malloc(sizeof(msgline));
            msglineptr->nextline = NULL;

            if(!strcasecmp(msgelement->Node.Name, "cc")) {

              if(firstcc) {
                firstcc->nextline = msglineptr;
                msglineptr->prevline = firstcc;
                firstcc = firstcc->nextline;
              } else {
                firstcc = msglineptr;
                firstcc->prevline = NULL;
              }

              firstcc->line = strdup(XMLgetAttr(msgelement, "address"));
              cccount++;

            } else if(!strcasecmp(msgelement->Node.Name, "bcc")) {

              if(firstbcc) {
                firstbcc->nextline = msglineptr;
                msglineptr->prevline = firstbcc;
                firstbcc = firstbcc->nextline;
              } else {
                firstbcc = msglineptr;
                firstbcc->prevline = NULL;
              }

              firstbcc->line = strdup(XMLgetAttr(msgelement, "address"));
              bcccount++;

            } else if(!strcasecmp(msgelement->Node.Name, "attach")) {

              if(firstattach) {
                firstattach->nextline = msglineptr;
                msglineptr->prevline = firstattach;
                firstattach = firstattach->nextline;
              } else {
                firstattach = msglineptr;
                firstattach->prevline = NULL;
              }

              firstattach->line = strdup(XMLgetAttr(msgelement, "file"));
              attachcount++;

            }
            msgelement = msgelement->NextElem;
          }

          while(firstcc->prevline != NULL && firstcc != NULL)
            firstcc = firstcc->prevline;

          while(firstbcc->prevline != NULL && firstbcc != NULL)
            firstbcc = firstbcc->prevline;

          while(firstattach->prevline != NULL && firstattach != NULL)
            firstattach = firstattach->prevline;
        }

        if(!strcasecmp(XMLgetAttr(message, "status"), "C"))
          compose(server,draftsboxindex, serverpath, strdup(XMLgetAttr(message, "to")), strdup(XMLgetAttr(message, "subject")), firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, COMPOSECONTINUED);
        else if(!strcasecmp(XMLgetAttr(message, "status"), "R"))
          compose(server,draftsboxindex, serverpath, strdup(XMLgetAttr(message, "to")), strdup(XMLgetAttr(message, "subject")), firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, REPLYCONTINUED);

        if(message->NextElem == message) {
          XMLremNode(message);
          message = NULL;

          goto setnomessages;

        } else {
          msgptr = message;
          message = message->NextElem;

          if(reference == msgptr)
            reference = message;

          XMLremNode(msgptr);
        }

        howmanymessages = messages->NumElements;

        lastline = rebuildlist(DRAFTSBOX,reference, direction, first, arrowpos, howmanymessages);
      break;

      case 'd':
        if(nomessages) 
          break;

        if(!XMLfindAttr(message, "delete")) {
          XMLsetAttr(message, "delete", "true");
          curright(1);
          putchar('D');
          curleft(2);
        } else {
          XMLremNode(XMLfindAttr(message, "delete"));
          curright(1);
          printf("%s", XMLgetAttr(message, "status"));
          curleft(2);
        }
        con_update();
      break;

      case 'Q':

      //handle expunging

        //msgptr keeps track of the message the user was last positioned at.

        msgptr = message;

        message = XMLgetNode(messages, "message");
        tempstr = (char *)malloc(strlen(serverpath)+strlen("drafts/")+15);

        for(i=0; i < howmanymessages; i++) {
          if(XMLfindAttr(message, "delete")) {
            
            reference = message->NextElem;

            if(msgptr == message) {
              if(reference->FirstElem)
                msgptr = message->PrevElem;
              else
                msgptr = reference;
            }
                        
            sprintf(tempstr, "%sdrafts/%s", serverpath, XMLgetAttr(message, "fileref"));
            unlink(tempstr);

            XMLremNode(message);
            message = reference;  
          } else 
            message = message->NextElem;
        }
        free(tempstr);

      //store current inbox position

        XMLsetAttr(messages, "lastmsgpos", XMLgetAttr(msgptr, "fileref"));

      //save inbox xmlfile

        XMLsaveFile(draftsboxindex,indexfilestr);
        free(indexfilestr);
        
      break;
    }    
  }
}

void opensentbox(DOMElement *server, char * serverpath) {
  DOMElement * sentboxindex, * messages, * message, * reference, * msgptr;
  DOMElement * msgelement;

  msgline * firstcc, * firstbcc, * firstattach, * msglineptr;
  int cccount, bcccount, attachcount;

  char * tempstr, * tempstr2, * indexfilestr;

  int lastline, more, howmanymessages, arrowpos, fileref, lastmsgpos;
  int direction, first, newmessages, i, input;
  int nomessages = 0;
  int elementcount;

  indexfilestr = (char *)malloc(strlen(serverpath) + strlen("sent/index.xml") +2);

  sprintf(indexfilestr, "%ssent/index.xml", serverpath);

  sentboxindex = XMLloadFile(indexfilestr);

  messages = XMLgetNode(sentboxindex, "xml/messages");
  message  = XMLgetNode(messages, "message");
  
  setnomessages:
  
  howmanymessages = messages->NumElements;

  //If no messages in the XML index, make a mock one as below.

  if(message == NULL) {
    nomessages = 1;
    message = XMLnewNode(NodeType_Element, "message", "");
    XMLsetAttr(message, "to", "");
    XMLsetAttr(message, "status", "");
    XMLsetAttr(message, "attachments", "");
    XMLsetAttr(message, "subject", "Sent box is currently empty.");
    message->FirstElem = 1;
    message->PrevElem = message;
    message->NextElem = message;
  }

  con_clrscr();

  reference = message;
  first = 1;
  direction = 0;
  arrowpos = 2;

  lastmsgpos = atoi(XMLgetAttr(messages, "lastmsgpos"));
  if(lastmsgpos != 0) {
    while(1) {
      if(lastmsgpos == atoi(XMLgetAttr(message, "fileref"))) {
        if(message->FirstElem)
          first = 1;
        break;
      }
      arrowpos++;
      message = message->NextElem;
      if(arrowpos > 22) {
        first = 0;
        arrowpos = 2;
        reference = message;
      }
      if(message->FirstElem) {
        reference = message;
        arrowpos = 2;
        first = 1;
        break;
      }
    }
  }

  //build the list, it should be in the position left off.

  lastline = rebuildlist(SENTBOX,reference, direction, first, arrowpos, howmanymessages);

  input = 'a';

  while(input != 'Q') {
    input = con_getkey();

    switch(input) {
      case CURD:
        if(arrowpos < lastline) {
          movechardown(0, arrowpos, '>');
          arrowpos++;
          message = message->NextElem;
        } else {
          message = message->NextElem;
          if(message->FirstElem) {
            message = message->PrevElem;
          } else {
            reference = message;
            direction = 0;
            first = 0;
            lastline = drawmailboxlist(SENTBOX, reference, direction, first, howmanymessages);
            con_update();
            arrowpos = 2;
            con_gotoxy(0,arrowpos);
            putchar('>');
            con_update();
          }
        }
      break;
      case CURU:
        if(arrowpos > 2) {
            movecharup(0, arrowpos, '>');
            arrowpos--;
            message = message->PrevElem;
        } else if(!message->FirstElem) {
          message = message->PrevElem;
          reference = message;
          direction = 1;
          first = 0;
          lastline = drawmailboxlist(SENTBOX, reference, direction, first, howmanymessages);
          con_update();
          arrowpos = 22;
          con_gotoxy(0, arrowpos);
          putchar('>');
          con_update();
        }
      break;

      case '\n':
      case 'r':
        if(nomessages)
          break;

        //move the fileref to temporary.txt
        tempstr  = (char *)malloc(strlen(serverpath)+strlen("sent/")+15);
        tempstr2 = (char *)malloc(strlen(serverpath)+strlen("drafts/")+15);

        sprintf(tempstr, "%ssent/%s", serverpath, XMLgetAttr(message, "fileref"));
        sprintf(tempstr2, "%sdrafts/temporary.txt", serverpath);           

        spawnlp(S_WAIT, "cp","-f", tempstr, tempstr2, NULL);

        //get cc, bcc, and attachment nodes...

        firstcc = firstbcc = firstattach = NULL;
        cccount = bcccount = attachcount = 0;

        elementcount = message->NumElements;

        if(elementcount) {
          msgelement = message->Elements;

          for(i = 0; i<elementcount; i++) {
            msglineptr = (msgline *)malloc(sizeof(msgline));
            msglineptr->nextline = NULL;

            if(!strcasecmp(msgelement->Node.Name, "cc")) {

              if(firstcc) {
                firstcc->nextline = msglineptr;
                msglineptr->prevline = firstcc;
                firstcc = firstcc->nextline;
              } else {
                firstcc = msglineptr;
                firstcc->prevline = NULL;
              }

              firstcc->line = strdup(XMLgetAttr(msgelement, "address"));
              cccount++;

            } else if(!strcasecmp(msgelement->Node.Name, "bcc")) {

              if(firstbcc) {
                firstbcc->nextline = msglineptr;
                msglineptr->prevline = firstbcc;
                firstbcc = firstbcc->nextline;
              } else {
                firstbcc = msglineptr;
                firstbcc->prevline = NULL;
              }

              firstbcc->line = strdup(XMLgetAttr(msgelement, "address"));
              bcccount++;

            } else if(!strcasecmp(msgelement->Node.Name, "attach")) {

              if(firstattach) {
                firstattach->nextline = msglineptr;
                msglineptr->prevline = firstattach;
                firstattach = firstattach->nextline;
              } else {
                firstattach = msglineptr;
                firstattach->prevline = NULL;
              }

              firstattach->line = strdup(XMLgetAttr(msgelement, "file"));
              attachcount++;

            }
            msgelement = msgelement->NextElem;
          }

          while(firstcc->prevline != NULL && firstcc != NULL)
            firstcc = firstcc->prevline;

          while(firstbcc->prevline != NULL && firstbcc != NULL)
            firstbcc = firstbcc->prevline;

          while(firstattach->prevline != NULL && firstattach != NULL)
            firstattach = firstattach->prevline;
        }

        compose(server, sentboxindex, serverpath, strdup(XMLgetAttr(message, "to")), strdup(XMLgetAttr(message, "subject")), firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, RESEND);

        howmanymessages = messages->NumElements;

        lastline = rebuildlist(SENTBOX,reference, direction, first, arrowpos, howmanymessages);
      break;

      case 'd':
        if(nomessages) 
          break;

        if(!XMLfindAttr(message, "delete")) {
          XMLsetAttr(message, "delete", "true");
          curright(1);
          putchar('D');
          curleft(2);
        } else {
          XMLremNode(XMLfindAttr(message, "delete"));
          curright(1);
          printf("%s", XMLgetAttr(message, "status"));
          curleft(2);
        }
        con_update();
      break;

      case 'Q':

      //handle expunging

        //msgptr keeps track of the message the user was last positioned at.

        msgptr = message;

        message = XMLgetNode(messages, "message");
        tempstr = (char *)malloc(strlen(serverpath)+strlen("sent/")+15);

        for(i=0; i < howmanymessages; i++) {
          if(XMLfindAttr(message, "delete")) {
            
            reference = message->NextElem;

            if(msgptr == message) {
              if(reference->FirstElem)
                msgptr = message->PrevElem;
              else
                msgptr = reference;
            }
                        
            sprintf(tempstr, "%ssent/%s", serverpath, XMLgetAttr(message, "fileref"));
            unlink(tempstr);

            XMLremNode(message);
            message = reference;  
          } else 
            message = message->NextElem;
        }
        free(tempstr);

      //store current inbox position

        XMLsetAttr(messages, "lastmsgpos", XMLgetAttr(msgptr, "fileref"));

      //save inbox xmlfile

        XMLsaveFile(sentboxindex,indexfilestr);
        free(indexfilestr);
        
      break;
    }    
  }
}

char * makeserverpath(DOMElement * server) {
  char *serverpath,*tempstr;

  serverpath = strdup(XMLgetAttr(server, "address"));
  if(strlen(serverpath) > 16)
    serverpath[16] = 0;  

  tempstr = (char *)malloc(strlen("data/servers//") + strlen(serverpath)+1);

  sprintf(tempstr, "data/servers/%s/", serverpath);
  free(serverpath);

  serverpath = fpathname(tempstr, getappdir(), 1);
  free(tempstr);

  return(serverpath);  
}

int takeaddress(DOMElement * message) {
  char *msgstr, * from, *ptr, *fname, *lname;
  int input, addressbook;

  from = ptr = strdup(XMLgetAttr(message, "from"));

  if(strchr(ptr,'<') && strchr(ptr, '>')) {
    ptr = strchr(ptr, '<') +1;
    *strchr(ptr, '>') = 0;
  } else {
    //strip the "from: " off the start of the line.
    ptr += 6;
         
    if(strchr(ptr, ' '))
      *strchr(ptr, ' ') = 0;
  }

  msgstr = (char *)malloc(strlen("Add '' to your address book?")+strlen(ptr)+1);
  sprintf(msgstr,"Add '%s' to your addressbook?", ptr);

  drawmessagebox(msgstr,"  (y)es, or (n)o",0);
  input = 'z';
  while(input != 'y' && input != 'n')
    input = con_getkey();

  if(input == 'n')
    return(1);

  drawmessagebox("Last name? (required)", " ",0);
  lname = getmyline(21,29,13,0);
  if(!strlen(lname)) {
    free(lname);
    return(1);
  }
  drawmessagebox("First name?          ", " ",0);
  fname = getmyline(21,29,13,0);

  sendCon(abookfd, MAKE_ENTRY, lname, fname, NULL,    NULL, 0);
  if(!sendCon(abookfd, PUT_ATTRIB, lname, fname, "email", ptr,  0))
    drawmessagebox("Added to addressbook.", "Press any key.", 1);

  free(from);
  free(lname);
  free(fname);

  return(0);
}

void openinbox(DOMElement * server) {
  DOMElement * inboxindex, * messages, * message, * reference, * msgptr;
  accountprofile * aprofile;

  int unread, direction, first, newmessages, input;
  int lastmsgpos, lastline, more, howmanymessages, arrowpos;
  int nomessages = 0;

  char * name, * tempstr, * serverpath;

  dataset * returndataset;
  msgboxobj * newmailmsgbox;

  unread = atoi(XMLgetAttr(server, "unread"));
  name   = XMLgetAttr(server, "name");

  serverpath = makeserverpath(server);
  tempstr = (char *)malloc(strlen(serverpath)+strlen("index.xml")+1);
  sprintf(tempstr, "%sindex.xml", serverpath);
  
  inboxindex = XMLloadFile(tempstr);
  con_update();
  free(tempstr);

  //Setup the profile 
  aprofile = (accountprofile *)malloc(sizeof(accountprofile));
  aprofile->username = XMLgetAttr(server, "username");
  aprofile->password = XMLgetAttr(server, "password");
  aprofile->address  = XMLgetAttr(server, "address");

  messages = XMLgetNode(inboxindex, "xml/messages");
  message  = XMLgetNode(messages, "message");
  
  howmanymessages = messages->NumElements;

  //If no messages in the XML index, make a mock one as below.

  if(message == NULL) {
    nomessages = 1;
    message = XMLnewNode(NodeType_Element, "message", "");
    XMLsetAttr(message, "from", "");
    XMLsetAttr(message, "status", " ");
    XMLsetAttr(message, "attachments", "");
    XMLsetAttr(message, "fileref", "");
    XMLsetAttr(message, "subject", "Inbox is currently empty.");
    message->FirstElem = 1;
    message->PrevElem = message;
    message->NextElem = message;
  }

  reference = message;
  first = 1;
  direction = 0;
  arrowpos = 2;

  lastmsgpos = atoi(XMLgetAttr(messages, "lastmsgpos"));
  if(lastmsgpos != 0) {
    while(1) {
      if(lastmsgpos == atoi(XMLgetAttr(message, "fileref"))) {
        if(message->FirstElem)
          first = 1;
        break;
      }
      arrowpos++;
      message = message->NextElem;
      if(arrowpos > 22) {
        first = 0;
        arrowpos = 2;
        reference = message;
      }
      if(message->FirstElem) {
        reference = message;
        arrowpos = 2;
        first = 1;
        break;
      }
    }
  }
  //build the list, it should be in the position left off.

  lastline = rebuildlist(INBOX,reference, direction, first, arrowpos, howmanymessages);

  input = 'a';

  while(input != 'Q') {
    input = con_getkey();

    forcenextaction:

    switch(input) {
      case CURD:
        if(arrowpos < lastline) {
          movechardown(0, arrowpos, '>');
          arrowpos++;
          message = message->NextElem;
        } else {
          message = message->NextElem;
          if(message->FirstElem) {
            message = message->PrevElem;
          } else {
            reference = message;
            direction = 0;
            first = 0;
            lastline = drawmailboxlist(INBOX, reference, direction, first, howmanymessages);
            con_update();
            arrowpos = 2;
            con_gotoxy(0,arrowpos);
            putchar('>');
            con_update();
          }
        }
      break;
      case CURU:
        if(arrowpos > 2) {
            movecharup(0, arrowpos, '>');
            arrowpos--;
            message = message->PrevElem;
        } else if(!message->FirstElem) {
          message = message->PrevElem;
          reference = message;
          direction = 1;
          first = 0;
          lastline = drawmailboxlist(INBOX, reference, direction, first, howmanymessages);
          con_update();
          arrowpos = 22;
          con_gotoxy(0, arrowpos);
          putchar('>');
          con_update();
        }
      break;

      //cursor right or return will view the message

      case CURR:
      case '\r':
      case '\n':
        if(nomessages)
          break;

        if(!strcmp(XMLgetAttr(message, "fileref"), "")) {
          drawmessagebox("Error:","The message file doesn't exist",1);
          break;
        }

        if(!strcmp(XMLgetAttr(message, "status"),"N")) {
          unread--;
          XMLsetAttr(message, "status", " ");
        }         

        //view returns a 1 if the message was replied to.

        if(view(server,atoi(XMLgetAttr(message, "fileref")), serverpath, ""))
          XMLsetAttr(message, "status", "R");

        lastline = rebuildlist(INBOX,reference, direction, first, arrowpos, howmanymessages);
      break;
      case 'a':
        if(atoi(XMLgetAttr(message, "attachments"))) {
          viewattachedlist(serverpath, message);
          lastline = rebuildlist(INBOX,reference, direction, first, arrowpos, howmanymessages);
        }
      break;

      case 't':
        if(nomessages || abookfd == -1)
          break;
        takeaddress(message);
        lastline = rebuildlist(INBOX,reference, direction, first, arrowpos, howmanymessages);
      break;

      case 'N':

        returndataset = getnewmsgsinfo(aprofile,messages,server);
        
        if(returndataset != NULL) {
          playsound(NEWMAIL);

          newmailmsgbox = initmsgboxobj(returndataset->string,"","",1,returndataset->number);
          drawmsgboxobj(newmailmsgbox);

          //getnewmail() adds all the XML nodes to the index.xml file.
          //but it still has to be written out to disk.

          newmessages = getnewmail(aprofile, messages, serverpath, newmailmsgbox, strtoul(XMLgetAttr(server, "skipsize"), NULL, 10), atoi(XMLgetAttr(server, "deletemsgs")));

          playsound(DOWNLOADDONE);

          //please dear god I hope this will decrease the number of 
          //corrupt index.xml files I always end up creating.
          system("sync");

          if(nomessages) {
            nomessages = 0;
            message = XMLgetNode(messages, "message");
            reference = message;
          }

          unread += newmessages;
          howmanymessages = howmanymessages + newmessages;

          free(returndataset->string);
          free(returndataset);

          tempstr = (char *)malloc(strlen(serverpath)+strlen("index.xml")+2);

          sprintf(tempstr, "%sindex.xml", serverpath);
          XMLsaveFile(inboxindex, tempstr);

          sprintf(tempstr, "%d", unread);
          XMLsetAttr(server, "unread", tempstr);

          XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(), 1));

          free(tempstr);

        } else {
          newmessages = 0;
          playsound(NONEWMAIL);
          drawmessagebox("     No new mail.    ","    Press any key.   ",1);
        }

        lastline = rebuildlist(INBOX,reference, direction, first, arrowpos, howmanymessages);
      break;

      case 'o':
        drawmessagebox("Switch to other Mail Boxes for this account:", "(d)rafts, (s)ent mail, (c)ancel",0);
        input = con_getkey();
        switch(input) {
          case 'd':
            opendrafts(server, serverpath);
          break;
          case 's':
            opensentbox(server, serverpath);
          break;
        }

        lastline = rebuildlist(INBOX,reference, direction, first, arrowpos, howmanymessages);
      break;
 
      case 'c':
        tempstr = (char *)malloc(strlen(serverpath)+strlen("echo >drafts/temporary.txt")+1);

        sprintf(tempstr, "echo >%sdrafts/temporary.txt", serverpath);
        system(tempstr);
        free(tempstr);

        compose(server,NULL, serverpath, strdup(""), strdup(""), NULL, 0, NULL, 0, NULL, 0, COMPOSENEW);
        lastline = rebuildlist(INBOX,reference, direction, first, arrowpos, howmanymessages);
      break;

      case 'd':
        if(nomessages) 
          break;

        if(!XMLfindAttr(message, "delete")) {
          XMLsetAttr(message, "delete", "true");
          curright(1);
          putchar('D');
          curleft(2);
        } else {
          XMLremNode(XMLfindAttr(message, "delete"));
          curright(1);
          printf("%s", XMLgetAttr(message, "status"));
          curleft(2);
        }
        con_update();
        input = CURD;
        goto forcenextaction;
      break;

      case 'Q':

      //handle expunging

        //msgptr keeps track of the message the user was last positioned at.

        msgptr = message;

        message = XMLgetNode(messages, "message");
        tempstr = (char *)malloc(strlen(serverpath)+17);

        for(arrowpos = 0; arrowpos < howmanymessages; arrowpos++){
          if(XMLfindAttr(message, "delete")) {
            
            reference = message->NextElem;

            //Message ptr was on a message being deleted, move the pointer
            //to the next message, unless it's already the last one. 
            //in which case, move it back one. 

            if(msgptr == message) {
              if(reference->FirstElem)
                msgptr = message->PrevElem;
              else
                msgptr = reference;
            }
                        
            if(!strcmp(XMLgetAttr(message, "status"), "N"))
              unread--;

            sprintf(tempstr, "%s%s", serverpath, XMLgetAttr(message, "fileref"));
            unlink(tempstr);

            XMLremNode(message);
            message = reference;  
          } else 
            message = message->NextElem;
        } 
        free(tempstr);

      //store current inbox position

        XMLsetAttr(messages, "lastmsgpos", XMLgetAttr(msgptr, "fileref"));

      //save inbox xmlfile

        tempstr = (char *)malloc(strlen(serverpath)+strlen("index.xml")+2);
  
        sprintf(tempstr, "%sindex.xml", serverpath);
        XMLsaveFile(inboxindex,tempstr);
        
      //set inbox unread attribute

        sprintf(tempstr, "%d", unread);
        XMLsetAttr(server, "unread", tempstr);

        free(tempstr);

      //save mailconfig.xml

        XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(), 1));

        free(aprofile);        

      break;
    }    
  }
}

int deleteserver(DOMElement *server) {
  int input;
  char * serverpath, * address, * subpath;  

  drawmessagebox("Are you SURE you want to delete this account?","            (Y)es   or   (n)o                ",0);
  input = 'a';
  while(input != 'Y' && input !='n')
    input = con_getkey();

  if(input == 'n')
    return(0);

  address = XMLgetAttr(server, "address");
  subpath = (char *)malloc(strlen(address) + strlen("data/servers/") + 1);
  sprintf(subpath, "data/servers/%s", address);
  serverpath = fpathname(subpath,getappdir(),1);

  spawnlp(0,"rm", "-r", "-f", serverpath, NULL);

  return(1);
}

/*
int peruseserver(accountprofile *aprofile, int howmany) {
  int totalcount, i, j, pos, input;

  typedef struct subjects_s {
    int msgnum;
    char * from;
    char * subject;
  } subjects;

  subjects *listarray;

  if(!establishconnection(aprofile))
    return(0);
  
  listarray = malloc(sizeof(subjects) * howmany);

  totalcount = countservermessages(aprofile,0);
  if(totalcount < howmany)
    howmany = totalcount;

  j = 0;

  //pop3 servers start at 1, not 0;
  for(i = (totalcount-howmany)+1; i<=totalcount; i++) {
    fflush(fp);
    fprintf(fp, "top %d 0\n", i);
    fflush(fp);
    listarray[j].msgnum = i;
    while(1) {
      getline(&buf, &size, fp);
      if(buf[0] == '.')
        break;
      if(strstr(buf, "ubject: ")) {
        listarray[j].subject = strdup(buf);
        listarray[j].subject[strlen(listarray[j].subject)-1] = 0;
        listarray[j].subject += 9;
      }
      if(strstr(buf, "rom: ")) {
        listarray[j].from = strdup(buf);
        listarray[j].from[strlen(listarray[j].from)-1] = 0;
        listarray[j].from += 6;
      }
    }
    j++;
  }

  terminateconnection();

  con_clrscr();

  for(i = 0; i<howmany; i++) {
    printf("  %s %s\n", listarray[i].from, listarray[i].subject);
  }

  pos = 0;
  con_gotoxy(0,pos);
  putchar('>');
  i = 0;

  input = 0;
  while(input != 'Q') {  
    con_update();
    input = con_getkey();

    switch(input) {
      case CURD:
        if(i<howmany) {
          movechardown(0,pos,'>');
          pos++;
          i++;
        }
      break;
      case CURU:
        if(i>0) {
          movecharup(0,pos,'>');
          pos--;
          i--; 
        }
      break;
      case '\n':
      case '\r':
        
      break;
    }
  }

  return(1);
}
*/

void startmailwatch(DOMElement * a) {
  DOMElement * indexXML;
  char * minutes, * path, *pathstr, *address;

  drawmessagebox("How many minutes between new mail checks?", " ",0);
  currentaccount = (accountprofile *)malloc(sizeof(accountprofile));

  minutes = getmylinen(9,19,13);
  if(strlen(minutes) && strcmp(minutes, "0")) {
    currentaccount->mailcheck = strtoul(minutes,NULL,10);
    currentaccount->mailcheck *= 60000; //make it in microseconds
    currentaccount->address   = strdup(XMLgetAttr(a, "address"));
    currentaccount->username  = strdup(XMLgetAttr(a, "username"));
    currentaccount->password  = strdup(XMLgetAttr(a, "password"));

    path = fpathname("data/servers/", getappdir(), 1);

    address = strdup(XMLgetAttr(a, "address"));
    if(strlen(address) > 16)
      address[16] = 0;

    pathstr = (char *)malloc(strlen(path) + strlen(address) + strlen("index.xml") + 1);

    sprintf(pathstr, "%s%s/index.xml", path, address);

    indexXML = XMLloadFile(pathstr);

    currentaccount->lastmsg = atoi(XMLgetAttr(XMLgetNode(indexXML, "xml/messages"), "firstnum"));

    free(path);
    free(pathstr);
    free(address);

    //XMLremNode(indexXML);

    newThread(mailwatch,STACK_DFL,NULL);
  }
}


int drawinboxselectlist(DOMElement * server, int direction, int first, int servercount) {
  DOMElement * splashlogo;
  char * servername, * unread;
  int i;

  con_setfgbg(logofg_col,logobg_col);
  con_clrscr();
  

  // DRAW LOGO
  con_gotoxy(0,0);
  splashlogo = XMLgetNode(configxml, "xml/splashlogo");
  printf("%s", splashlogo->Node.Value);

  con_gotoxy(1,24);
  printf(" (Q)uit, (a)dd new account, (e)dit account, (D)elete account, (m)ailwatch");

  if(servercount > 5) {
    con_gotoxy(11,17);
    putchar('/');
    putchar('\\');
    con_gotoxy(11,18);
    printf("||");
    con_gotoxy(11,20);
    printf("||");
    con_gotoxy(11,21);
    putchar('\\');
    putchar('/');
  }

  if(direction == 0) {

    for(i = 17; i<22; i++) {
    
      if(server->FirstElem && (!first)) {
        return(i-1);
      } else {
        first = 0;
      }        

      con_gotoxy(15, i);
      servername = XMLgetAttr(server, "name"); 
      printf("%s", servername);
     
      con_gotoxy(49, i);
      unread = XMLgetAttr(server, "unread");
      printf("(%s unread)", unread);

      server = server->NextElem;
    } 
  } else {
    for(i = 21; i>16; i--) {

      con_gotoxy(15, i);
      servername = XMLgetAttr(server, "name"); 
      printf("%s", servername);

      con_gotoxy(45, i);
      unread = XMLgetAttr(server, "unread");
      printf("(%s unread)", unread);

      if(server->FirstElem)
        return(21);

      server = server->PrevElem;
    }
  }
  return(21);
}

void inboxselect() {
  DOMElement *temp, *server, *reference;
  int first, direction, unread, lastline, arrowpos, arrowhpos, servercount;
  char * inboxname;
  int input;
  int noservers = 0;
  soundsprofile * soundfiles;

  arrowhpos = 14;

  soundfiles = initsoundsettings();

  temp = XMLgetNode(configxml, "xml/servers");
  servercount = temp->NumElements;

  server = XMLgetNode(temp, "server");

  if(server == NULL) {
    noservers = 1;
    server = XMLnewNode(NodeType_Element, "server", "");
    XMLsetAttr(server, "name", "No accounts configured.");
    XMLsetAttr(server, "address", "");
    XMLsetAttr(server, "username", "");
    XMLsetAttr(server, "password", "");
    XMLsetAttr(server, "unread", "0");
    server->FirstElem = 1;
    server->PrevElem = server;
    server->NextElem = server;
  }

  lastline = drawinboxselectlist(server, 0, 1, servercount);

  reference = server;
  direction = 0;
  first = 1;

  arrowpos = 17;
  con_gotoxy(arrowhpos,arrowpos);
  putchar('>');

  input = 0;
  while(input != 'Q') {
    con_update();
    input = con_getkey();  

    switch(input) {
      case CURD:
        if(arrowpos < lastline) {
          movechardown(arrowhpos, arrowpos, '>');
          arrowpos++;
          server = server->NextElem;
        } else {
          server = server->NextElem;
          if(server->FirstElem) {
            server = server->PrevElem;
          } else {
            reference = server;
            direction = 0;
            first = 0;
            lastline = drawinboxselectlist(server, 0, 0, servercount);
            con_update();
            arrowpos = 17;
            con_gotoxy(arrowhpos,arrowpos);
            putchar('>');
            con_update();
          }
        }
      break;
      case CURU:
        if(arrowpos > 17) {
          movecharup(arrowhpos, arrowpos, '>');
          arrowpos--;
          server = server->PrevElem;
        } else if(!server->FirstElem) {
          server = server->PrevElem;
          reference = server;
          direction = 1;
          first = 0;
          lastline = drawinboxselectlist(server, 1, 0, servercount);
          con_update();
          arrowpos = 21;
          con_gotoxy(arrowhpos, arrowpos);
          putchar('>');
          con_update();
        }
      break;
      case 'a':
        if(addserver(temp)) {
          XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(), 1));
          if(noservers) {
            noservers = 0;
            server = XMLgetNode(temp, "server");
            reference = server;
            servercount++;
          }
          system("sync");
        }
        lastline = drawinboxselectlist(reference, direction, first, servercount);
        con_gotoxy(arrowhpos,arrowpos);
        putchar('>');
        con_update();
      break;

      case 'e':
        if(noservers)
          break;
        if(editserver(server, soundfiles)) {
          XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(), 1));
          soundsettings = setupsounds(soundsettings);
          system("sync");
        } else {
          soundfiles = setupsounds(soundsettings);
        }
        lastline = drawinboxselectlist(reference, direction, first, servercount);
        con_gotoxy(arrowhpos,arrowpos);
        putchar('>');
        con_update();
      break;

      case 'D':
        if(noservers)
          break;
        if(deleteserver(server)) {
          XMLremNode(server);
          XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(),1));
          system("sync");
        }
        lastline = drawinboxselectlist(reference, direction, first, servercount);
        con_gotoxy(arrowhpos,arrowpos);
        putchar('>');
        con_update();
      break;
      case 'm':
        if(noservers)
          break;
        else {
          startmailwatch(server);
          lastline = drawinboxselectlist(reference, direction, first, servercount);
          con_gotoxy(arrowhpos,arrowpos);
          putchar('>');
        }
      break;
      case CURR:
      case '\r':
      case '\n':
        if(noservers)
          break;
        openinbox(server);
        system("sync");
        lastline = drawinboxselectlist(reference, direction, first, servercount);
        con_gotoxy(arrowhpos,arrowpos);
        putchar('>');
        con_update();
      break;
    }
  }
}

void msgreplythread() {
  int channel, recvid;
  msgpass * msg;

  channel = makeChanP("/sys/mail");

  while(1) {
    recvid = recvMsg(channel, (void *)&msg);
    if(*(int *)( ((char *)msg) +6 ) & (O_PROC | O_STAT))
      replyMsg(recvid, makeCon(recvid, 1));
    else
      replyMsg(recvid, -1);
  }
}

soundsprofile * initsoundsettings() {
  soundsprofile * soundtemp;

  soundtemp = (soundsprofile *)malloc(sizeof(soundsprofile));

  soundtemp->position = 0;

  soundtemp->hello        = NULL;
  soundtemp->newmail      = NULL;
  soundtemp->nonewmail    = NULL;
  soundtemp->downloaddone = NULL;
  soundtemp->mailsent     = NULL;
  soundtemp->goodbye      = NULL;

  return(setupsounds(soundtemp));
}

void mailwatch() {
  uchar *MsgP;
  int RcvID, Channel;
  int msgcount, lastmsgcount;
  accountprofile * thisaccount;
      
  thisaccount = currentaccount;

  msgcount = thisaccount->lastmsg;

  Channel = makeChan();
  setTimer(-1, thisaccount->mailcheck, 0, Channel, PMSG_Alarm);
        
  while(1) {
    RcvID = recvMsg(Channel, (void *)&MsgP);
     
    lastmsgcount = countservermessages(thisaccount, 1);
    if(lastmsgcount > msgcount) {
      playsound(NEWMAIL);
      msgcount = lastmsgcount;
    }    
 
    replyMsg(RcvID,-1);
    setTimer(-1, thisaccount->mailcheck, 0, Channel, PMSG_Alarm);
  }
}

// *** MAIN ***

void main(int argc, char *argv[]){
  char * path    = NULL;
  char * tempstr = NULL;
  DOMElement * servers;
  
  //if mail is already running, it will get a response of 1. 

  if(open("/sys/mail", O_PROC) != -1) {
    printf("Mail is already running.\n");
    exit(EXIT_FAILURE);
  }

  //Register mail, and send messages to other copies of mail that
  //try to start, telling them to abort. Only 1 copy of mail can run
  //at a time. 

  newThread(msgreplythread, STACK_DFL, NULL); 

  prepconsole();

  if(con_xsize != 80) {
    printf("Mail V%s for WiNGs will only run on an 80 column console\n", VERSION);
    con_update();
    exit(EXIT_FAILURE);
  }

  path = fpathname("resources/mailconfig.xml", getappdir(), 1);
  configxml = XMLloadFile(path);

  soundsettings = initsoundsettings();
  setupcolors();

  //allocated 2 chars and \n for the quoted-printable Hex encoding.
  pqhexbuf = (char *)malloc(3);

  //Establish connection to AddressBook Service. 

  if((abookfd = open("/sys/addressbook", O_PROC)) == -1) {
    system("addressbook");
    if((abookfd = open("/sys/addressbook", O_PROC)) == -1)
      drawmessagebox("The addressbook service could not be started.","          Press a key to continue.           ",1);
  }

  playsound(HELLO);

  inboxselect(); //The program never leaves the inboxselect function, til
                 //the user is quitting the program.

  playsound(GOODBYE);

  con_end();

  printf("\x1b[0m"); //reset the terminal.
  con_clrscr();
}

soundsprofile * setupsounds(soundsprofile * soundtemp){
  DOMElement * temp;

  temp = XMLgetNode(configxml, "xml/sounds");

  if(!strcmp(XMLgetAttr(temp,"active"),"no"))
    soundtemp->active = 0;
  else
    soundtemp->active = 1;

  temp = XMLgetNode(configxml, "xml/sounds/hello");
  if(soundtemp->hello)
    free(soundtemp->hello);
  soundtemp->hello = strdup(XMLgetAttr(temp, "file"));

  temp = XMLgetNode(configxml, "xml/sounds/newmail");
  if(soundtemp->newmail)
    free(soundtemp->newmail);
  soundtemp->newmail = strdup(XMLgetAttr(temp, "file"));

  temp = XMLgetNode(configxml, "xml/sounds/nonewmail");
  if(soundtemp->nonewmail)
    free(soundtemp->nonewmail);
  soundtemp->nonewmail = strdup(XMLgetAttr(temp, "file"));

  temp = XMLgetNode(configxml, "xml/sounds/downloaddone");
  if(soundtemp->downloaddone)
    free(soundtemp->downloaddone);
  soundtemp->downloaddone = strdup(XMLgetAttr(temp, "file"));

  temp = XMLgetNode(configxml, "xml/sounds/mailsent");
  if(soundtemp->mailsent)
    free(soundtemp->mailsent);
  soundtemp->mailsent = strdup(XMLgetAttr(temp, "file"));

  temp = XMLgetNode(configxml, "xml/sounds/goodbye");
  if(soundtemp->goodbye)
    free(soundtemp->goodbye);
  soundtemp->goodbye = strdup(XMLgetAttr(temp, "file"));

  return(soundtemp);
}

int setupcolors() {
  DOMElement * colors;
  DOMElement * color;

  colors = XMLgetNode(configxml, "xml/colors");

  color = XMLgetNode(colors, "logofg");
  logofg_col = atoi(XMLgetAttr(color, "value"));
  color = XMLgetNode(colors, "logobg");
  logobg_col = atoi(XMLgetAttr(color, "value"));

  color = XMLgetNode(colors, "serverselectfg");
  serverselectfg_col = atoi(XMLgetAttr(color, "value"));
  color = XMLgetNode(colors, "serverselectbg");
  serverselectbg_col = atoi(XMLgetAttr(color, "value"));

  color = XMLgetNode(colors, "listfg");
  listfg_col = atoi(XMLgetAttr(color, "value"));
  color = XMLgetNode(colors, "listbg");
  listbg_col = atoi(XMLgetAttr(color, "value"));

  color = XMLgetNode(colors, "listheadfg");
  listheadfg_col = atoi(XMLgetAttr(color, "value"));
  color = XMLgetNode(colors, "listheadbg");
  listheadbg_col = atoi(XMLgetAttr(color, "value"));

  color = XMLgetNode(colors, "listmenufg");
  listmenufg_col = atoi(XMLgetAttr(color, "value"));
  color = XMLgetNode(colors, "listmenubg");
  listmenubg_col = atoi(XMLgetAttr(color, "value"));

  color = XMLgetNode(colors, "messagefg");
  messagefg_col = atoi(XMLgetAttr(color, "value"));
  color = XMLgetNode(colors, "messagebg");
  messagebg_col = atoi(XMLgetAttr(color, "value"));

  return(0);
}

int playsound(int soundevent) {
  char * tempstr = NULL;
  char * part1   = NULL;
  char * part2   = NULL;
  int partslen;

  if (!soundsettings->active) 
    return(0);

  part1 = strdup("wavplay ");
  part2 = strdup(" 2>/dev/null >/dev/null &");
  
  partslen = strlen(part1) + strlen(part2) + 1;

  switch(soundevent) {
   case HELLO:
     tempstr = (char *)malloc(partslen + strlen(soundsettings->hello));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->hello, part2);
   break;
   case NEWMAIL:
     tempstr = (char *)malloc(partslen + strlen(soundsettings->newmail));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->newmail, part2);
   break;
   case NONEWMAIL:
     tempstr = (char *)malloc(partslen + strlen(soundsettings->nonewmail));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->nonewmail, part2);
   break;
   case DOWNLOADDONE:
     tempstr = (char *)malloc(partslen + strlen(soundsettings->downloaddone));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->downloaddone, part2);
   break;
   case MAILSENT:
     tempstr = (char *)malloc(partslen + strlen(soundsettings->mailsent));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->mailsent, part2);
   break;
   case GOODBYE:
     tempstr = (char *)malloc(partslen + strlen(soundsettings->goodbye));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->goodbye, part2);
   break;
  }

  system(tempstr);

  free(tempstr);
  free(part1);
  free(part2);

  return(1);
}

int establishconnection(accountprofile *aprofile) {
  char * tempstr;
  int temptioflags;

  tempstr = (char *)malloc(strlen("/dev/tcp/:110")+strlen(aprofile->address)+2);
  sprintf(tempstr, "/dev/tcp/%s:110", aprofile->address);
  fp = fopen(tempstr, "r+");
  free(tempstr);

  if(!fp){
    tempstr = (char *)malloc(strlen("The server '' could not be connected to.") + strlen(aprofile->address) +2);
    sprintf(tempstr, "The server '%s' could not be connected to.", aprofile->address);
    drawmessagebox(tempstr, "",1);
    free(tempstr);
    return(0);
  }

  fflush(fp);
  getline(&buf, &size, fp);

  fflush(fp);
  fprintf(fp, "USER %s\r\n", aprofile->username);

  fflush(fp);
  getline(&buf, &size, fp);

  fflush(fp);
  fprintf(fp, "PASS %s\r\n", aprofile->password);

  fflush(fp);
  getline(&buf, &size, fp);

  if(buf[0] == '-') {
    terminateconnection();
    drawmessagebox("Error: Username and/or Password incorrect.", "",1);
    return(0);
  }
  return(1);
}

void terminateconnection() {
  fflush(fp);
  fprintf(fp, "QUIT\r\n");
  fclose(fp);
}

int countservermessages(accountprofile *aprofile, int connect) {
  int count = 0;
  int status;
  
  if(connect)
    if(!establishconnection(aprofile))
      return(count);

  fflush(fp);
  fprintf(fp, "LIST\r\n");
  
  fflush(fp);

  getline(&buf, &size, fp); //Gets the +OK message

  status = getline(&buf, &size, fp);
  while(buf[0] != '.' && status != EOF) {
    count++;
    status = getline(&buf, &size, fp);
  }

  if(connect)
    terminateconnection();

  return(count);
}

dataset * getnewmsgsinfo(accountprofile *aprofile, DOMElement *messages, DOMElement *server) {
  int count, skipped;
  ulong firstnum, totalsize, msgsize, skipsize;
  char * ptr;
  dataset * ds;

  if(!establishconnection(aprofile))
    return(NULL);

  firstnum = atol(XMLgetAttr(messages, "firstnum"));

  fflush(fp);
  fprintf(fp, "LIST\r\n");
  
  fflush(fp);

  getline(&buf, &size, fp); //Gets the +OK message
  count = 0;
  totalsize = 0;
  skipped = 0;
  skipsize = strtoul(XMLgetAttr(server, "skipsize"),NULL,10);

  do {
    count++;
    getline(&buf, &size, fp);
    if(count >= firstnum && buf[0] != '.') {
      ptr = strchr(buf, ' ') +1;
      msgsize = strtoul(ptr, NULL, 10);
      if(skipsize) {
        if(msgsize < skipsize)
          totalsize += msgsize;
        else
          skipped++;
      } else
        totalsize += msgsize;
    }  
  } while(buf[0] != '.');

  terminateconnection();

  if((count - firstnum) < 1)
    return(NULL);

  count -= firstnum;

  ptr = (char *)malloc(80);

  ds = (dataset *)malloc(sizeof(dataset));    

  //progressbar measures in increments of 1k
  ds->number = totalsize; 

  if(totalsize > 1048576) {
    totalsize /= 1048576;
    if(totalsize > 1)
      sprintf(ptr, "%d New messages. %d skipped. %ld Megabytes.",count-skipped,skipped,totalsize);
    else
      sprintf(ptr, "%d New messages. %d skipped. 1 Megabyte.",count-skipped, skipped);
  } else if(totalsize > 1024) {
    totalsize /= 1024;
    if(totalsize > 1)
      sprintf(ptr, "%d New messages. %d skipped. %ld Kilobytes.",count-skipped,skipped,totalsize);
    else
      sprintf(ptr, "%d New messages. %d skipped. 1 Kilobyte.",count-skipped,skipped);
  } else {
    sprintf(ptr, "%d New messages. %d skipped. %ld bytes.",count-skipped,skipped,totalsize);
  }

  ds->string = ptr;

  return(ds);
}

int getnewmail(accountprofile *aprofile, DOMElement *messages, char * serverpath, msgboxobj *mb, ulong skipsize, int deletefromserver){
  DOMElement * message, * attachment;
  char * tempstr;
  FILE * outfile;
  int count, i, eom, returnvalue;
  ulong firstnum, refnum,progbarcounter, msgsize;
  char * subject, * from, * boundary, * bstart, * name, *ptr;
  int attachments;

  if(!establishconnection(aprofile))
    return(0);

  refnum   = atol(XMLgetAttr(messages, "refnum"));
  firstnum = atol(XMLgetAttr(messages, "firstnum"));

  fflush(fp);
  fprintf(fp, "LIST\r\n");

  fflush(fp);
  count = 0;

  getline(&buf, &size, fp);

  do {
    count++;
    getline(&buf, &size, fp);
  } while(buf[0] != '.');
  count--;  

  progbarcounter = 0;
  returnvalue = 0;

  for(i = firstnum; i<=count; i++) {
    attachments = 0;
    eom = 0;
    bstart   = NULL;
    boundary = NULL;
    name     = NULL;

    if(skipsize) {
      //check the size of the message.
      fflush(fp);
      fprintf(fp, "LIST %d\r\n", i);
      fflush(fp);
      getline(&buf, &size, fp);

      //returned format is +OK 2 23456
      //2 seperate spaces before the value that is the size

      ptr = strchr(buf, ' ') +1;
      ptr = strchr(ptr, ' ') +1;

      msgsize = strtoul(ptr, NULL, 10);
      if(msgsize > skipsize)
        continue;
    }

    message = XMLnewNode(NodeType_Element, "message", "");

    //request the whole message. 
    fflush(fp);
    fprintf(fp, "RETR %d\r\n", i);
    
    returnvalue++;

    fflush(fp);
    getline(&buf, &size, fp);

    //Could get an error here... we don't bother to check.
    //drawmessagebox(buf,"",1);

    tempstr = (char *)malloc(strlen(serverpath)+8);
    sprintf(tempstr, "%s%ld", serverpath, refnum);
    outfile = fopen(tempstr, "w");

    progbarcounter += getline(&buf, &size, fp);
    setprogress(mb,progbarcounter);

    //Get message header

    while(!(buf[0] == '\n' || buf[0] == '\r')) {
      fprintf(outfile, "%s", buf);
      if(!strncasecmp("subject:", buf, 8)) 
        subject = strdup(buf);
      else if(!strncasecmp("from:", buf, 5))
        from = strdup(buf);
      else if(!strncasecmp("content-type: multipart/", buf, 24)) {

        //Get the Next line presumably with the boundary... 
        if(!strstr(buf, "oundary")) {
          progbarcounter += getline(&buf, &size, fp);
          setprogress(mb,progbarcounter);
          fprintf(outfile, "%s", buf);
        }

        //Check for and extract the boundary

        bstart = strdup(buf);
        if(boundary = strpbrk(bstart, "bB")) {
          if(!strncasecmp(boundary, "boundary=\"", 10)) {
            boundary += 10;
            *strchr(boundary, '"') = 0;
          } else
          boundary = NULL;
        } 
      }

      progbarcounter += getline(&buf, &size, fp);
      setprogress(mb,progbarcounter);
    }

    //Finished Getting the header

    //this saves the blank space between the header and body
    fprintf(outfile, "%s", buf);
    progbarcounter += getline(&buf, &size, fp);
    setprogress(mb,progbarcounter);

    if(boundary) {
      while(!((buf[0] == '.' && buf[1] == '\r')||(buf[0] == '.' && buf[1] == '\n'))) {
        fprintf(outfile, "%s", buf);

        if(!strncmp(buf, "--", 2)) {
          if(!strncmp(&buf[2], boundary, strlen(boundary))) {

            while(!((buf[0]=='\n')||(buf[0]=='\r'))) {
              progbarcounter += getline(&buf, &size, fp);
              setprogress(mb,progbarcounter);

              //The last boundary is right before the end of the message

              if((buf[0]=='.'&&buf[1]=='\n')||(buf[0]=='.'&&buf[1]=='\r')) {
                eom = 1;
                break;
              }
 
              //Get attached file's name, and create XML child. 

              if(strstr(buf, "name")) {
                attachments++;

                name = strdup(strstr(buf, "name"));
                name += 4;
                while(name[0] == ' ')
                  name++;
                while(name[0] == '=')
                  name++;
                while(name[0] == ' ')
                  name++;
                while(name[0] == '"')
                  name++;

                *strchr(name, '"') = 0;
                attachment = XMLnewNode(NodeType_Element, "attachment", "");
                XMLsetAttr(attachment, "filename", name);
                XMLinsert(message, NULL, attachment);
              }
              fprintf(outfile, "%s", buf);            
            }
          }
        }
  
        if(!eom) {
          progbarcounter += getline(&buf, &size, fp);
          setprogress(mb,progbarcounter);
        }
      } 
    } else {
      while(!((buf[0] == '.' && buf[1] == '\r')||(buf[0] == '.' && buf[1] == '\n'))) {
        fprintf(outfile, "%s", buf);
        progbarcounter += getline(&buf, &size, fp);
        setprogress(mb,progbarcounter);
      } 
    }

    fclose(outfile);

    if(deletefromserver) {
      fflush(fp);
      fprintf(fp, "DELE %d\r\n", i);
      fflush(fp);
      getline(&buf, &size, fp);
    }

    fflush(fp);
    free(tempstr); 
    if(bstart)
      free(bstart);

    if(from) {
      if(from[strlen(from)-2] == '\r')
        from[strlen(from)-2] = 0;
      else if(from[strlen(from)-1] == '\n')
        from[strlen(from)-1] = 0;
    }
    if(subject) {
      if(subject[strlen(subject)-2] == '\r')
        subject[strlen(subject)-2] = 0;
      else if(subject[strlen(subject)-1] == '\n')
        subject[strlen(subject)-1] = 0;
    }

    XMLsetAttr(message, "status", "N");
    XMLsetAttr(message, "from", &from[6]);
    XMLsetAttr(message, "subject", &subject[9]);
    tempstr = (char *)malloc(15);
    sprintf(tempstr, "%d", attachments);
    XMLsetAttr(message, "attachments", tempstr);
    tempstr = (char *)malloc(15);
    sprintf(tempstr, "%d", refnum);
    XMLsetAttr(message, "fileref", tempstr);
    XMLinsert(messages, NULL, message);
    
    refnum++;
  } // Loop Back up to get next new message.

  terminateconnection();

  if(deletefromserver) {
    fflush(fp);
    fprintf(fp, "LIST\r\n");

    fflush(fp);
    count = 0;

    getline(&buf, &size, fp);

    do {
      count++;
      getline(&buf, &size, fp);
    } while(buf[0] != '.');

    //note count will end up one too high, which is fine, 
    //we just don't add one when we write it out to the xml file.

    tempstr = (char *)malloc(15);
    sprintf(tempstr, "%d", count);
    XMLsetAttr(messages, "firstnum", tempstr);
  } else {
    tempstr = (char *)malloc(15);
    sprintf(tempstr, "%d", count+1);
    XMLsetAttr(messages, "firstnum", tempstr);
  }

  tempstr = (char *)malloc(15);
  sprintf(tempstr, "%d", refnum);
  XMLsetAttr(messages, "refnum", tempstr);

  return(returnvalue);
}

int view(DOMElement * server, int fileref, char * serverpath, char * subpath){
  FILE * incoming, * replyfile;
  char * subject, * from, * date, * bstart, * name, * replyto, *replysubject;
  char * bodytext, * headertext, * tempstr, *tempstr2, * line, * lineptr;
  msgline * thisline, * prevline, * topofview, * firstline;
  int charcount, i, html, c, input;
  int replied = 0;  
  
  tempstr = (char *)malloc(strlen(serverpath)+strlen(subpath)+17);
  
  sprintf(tempstr, "%s%s%d", serverpath, subpath, fileref);
  msgfile = fopen(tempstr, "r");

  if(!msgfile) {
    drawmessagebox("An internal error has occurred. File Not Found.", "",1);
    return(0);
  } 

  boundary = NULL;
  subject  = NULL;
  date     = NULL;
  from     = NULL;
  eom      = 0;
  html     = 0;
  pq       = 0;

  getline(&buf, &size, msgfile);
  while(!(buf[0] == '\n' || buf[0] == '\r')) {

    if(buf[strlen(buf)-2] == '\r') {
      buf[strlen(buf)-2] = '\n';
      buf[strlen(buf)-1] = 0;
    }

    if(!strncasecmp("subject:", buf, 8)) 
      subject = strdup(buf);
    else if(!strncasecmp("from:", buf, 5))
      from = strdup(buf);
    else if(!strncasecmp("date:", buf, 5))
      date = strdup(buf);
    else if(!strncasecmp("content-type: multipart/", buf, 24)) {

    if(subject[strlen(subject) - 2] == '\r')
      subject[strlen(subject) - 2] = 0;
    else if (subject[strlen(subject) - 1] == '\n')
      subject[strlen(subject) -1] = 0;

    if(from[strlen(from) - 2] == '\r')
      from[strlen(from) - 2] = 0;
    else if (from[strlen(from) - 1] == '\n')
      from[strlen(from) -1] = 0;

    if(date[strlen(date) - 2] == '\r')
      date[strlen(date) - 2] = 0;
    else if (date[strlen(date) - 1] == '\n')
      date[strlen(date) -1] = 0;

    if(strlen(subject) > 80) 
      subject[80] = 0;
    if(strlen(from) > 80)
      from[80] = 0;
    if(strlen(date) > 80)
      from[80] = 0;

      //Get the Next line presumably with the boundary... 

      if(!strstr(buf, "oundary"))
        getline(&buf, &size, msgfile);

      //Check for and extract the boundary

      bstart = strdup(buf);
      if(boundary = strpbrk(bstart, "bB")) {
        if(!strncasecmp(boundary, "boundary=\"", 10)) {
          boundary += 10;
          *strchr(boundary, '"') = 0;
        } else
          boundary = NULL;
      } 
    } else if(!strncasecmp("content-type: text/html", buf, 23)) {
      html = 1;
    } else if(!strncasecmp("content-transfer-encoding: quoted-printable", buf, 43)) {
      pq = 1;
    }
    getline(&buf, &size, msgfile);      
  }

  //We now have subject, from, date and Possibly a boundary. 

  //IF no boundary, then no attachments. Put all of following text into
  //the linked lines, parse ALL the text through web if it's html; 
  if(!boundary) {

    if(html){
      pipe(writetowebpipe);
      pipe(readfromwebpipe);

      redir(writetowebpipe[0], STDIN_FILENO);
      redir(readfromwebpipe[1], STDOUT_FILENO);
      spawnlp(0, "web", NULL);
      close(readfromwebpipe[1]);
      close(writetowebpipe[0]);

      incoming = fdopen(readfromwebpipe[0], "r");  

      charcount = 0;
      eom       = 0;
      prevline  = NULL;
      thisline  = NULL;
      line      = (char *)malloc(81);
      lineptr   = line;

      newThread(givealldatatoweb, STACK_DFL, NULL);

      while(!eom) {
        charcount = 0;

        //Create a new line struct. setting the Prev and Next line pointers.
        thisline = (msgline *)malloc(sizeof(msgline));
        thisline->prevline = prevline;
        if(prevline)
          prevline->nextline = thisline;

        memset(line, 0, 81);
        lineptr = line;

        while(charcount < 80) {
          c = fgetc(incoming);
          //printf("%d\n", c); con_update();
         
          switch(c) {
            case '\n':
              //end msgline struct here. 
              charcount = 80;
            break;
            case '\r':
              //Do Nothing... simply leave it out.
            break;
            case EOF:
              eom = 1;
              charcount = 80;
            break;
            default:
              //increment character count for current line. 
              //store character at lineptr, increment lineptr
              charcount++;
              *lineptr = c;
              lineptr++;
          }
        }
        thisline->line = strdup(line);
        prevline = thisline;
      }
      fclose(incoming);
       

    } else {
      charcount = 0;
      prevline  = NULL;
      thisline  = NULL;
      line      = (char *)malloc(81);
      lineptr   = line;

      while(!eom) {
        charcount = 0;

        //Create a new line struct. setting the Prev and Next line pointers.
        thisline = (msgline *)malloc(sizeof(msgline));
        thisline->prevline = prevline;
        if(prevline)
          prevline->nextline = thisline;

        memset(line, 0, 81);
        lineptr = line;

        while(charcount < 79) {
          c = fgetc(msgfile);

          switch(c) {
            case '\n':
              //end msgline struct here. 
              charcount = 79;
            break;
            case '\r':
              //Do Nothing... simply leave it out.
            break;
            case EOF:
              eom = 1;
              charcount = 79;
            break;
            default:
              //increment character count for current line. 
              //store character at lineptr, increment lineptr
              charcount++;
              *lineptr = c;
              lineptr++;
          }
        }
        thisline->line = strdup(line);
        prevline = thisline;
      }
    }
    fclose(msgfile);


  //If there IS a boundary... read the file character at a time, filling 
  //lines, but after each line is filled, check to see if it contains a 
  //boundary. if it does, remove that line from the linked list of lines. 
  //and deal with the next several lines as mimeheaders. if you encounter
  //a content-type header that is not text, close the file and move on.
  //if you find text that is HTML pipe it through web in a seperate thread.

  } else {

    charcount = 0;
    html      = 0;
    pq        = 0;
    prevline  = NULL;
    thisline  = NULL;
    line      = (char *)malloc(81);
    lineptr = line;

    while(!eom) {
      charcount = 0;

      //Create a new line struct. setting the Prev and Next line pointers.
      thisline = (msgline *)malloc(sizeof(msgline));
      thisline->prevline = prevline;
      if(prevline)
        prevline->nextline = thisline;

      memset(line, 0, 81);
      lineptr = line;

      while(charcount < 79) {
        c = fgetc(msgfile);

        switch(c) {
          case '\n':
            //end msgline struct here. 
            charcount = 79;
          break;
          case '\r':
            //Do Nothing... simply leave it out.
          break;
          case EOF:
            eom = 1;
            charcount = 79;
          break;
          case '=':
            if(pq) {
              c = fgetc(msgfile);
              if(c == EOF) {
                eom = 1;
                charcount = 79;
                break;
              }
              if(c == '\n' || c == '\r') 
                break; 
                //do nothing. don't store the = or the \n

              //Check for valid character. if not, could be a boundary
              if((c < '0' || c > '9') && (c < 'A' || c > 'F')) { 
                                  
                charcount++;
                *lineptr = '=';
                lineptr++;

              } else {
                pqhexbuf[0] = c;
             
                c = fgetc(msgfile);
                if(c == EOF) {
                  eom = 1;
                  charcount = 79;
                  break;
                }
                pqhexbuf[1] = c;
                pqhexbuf[2] = 0;

                c = (int)strtoul(pqhexbuf, NULL, 16);
              }
            }
          default:
            //increment character count for current line. 
            //store character at lineptr, increment lineptr
            charcount++;
            *lineptr = c;
            lineptr++;
        }
      }

      if(strstr(line, boundary) || html || ((boundary2 != NULL)&&(strstr(line, boundary2)))) {
        html = 0;
        pq   = 0;
        if(strstr(line, boundary) || ((boundary2 != NULL)&&(strstr(line, boundary2))))
          getline(&buf, &size, msgfile);
        while(!(buf[0] == '\n' || buf[0] == '\r')) {
          if(!strncasecmp(buf, "content-type:", 13)) {
            if(strncasecmp(buf, "content-type: text", 18)) {
              eom = 1;
            } else {
              if(!strncasecmp(buf, "content-type: text/plain", 24))
                sprintf(line, "         ---***** MIME Section Change:   Plain Text     *****---");
              else if(!strncasecmp(buf, "content-type: text/html", 23)) {
                sprintf(line, "         ---***** MIME Section Change:   Parsed HTML     *****---");
                html = 1;
              } else
                sprintf(line, "         ---***** MIME Section Change:   Plain Text     *****---");
            }
          } else if(!strncasecmp(buf, "content-transfer-encoding:", 26)) {
            if(!strncasecmp(buf, "content-transfer-encoding: quoted-printable", 43)) {
              pq = 1;
            }
          } else if(strstr(buf, "oundary")) {

            bstart    = strdup(buf);
            boundary2 = strstr(bstart, "oundary");

            if(!strncasecmp(boundary2, "oundary=\"", 9)) {
              boundary2 += 10;
              *strchr(boundary2, '"') = 0;
            } else
              boundary2 = NULL;
           
          }
          if(EOF == getline(&buf, &size, msgfile))
            break;
        }
      }

      thisline->line = strdup(line);
      prevline = thisline;

      if(html) {
        prevline = parsehtmlcontent(prevline);
      }
    }
    fclose(msgfile);
  }

  //At this point, we have prevline set to the last line struct. 
  //Loop moving back through the linked list until you hit the line struct
  //with a NULL prevline. This is the first. And start displaying!

  displayview:  

  thisline = prevline;
  thisline->nextline = NULL;
  while(thisline->prevline) {
    thisline = thisline->prevline;
  }

  topofview = thisline;

  con_clrscr();

  for(i = 4; i<24; i++) {
    con_gotoxy(0, i);
    printf("%s", thisline->line);
    if(thisline->nextline)
      thisline = thisline->nextline;
    else
      break;
  }  
  if(thisline->nextline)
    thisline = thisline->prevline;

  con_gotoxy(0,0);
  con_setfgbg(COL_Blue, COL_Blue);
  con_clrline(LC_End);
  con_gotoxy(0,1);
  con_setfgbg(COL_Purple, COL_Blue);
  con_clrline(LC_End);
  con_gotoxy(0,2);
  con_setfgbg(COL_Blue, COL_Blue);
  con_clrline(LC_End);

  con_gotoxy(0,0);
  printf("%s", date);

  con_gotoxy(0,1);
  printf("%s", from);

  con_gotoxy(0,2);
  printf("%s", subject);

  con_gotoxy(0,3);
  con_setfgbg(COL_Black, COL_Blue);
  con_clrline(LC_End);
  printf("________________________________________________________________________________");
 
  con_setfgbg(COL_Cyan, COL_Black);

  con_gotoxy(2,24);
  if(strlen(subpath))
    printf(" (Q)uit to list");
  else
    printf(" (Q)uit to list, (r)eply");

  con_update();
  input = 'A';

  con_setscroll(4,24);

  while(input != 'Q' && input != CURL) {

    input = con_getkey();

    switch(input) {
      case CURD:
        if(thisline->nextline) {
          topofview = topofview->nextline;
          thisline = thisline->nextline;
          con_gotoxy(0, 23);
          putchar('\n');
          con_gotoxy(0, 23);
          printf("%s", thisline->line);
          con_update();
        }
      break;
      case CURU:
        if(topofview->prevline) {
          topofview = topofview->prevline;
          thisline = thisline->prevline;
          con_gotoxy(0,4);
          printf("\x1b[1L");
          printf("%s", topofview->line);
          con_update();
        }
      break;
      case 'r':
        if(!strlen(subpath)) {
          //Write message lines to drafts/temporary.txt with quote markers.

          tempstr = (char *)malloc(strlen(serverpath)+strlen("drafts/temporary.txt")+1);
  
          sprintf(tempstr, "%sdrafts/temporary.txt", serverpath);
          
          firstline = thisline;

          while(firstline->prevline) 
            firstline = firstline->prevline;

          replyfile = fopen(tempstr, "w");
          if(!replyfile) {
            drawmessagebox("ERROR: Could not create temporary reply file.","Press any key.",1);
            break;
          }

          fprintf(replyfile, ">%s\n", firstline->line);

          while(firstline->nextline) {
            firstline = firstline->nextline;
            fprintf(replyfile, ">%s\n", firstline->line);
          }

          fclose(replyfile);
 
          //call compose() with REPLY as the compose type. 

          replyto = strdup(from);

          if(strchr(replyto,'<') && strchr(replyto, '>')) {
            replyto = strchr(replyto, '<');
            replyto++;
            *strchr(replyto, '>') = 0;
          } else {
            //strip the "from: " off the start of the line.
            replyto += 6;
            
            if(strchr(replyto, ' '))
              *strchr(replyto, ' ') = 0;

            else if(strchr(replyto, '\r'))
              *strchr(replyto, '\r') = 0;

            else
              *strchr(replyto, '\n') = 0;
          }

          //subject has "Subject: ", at the start of it. we strip it.

          replysubject = (char *)malloc(strlen(subject)-7+strlen("Re: "));
          sprintf(replysubject,"Re: %s",subject+9);

          if(strchr(replysubject,'\r'))
            *strchr(replysubject,'\r') = 0;
          else
            *strchr(replysubject,'\n') = 0;

          compose(server,NULL, serverpath, replyto, replysubject, NULL, 0, NULL, 0, NULL, 0, REPLY);
          replied = 1;
        }
        goto displayview;
      break;
    }
  }
  return(replied);  
}

void givealldatatoweb() {
  FILE * output;  
  int eomlocal, i;
  char * tempstr, * ptr;

  output = fdopen(writetowebpipe[1], "w");

  if(pq) {
    //Might want to convert this to Char at a time.

    while(eomlocal = getline(&buf,&size, msgfile)) {
      if(eomlocal == EOF) {
        break;
      }
    
      ptr = buf;
      tempstr = (char *)malloc(strlen(buf)+1);
      memset(tempstr, 0, strlen(buf)+1);
      for(i = 0; i<strlen(buf); i++) {
        if(*ptr == '=') {
          ptr++; i++;
          if(*ptr == '\n' || *ptr == '\r')
            break;
          pqhexbuf[0] = *ptr;
          ptr++; i++;
          pqhexbuf[1] = *ptr;
          ptr++; 
          pqhexbuf[2] = 0;
          sprintf(tempstr, "%s%c", tempstr, (int)strtoul(pqhexbuf, NULL, 16));
        } else { 
         sprintf(tempstr, "%s%c", tempstr, *ptr);
         ptr++; 
        }
      }    
     
      fprintf(output, "%s", tempstr);
      free(tempstr);
    }
  } else {
    while(eomlocal = getline(&buf,&size, msgfile)) {
      if(eomlocal == EOF) {
        break;
      }
      fprintf(output, "%s", buf);
    }
  }
  fclose(output);
}

void givesomedatatoweb() {
  FILE * output;  
  int eomlocal, i;
  char * tempstr, * ptr;

  output = fdopen(writetowebpipe[1], "w");

  if(pq) {
    while(eomlocal = getline(&buf,&size, msgfile)) {
      if(eomlocal == EOF) {
        eom = 1;
        break;
      }
      if(strstr(buf, boundary) || ((boundary2 != NULL)&&(strstr(buf, boundary2))))
        break;
    
      ptr = buf;
      tempstr = (char *)malloc(strlen(buf)+1);
      memset(tempstr, 0, strlen(buf)+1);
      for(i = 0; i<strlen(buf); i++) {
        if(*ptr == '=') {
          ptr++; i++;
          if(*ptr == '\n' || *ptr == '\r')
            break;
          pqhexbuf[0] = *ptr;
          ptr++; i++;
          pqhexbuf[1] = *ptr;
          ptr++; 
          pqhexbuf[2] = 0;
          sprintf(tempstr, "%s%c", tempstr, (int)strtoul(pqhexbuf, NULL, 16));
        } else { 
         sprintf(tempstr, "%s%c", tempstr, *ptr);
         ptr++; 
        }
      }    

      fprintf(output, "%s", tempstr);
      free(tempstr);
    }
  } else {
    while(eomlocal = getline(&buf,&size, msgfile)) {
      if(eomlocal == EOF) {
        eom = 1;
        break;
      }
      if(strstr(buf, boundary) || ((boundary2 != NULL)&&(strstr(buf, boundary2))))
        break;

      fprintf(output, "%s", buf);
    }
  }
  fclose(output);
}

msgline * parsehtmlcontent(msgline * prevline) {
  FILE * incoming;
  msgline * thisline;
  char * line, * lineptr;
  int c;
  int charcount, eom;

  pipe(writetowebpipe);
  pipe(readfromwebpipe);

  redir(writetowebpipe[0], STDIN_FILENO);
  redir(readfromwebpipe[1], STDOUT_FILENO);
  spawnlp(0, "web", NULL);
  close(readfromwebpipe[1]);
  close(writetowebpipe[0]);

  incoming = fdopen(readfromwebpipe[0], "r");  

  charcount = 0;
  eom = 0;
  thisline  = NULL;
  line      = (char *)malloc(81);
  lineptr   = line;

  newThread(givesomedatatoweb, STACK_DFL, NULL);

  while(!eom) {
    charcount = 0;

    //Create a new line struct. setting the Prev and Next line pointers.
    thisline = (msgline *)malloc(sizeof(msgline));
    thisline->prevline = prevline;
    prevline->nextline = thisline;

    memset(line, 0, 81);
    lineptr = line;

    while(charcount < 80) {
      c = fgetc(incoming);

      switch(c) {
        case '\n':
          //end msgline struct here. 
          charcount = 80;
        break;
        case '\r':
          //Do Nothing... simply leave it out.
        break;
        case EOF:
          eom = 1;
          charcount = 80;
        break;
        default:
          //increment character count for current line. 
          //store character at lineptr, increment lineptr
          charcount++;
          *lineptr = c;
          lineptr++;
      }
    }
    thisline->line = strdup(line);
    prevline = thisline;
  }
  fclose(incoming);

  return(prevline);
}

void base64decode() {
  FILE * output;

  output = fdopen(writetobase64pipe[1], "w");
  getline(&buf, &size, msgfile);
  while(strlen(buf) > 2) {
    fprintf(output, "%s", buf);
    getline(&buf, &size, msgfile);
  }
  fclose(output);
}

void viewattachedlist(char * serverpath, DOMElement * message) {
  DOMElement * attachment, * firstattach, *currentattach;
  int i, maxnum, numofattachs, input, curpos, decodedbyte;
  char * tempstr, * filename, * originalfilename, * boundary, * fileref;
  FILE * savefile, *incoming;

  maxnum = 19;

  con_clrscr();

  con_gotoxy(0,1);
  printf(" Attachments: ");

  con_gotoxy(0,24);
  printf("(Q)uit to inbox, (s)ave selected file");

  con_setscroll(2,22);

  attachment = XMLgetNode(message, "attachment");
  firstattach = currentattach = attachment;

  fileref = XMLgetAttr(message, "fileref");

  curpos = 2;

  numofattachs = message->NumElements;
  if(numofattachs < maxnum)
    maxnum = numofattachs;   

  for(i = 2; i < maxnum + 2; i++) {  
    con_gotoxy(0,i);
    printf("  %s", XMLgetAttr(attachment, "filename"));
    attachment = attachment->NextElem;
  }

  con_gotoxy(0,2);
  putchar('>');

  con_update();

  input = 'e';

  while(input != 'Q') {
    input = con_getkey();

    switch(input) {
      case CURD:
        if(!currentattach->NextElem->FirstElem) {
          currentattach = currentattach->NextElem;
          if(curpos == 22) {
            putchar('\n');
            con_gotoxy(0,22);
            printf("%s", XMLgetAttr(currentattach, "filename"));
          } else {
            con_gotoxy(0,curpos);
            putchar(' ');
            curpos++;
            con_gotoxy(0,curpos);
            putchar('>');
          }
        }
      break;
      case CURU:
        if(!currentattach->FirstElem) {
          currentattach = currentattach->PrevElem;
          if(curpos == 2) {
            printf("\x1b[1L");
            con_gotoxy(0,2);
            printf("%s", XMLgetAttr(currentattach, "filename"));
          } else {
            con_gotoxy(0,curpos);
            putchar(' ');
            curpos--;
            con_gotoxy(0,curpos);
            putchar('>');
          }
        }
      break;
      case 's':
        filename = strdup(XMLgetAttr(currentattach, "filename"));
        originalfilename = strdup(filename);

        if(strlen(filename) > 16) {
          drawmessagebox("Filename is longer than 16 characters", "(t)runcate, (r)ename", 0);

          while(input != 't' && input != 'r') 
            input = con_getkey();

          switch(input) {
            case 't':
              //Leave extension intact if there is one.
              if(filename[strlen(filename)-4] == '.') {
                filename[12] = filename[strlen(filename)-4];   
                filename[13] = filename[strlen(filename)-3];   
                filename[14] = filename[strlen(filename)-2];   
                filename[15] = filename[strlen(filename)-1];   
              }  
              filename[16] = 0;
            break;
            case 'r':
              tempstr = (char *)malloc(strlen("Old filename: ")+strlen(filename)+2);
              sprintf(tempstr, "Old filename: %s", filename);
              drawmessagebox(tempstr, "New filename:", 0);
              free(tempstr);
              if(filename)
                free(filename);
              filename = getmyline(16,20,15,0);
            break;
          }
        }
        tempstr = (char *)malloc(strlen(serverpath)+strlen(fileref)+2);        
        sprintf(tempstr, "%s%s", serverpath, fileref);
        msgfile = fopen(tempstr, "r");
        free(tempstr);

	//Initialize Boundary to NULL, 
        boundary = NULL;

        //Then scan through the header and set boundary if one is found.
        getline(&buf, &size, msgfile);
        while(strlen(buf) > 2) {
          if(strstr(buf, "oundary")) {
            boundary = strdup(buf);
            boundary = strchr(boundary, '"');
            boundary++;
            *strchr(boundary, '"') = 0;
            break;
          }
          getline(&buf, &size, msgfile);
        }

        if(boundary) {

          //printf("boundary found... '%s'\n", boundary);
          //con_update();

          while(getline(&buf, &size, msgfile) != EOF) {

            //printf("got a line!\n%s", buf);
            //con_update();

            if(strstr(buf, boundary)) {
              getline(&buf, &size, msgfile);
              while(strlen(buf) > 2) {

                //printf("inside the header!\n%s", buf);
                //con_update();

                if(strstr(buf, originalfilename)) {

                  //printf("found original filename!!\n");
                  //con_update();

                  //Skip past the rest of the mime header... 
                  //I'm only assuming it's standard base64 encoded.
                  //if it's not, then it's not supported. yet. 
                  while(strlen(buf) > 2)
                    getline(&buf, &size, msgfile);

                  //Send all the next lines to base64 d !!
                  
                  pipe(writetobase64pipe);
                  pipe(readfrombase64pipe);

                  redir(writetobase64pipe[0], STDIN_FILENO);
                  redir(readfrombase64pipe[1], STDOUT_FILENO);
                  spawnlp(0, "base64", "d", NULL);
                  close(readfrombase64pipe[1]);
                  close(writetobase64pipe[0]);

                  incoming = fdopen(readfrombase64pipe[0], "r");  
                  newThread(base64decode, STACK_DFL, NULL);

                  savefile = fopen(filename, "w");  

                  while((decodedbyte = fgetc(incoming)) != EOF)
                    fputc(decodedbyte, savefile);

                  fclose(savefile);
                  goto leaveloops;
                } //end if found original filename in sub header.
                getline(&buf, &size, msgfile);

              } //end looping through the sub header.
            } //end found the start of a boundary.
          } //end looping through the whole message.

        } //end a boundary exists.

        leaveloops:
        fclose(msgfile);
      break; 
    }
    //If a message box drew onto the screen... I might have to do
    //some sort of a redraw here. 
    con_update();
  }
  con_setscroll(0,24);
}

