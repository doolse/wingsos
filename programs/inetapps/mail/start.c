//Mail V2.x for Wings
#include "mailheader.h"

char * VERSION = "2.3";

extern DOMElement * configxml; //the root element of the config xml element.
extern soundsprofile * soundsettings;

extern int logofg_col,     logobg_col,     serverselectfg_col, serverselectbg_col;
extern int listfg_col,     listbg_col,     listheadfg_col,     listheadbg_col;
extern int listmenufg_col, listmenubg_col, messagefg_col,      messagebg_col;

int  size = 0;
char *buf = NULL;

char *server;           // Server name as text
FILE *fp;               // Main Server connection.
FILE *msgfile;          // global to be piped through web or base64 in a thread
int abookfd;            // addressbook filedescriptor
namelist *abook = NULL; // ptr to array of AddressBook info
char *abookbuf  = NULL; // ptr to Raw AddressBook data buffer

char *scrollup = "\x1b[1L"; //scroll up esc sequence. should be in console.h

//single linked list of all mailwatches active
activemailwatch * headmailwatch = NULL; 
struct wmutex exclservercon = {-1, -1}; //exclusive server connection

//Multithreaded HTML parsing globals 
int writetowebpipe[2]; 
int readfromwebpipe[2]; 
msgline * htmlfirstline;
int writetobase64pipe[2];
int readfrombase64pipe[2];

char * strcasestr(char * big, char * little) {
  char * ptr;
  int len;
  char firstchar;

  firstchar = little[0];
  len = strlen(little);

  ptr = big;
  while(ptr != NULL) {
    ptr = strchr(ptr, firstchar);
    if(ptr) {
      if(!strncasecmp(ptr, little, len))
        return(ptr);
      ptr++;
    }
  }

  //Switch case of search character
  if(firstchar > 64 && firstchar < 91) 
    firstchar = firstchar + 32;
  else if(firstchar > 96 && firstchar < 123)
    firstchar = firstchar - 32;

  ptr = big;
  while(ptr != NULL) {
    ptr = strchr(ptr, firstchar);
    if(ptr) {
      if(!strncasecmp(ptr, little, len))
        return(ptr);
      ptr++;
    }
  }

  return(NULL);
}

char * itoa(int number) {
  char * string;
  int numlen;

  if(number < 10)
    numlen = 2;
  else if(number < 100)
    numlen = 3;
  else if(number < 1000)
    numlen = 4;
  else if(number < 10000)
    numlen = 5;
  else
    numlen = 6;

  string = malloc(numlen);
  sprintf(string, "%d", number);

  return(string);
}

void curleft(int num) {
  printf("\x1b[%dD", num);
  con_update();
}

void curright(int num) {
  printf("\x1b[%dC", num);
  con_update();
}

void con_reset() {
  //reset the terminal.
  printf("\x1b[0m"); 
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

void setcolors(int fg_col, int bg_col) {
  con_setfg(fg_col);
  con_setbg(bg_col);
  con_update();
}

void drawaddressbookselector(int width, int total, int start) {
  int row, col, i, j;
  namelist * aptr = abook;

  col = 30;
  row = 1;

  if(total > 20)
    total = 20;

  for(i=0;i<start;i++)
    aptr++;

  con_gotoxy(col,row);
  for(i=0;i<width+4;i++)
    putchar('_');

  for(i=0;i<total;i++) {
    row++;
    con_gotoxy(col,row);

    putchar('|');
    printf(" %s %s", aptr->firstname,aptr->lastname);

    for(j=0;j<width-strlen(aptr->firstname)-strlen(aptr->lastname);j++) 
      putchar(' ');

    putchar('|');
    aptr++;
  }
  row++;
  con_gotoxy(col,row);
  putchar('|');
  for(i=0;i<width+2;i++)
    putchar('_');
  putchar('|');
}

char * selectfromaddressbook(char * oldaddress) {
  int buflen = 100;
  namelist * abookptr;
  char * ptr, * returnbuf;
  int total, i, returncode, maxwidth, current, start, input, tempwidth;
  int arrowxpos, arrowypos;

  if(abookbuf)
    free(abookbuf);

  if(abook)
    free(abook);

  abookbuf = (char *)malloc(buflen);
  
  returncode = sendCon(abookfd, GET_ALL_LIST, NULL, NULL, NULL, abookbuf, buflen);
    
  //The addressbook returns an ERROR code if the buffer was too small,
  //and puts the minimum buffer size needed as ascii in the buffer.
  
  if(returncode == ERROR) {
    buflen = atoi(abookbuf);
    free(abookbuf);

    abookbuf = (char *)malloc(buflen);
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
    
  abookptr = abook = (namelist *)malloc(sizeof(namelist) * (total +1));

  ptr = abookbuf;

  abook[total].use = -1;

  maxwidth = 0;

  while(ptr != NULL) {
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
      tempwidth = strlen(abookptr->lastname) + strlen(abookptr->firstname);
      if(tempwidth > maxwidth)
        maxwidth = tempwidth;
      abookptr++;
    } else {
      tempwidth = strlen(abookptr->lastname) + strlen(abookptr->firstname);
      if(tempwidth > maxwidth)
        maxwidth = tempwidth;
    }
  }

  current   = 0;
  start     = current;
  maxwidth += 2;

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
      case 96:
        return(oldaddress);
      break;
    }   
  }

  returnbuf = (char *)malloc(50);

  returncode = sendCon(abookfd,GET_ATTRIB,abook[current].lastname,abook[current].firstname,"email",returnbuf,50);  

  if(oldaddress)
    free(oldaddress);

  if(returncode == NOENTRY || returncode == ERROR)
    return(NULL);
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
  colourheaderrow(row);
  con_gotoxy(0,row);
  printf("___[ Modify Options ]__________________________________/ Mail v%s - %s", VERSION, headerstr);
  row++;
  colourheaderrow(row);
  con_gotoxy(0,row);
  putchar('|');
  row++;
  colourheaderrow(row);
  con_gotoxy(0,row);

  if(to && strlen(to))
    printf("|       To: [ %s ]", to);
  else
    printf("|       To: [ ]");

  row++;
  colourheaderrow(row);
  con_gotoxy(0,row);

  if(subject && strlen(subject))
    printf("|  Subject: [ %s ]",subject);  
  else
    printf("|  Subject: [ ]");  

  //deal with multiple CC's. 
  row++;
  colourheaderrow(row);
  con_gotoxy(0,row);
  if(cccount < 1) {
    printf("|       CC: [ ]");
  } else {
    printf("|       CC: [ %s ]", ccptr->line);    
  }
  if(cccount > 1) {
    for(i=1; i<cccount; i++) {
      ccptr = ccptr->nextline;
      row++;
      colourheaderrow(row);
      con_gotoxy(0,row);
      printf("|           [ %s ]", ccptr->line);
    }
  }

  //deal with multiple BCC's. (handled the EXACT same way as CC's)
  row++;
  colourheaderrow(row);
  con_gotoxy(0,row);
  if(bcccount < 1) {
    printf("|      BCC: [ ]");
  } else {
    printf("|      BCC: [ %s ]", bccptr->line);    
  }
  if(bcccount > 1) {
    for(i=1; i<bcccount; i++) {
      bccptr = bccptr->nextline;
      row++;
      colourheaderrow(row);
      con_gotoxy(0,row);
      printf("|           [ %s ]", bccptr->line);
    }
  }

  //deal with multiple attachs's. (handled the EXACT same way as CC's)
  row++;
  colourheaderrow(row);
  con_gotoxy(0,row);
  if(attachcount < 1) {
    printf("|   Attach: [ ]");
  } else {
    printf("|   Attach: [ %s ]", attachptr->line);    
  }
  if(attachcount > 1) {
    for(i=1; i<attachcount; i++) {
      attachptr = attachptr->nextline;
      row++;
      colourheaderrow(row);
      con_gotoxy(0,row);
      printf("|           [ %s ]", attachptr->line);
    }
  }

  //finally draw the header closing line. 

  row++;
  colourheaderrow(row);
  con_gotoxy(0,row);
  printf("|_______________________________________________________________________________");

  con_setfgbg(listfg_col,listbg_col);

  row++;
  con_gotoxy(0,row);
  printf("    Edit Body Contents");

  //and the commands help line at the bottom.
  colourheaderrow(con_ysize-1);
  con_gotoxy(0,con_ysize-1);
  printf(" (Q)uit to %s..., (a)ddressbook", footerstr);

  free(headerstr);
  free(footerstr);

  con_setfgbg(listfg_col,listbg_col);
}

void freemsgpreview(msgline * msglinenode) {

  while(msglinenode)
    msglinenode = remQueue(msglinenode,msglinenode);
}

void freeccbccattachs(msgline * curcc,msgline* curbcc,msgline * curattach) {

  //Free all the CC's
  if(curcc) {
    while(curcc->prevline)
      curcc = curcc->prevline;
    while(curcc) {
      free(curcc->line);
      if(curcc->nextline) {
        curcc = curcc->nextline;
        free(curcc->prevline);
      } else {
        free(curcc);
        curcc = NULL;
      }
    }
  }

  //Free all the BCC's
  if(curbcc) {
    while(curbcc->prevline)
      curbcc = curbcc->prevline;
    while(curbcc) {
      free(curbcc->line);
      if(curbcc->nextline) {
        curbcc = curbcc->nextline;
        free(curbcc->prevline);
      } else {
        free(curbcc);
        curbcc = NULL;
      }
    }
  }

  //Free all the Attaches
  if(curattach) {
    while(curattach->prevline)
      curattach = curattach->prevline;
    while(curattach) {
      free(curattach->line);
      if(curattach->nextline) {
        curattach = curattach->nextline;
        free(curattach->prevline);
      } else {
        free(curattach);
        curattach = NULL;
      }
    }
  }
}

msgline * newmsglinenode() {
  msgline * newline;

  newline = malloc(sizeof(msgline));
  newline->prevline = NULL;
  newline->nextline = NULL;

  return(newline);
}

msgline * buildmsgpreview(char * msgfilestr) {
  FILE * msgfile;
  char * line, * lineptr;
  char c;
  int charcount, eom;
  msgline *thisline, *firstline = NULL;

  msgfile = fopen(msgfilestr, "r");
  if(!msgfile)
    return(NULL);

  eom  = 0;
  line = malloc(con_xsize+1);

  while(!eom) {
    thisline  = newmsglinenode();
    lineptr   = line;
    charcount = 0;

    while(charcount < con_xsize) {
      c = fgetc(msgfile);

      switch(c) {
        case '\n':
          charcount = con_xsize;
        break;
        case EOF:
          eom = 1;
          charcount = con_xsize;
        break;
        default:
          charcount++;
          *lineptr++ = c;
      }
    }
    *lineptr = 0;

    thisline->line = strdup(line);
    firstline = addQueueB(firstline,firstline,thisline);
  }

  free(line);
  fclose(msgfile);  
  return(firstline);
}

msgline * drawmsgpreview(msgline * firstline, int cccount, int bcccount, int attachcount) {
  msgline * lastline = firstline;
  int upperscrollrow, i;

  if(!firstline) 
    return(NULL);

  //starting position without duplicates of cc, bcc, or attachments
  upperscrollrow = 9; 

  if(bcccount)
    upperscrollrow += (bcccount - 1);
  if(cccount)
    upperscrollrow += (cccount -1);
  if(attachcount)
    upperscrollrow += (attachcount -1);
 
  for(i=upperscrollrow;i<(con_ysize-1);i++) {
    con_gotoxy(0,i);
    printf("%s", lastline->line);

    if(lastline->nextline == firstline)
       break;
    lastline = lastline->nextline;
  }            

 con_setscroll(upperscrollrow,con_ysize-1);

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
    smtpserver = getmyline(smtpserver, 20,30,13,0);

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
    smtpserver = getmyline(smtpserver, 20,30,13,0);

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

    indexfilepath = malloc(strlen(serverpath) + strlen("drafts/index.xml") +1);
    sprintf(indexfilepath, "%sdrafts/index.xml", serverpath);
    draftxml = XMLloadFile(indexfilepath);
  }

  activeelemptr = XMLgetNode(draftxml, "xml/messages");

  tempint = atoi(XMLgetAttr(activeelemptr, "refnum"));
  tempint++;

  tempfilestr = malloc(strlen(serverpath) + strlen("drafts/temporary.txt")+1);
  destfilestr = malloc(strlen(serverpath) + strlen("drafts/")+20);

  sprintf(tempfilestr, "%sdrafts/temporary.txt",serverpath);
  sprintf(destfilestr, "%sdrafts/%d", serverpath, tempint);
            
  spawnlp(S_WAIT,"mv","-f",tempfilestr,destfilestr,NULL);            
    
  free(tempfilestr);
  free(destfilestr);

  tempstr = (char *)malloc(17);
  sprintf(tempstr, "%d", tempint);

  XMLsetAttr(activeelemptr, "refnum", tempstr);

  tempelemptr = XMLnewNode(NodeType_Element, "message", "");

  if(to)
    XMLsetAttr(tempelemptr, "to", to);
  else
    XMLsetAttr(tempelemptr, "to", "");

  if(subject)
    XMLsetAttr(tempelemptr, "subject", subject);
  else
    XMLsetAttr(tempelemptr, "subject", "");

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
  FILE * incoming;

  msgline * msglineptr;
  msgline * curcc     = firstcc;
  msgline * curbcc    = firstbcc;
  msgline * curattach = firstattach;
  msgline * headline, * firstline, * lastline;

  char * tempfilestr, * templine;
  int tempint, refresh;

  int arrowxpos, arrowypos, upperscrollrow, input;
  int bonuslines = 0;
  int section = 0;

  tempfilestr = malloc(strlen(serverpath) + strlen("drafts/temporary.txt") + 1);
  sprintf(tempfilestr,"%sdrafts/temporary.txt", serverpath);

  headline = firstline = buildmsgpreview(tempfilestr);

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
            else
              curcc = curcc->nextline;
          break;
          case BCC:
            movechardown(arrowxpos,arrowypos, '>');
            arrowypos++;
            if(curbcc == NULL || curbcc->nextline == NULL)
              section++;
            else
              curbcc = curbcc->nextline;
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
            if(lastline && lastline->nextline != headline) {
              firstline = firstline->nextline;
              lastline = lastline->nextline;
              con_gotoxy(0, con_ysize-2);
              putchar('\n');
              con_gotoxy(0, con_ysize-2);
              printf("%s", lastline->line);
              con_gotoxy(0, con_ysize-2);
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
            if(firstline && firstline != headline) {
              firstline = firstline->prevline;
              lastline = lastline->prevline;
              con_gotoxy(0,upperscrollrow);
              printf(scrollup);
              printf("%s", firstline->line);
              con_gotoxy(0,upperscrollrow);
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

      //addressbook
      case 'a':
        switch(section) {
          case TO:
            to = strdup(selectfromaddressbook(to));
          break;

          case CC:
            if(bonuslines > 12 && curcc != NULL)
              break;
            templine = selectfromaddressbook(NULL);
            if(!templine)
              break;

            if(strlen(templine)) {
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
                bonuslines++;
              }            

              curcc = msglineptr;              
              curcc->line = strdup(templine);
              cccount++;
            }
          break;

          case BCC:
            if(bonuslines > 12 && curbcc != NULL)
              break;
            templine = selectfromaddressbook(NULL);
            if(!templine)
              break;

            if(strlen(templine)) {
              msglineptr = malloc(sizeof(msgline));

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
                bonuslines++;
              }            

              curbcc = msglineptr;              
              curbcc->line = strdup(templine);
              bcccount++;
            }
          break;

          default:
            refresh = 0;
        }
      break;

      //edit field manually
      case '\n':
      case '\r':
        switch(section) {
          case TO:
            drawmessagebox("To:","                                ",0);
            to = getmylinerestrict(to,64,32,24,13,"",0);
          break;
          case SUBJECT:
            drawmessagebox("Subject:","                                                            ",0);
            subject = getmylinerestrict(subject,120,60,10,13,"",0); 
          break;
          case CC:
            if(curcc != NULL) {
              drawmessagebox("Edit this CC:","                                ",0);
              curcc->line = getmylinerestrict(curcc->line,64,32,24,13,"",0);
            } else {
              msglineptr = (msgline *)malloc(sizeof(msgline));

              firstcc = msglineptr;
              msglineptr->prevline = NULL;
              msglineptr->nextline = NULL;
              msglineptr->line = NULL;

              curcc = msglineptr;              
              drawmessagebox("Edit new CC:","                                ",0);
              curcc->line = getmylinerestrict(curcc->line,64,32,24,13,"",0);
              cccount++;
            }
          break;
          case BCC:
            if(curbcc != NULL) {
              drawmessagebox("Edit this BCC:","                                ",0);
              curbcc->line = getmylinerestrict(curbcc->line,64,32,24,13,"",0);
            } else {
              msglineptr = (msgline *)malloc(sizeof(msgline));

              firstbcc = msglineptr;
              msglineptr->prevline = NULL;
              msglineptr->nextline = NULL;
              msglineptr->line = NULL;

              curbcc = msglineptr;              
              drawmessagebox("Edit new BCC:","                                ",0);
              curbcc->line = getmylinerestrict(curbcc->line,64,32,24,13,"",0);
              bcccount++;
            }
          break;
          case ATTACH:
            if(bonuslines > 12 && curattach != NULL)
              break;
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
              bonuslines++;
              arrowypos++;
            }            

            con_end();    

            //add attachment from fileman. 
            spawnlp(S_WAIT, "fileman", NULL);

            prepconsole();

            //the temp file is heinous and bad. but until I figure out
            //how to do it with pipes, a temp file it shall remain.

            incoming = fopen("/wings/.fm.filepath.tmp", "r");
            getline(&buf, &size, incoming);
            fclose(incoming);

            curattach = msglineptr;              
            curattach->line = strdup(buf);
            attachcount++;
          break;
          case BODY:

            con_end();
            con_setscroll(0,0);

            //open the tempfile with ned.
            spawnlp(S_WAIT, "ned", tempfilestr, NULL);

            prepconsole();
  
            if(headline)
              freemsgpreview(headline);

            headline = firstline = buildmsgpreview(tempfilestr);
          break;
          default:
            refresh = 0;
        }        
      break;

      //remove field ... if applicable. only for CC and BCC
      case 8:
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
              if(curcc)
                bonuslines--;
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
              if(curbcc)
                bonuslines--;
              bcccount--;
            }
          break;

          case ATTACH:
            if(curattach != NULL) {
              if(curattach->prevline == NULL && curattach->nextline == NULL) {
                msglineptr  = curattach;
                curattach   = NULL;
                firstattach = NULL;
                //Arrowposition does not move. 
              } else if(curattach->prevline != NULL && curattach->nextline == NULL) {
                msglineptr = curattach;
                curattach  = msglineptr->prevline;
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
                curattach  = curattach->nextline;
                curattach->prevline = NULL;
                firstattach = curattach;
                //arrowposition does not move.
              }
              free(msglineptr);
              if(curattach)
                bonuslines--;
              attachcount--;
            }
          break;
          default:
            refresh = 0;
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
        remove(tempfilestr);
        break;
    }
  } 

  freeccbccattachs(curcc,curbcc,curattach);
  free(tempfilestr);
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
  colourheaderrow(0);

  if(howmanymessages != 1)
    printf("Mail v%s for WiNGs    %s    (%d Messages Total)          Copyright 2004", VERSION, boxtitle, howmanymessages);
  else
    printf("Mail v%s for WiNGs    %s    (%d Message)                 Copyright 2004", VERSION, boxtitle, howmanymessages);

  con_gotoxy(0,1);
  colourheaderrow(1);

  printf("[ S ]__[ FROM ]_________ _________[ SUBJECT ]______________________________[ A ]");
}

void drawmsglistboxmenu(int type) {
  con_gotoxy(1,con_ysize-1);
  colourheaderrow(con_ysize-1);

  switch(type) {
    case INBOX:
      if(abookfd != -1)
        printf(" (Q)uit, (N)ew Mail, (c)ompose, (a)ttached, (o)ther boxes..., (t)ake address");
      else
        printf(" (Q)uit, (N)ew Mail, (c)ompose, (a)ttached, (o)ther boxes...");
    break;
    case DRAFTSBOX:
      printf(" (Q)uit to inbox");
    break;
    case SENTBOX:
      printf(" (Q)uit to inbox");
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

    for(i = 2; i<con_ysize-1; i++) {
    
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
    for(i = con_ysize-2; i>1; i--) {
    
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
        return(con_ysize-2);

      message = message->PrevElem;
    }
  }
  return(con_ysize-2);
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
      if(arrowpos > con_ysize-2) {
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

    forcekeypress:

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
          arrowpos = con_ysize-2;
          con_gotoxy(0, arrowpos);
          putchar('>');
          con_update();
        }
      break;

      case '\n':
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

    drawmessagebox("back in opendrafts box","",1);

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

  drawmessagebox("just handled removing message from drafts box list.","",1);

        howmanymessages = messages->NumElements;

        lastline = rebuildlist(DRAFTSBOX,reference, direction, first, arrowpos, howmanymessages);
      break;

      case 8:
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
        input = CURD;
        goto forcekeypress;
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
            remove(tempstr);

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
      if(arrowpos > con_ysize-2) {
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

    forcekeypress:

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
          arrowpos = con_ysize-2;
          con_gotoxy(0, arrowpos);
          putchar('>');
          con_update();
        }
      break;

      case '\n':
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

      case 8:
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
        input = CURD;
        goto forcekeypress;        
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
            remove(tempstr);

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
  char *datadir, *tempstr;

  datadir = strdup(XMLgetAttr(server, "datadir"));
  tempstr = (char *)malloc(strlen("data/servers//") + strlen(datadir)+1);
  sprintf(tempstr, "data/servers/%s/", datadir);
  free(datadir);

  datadir = fpathname(tempstr, getappdir(), 1);
  free(tempstr);

  return(datadir);  
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
  lname = getmyline(strdup(""),21,29,13,0);
  if(!strlen(lname)) {
    free(lname);
    return(1);
  }
  drawmessagebox("First name?          ", " ",0);
  fname = getmyline(strdup(""),21,29,13,0);

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
      if(arrowpos > con_ysize-2) {
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
          arrowpos = con_ysize-2;
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
          drawmessagebox("Error: The message file doesn't exist","",1);
          break;
        }

        if(!strcmp(XMLgetAttr(message, "status"),"N")) {
          unread--;
          XMLsetAttr(message, "status", " ");
        }         

        //prepview returns a 1 if the message was replied to.

        if(prepforview(server,atoi(XMLgetAttr(message, "fileref")), serverpath))
          XMLsetAttr(message, "status", "R");

        lastline = rebuildlist(INBOX,reference, direction, first, arrowpos, howmanymessages);
      break;
      case 18: // C= r  stands for view raw source
        if(nomessages)
          break;

        if(!strcmp(XMLgetAttr(message, "fileref"), "")) {
          drawmessagebox("Error: The message file doesn't exist","",1);
        } else {
          tempstr = malloc(strlen(serverpath)+strlen(XMLgetAttr(message,"fileref"))+1);
          sprintf(tempstr, "%s%s", serverpath,XMLgetAttr(message,"fileref"));
          spawnlp(S_WAIT,"ned",tempstr,NULL);
          free(tempstr);
        }

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

        compose(server,NULL, serverpath, NULL, NULL, NULL, 0, NULL, 0, NULL, 0, COMPOSENEW);
        lastline = rebuildlist(INBOX,reference, direction, first, arrowpos, howmanymessages);
      break;

      case 8:
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
      drawmessagebox("        Cleaning up mail boxes...","Expunging messages marked for deletion...",0);
 
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
            remove(tempstr);

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

void mailwatch() {
  uchar *MsgP;
  int RcvID, Channel;
  int msgcount, lastmsgcount;
  activemailwatch * mw_s;

  mw_s = headmailwatch;

  while(mw_s->next)
    mw_s = mw_s->next;    

  msgcount = mw_s->theaccount->lastmsg;

  Channel = makeChan();
  setTimer(-1, mw_s->theaccount->mailcheck, 0, Channel, PMSG_Alarm);

  //printf("mailcheck every %ld\n", mw_s->theaccount->mailcheck);
        
  while(1) {
    RcvID = recvMsg(Channel, (void *)&MsgP);
     
    if(mw_s->theaccount->cancelmailcheck) 
      break;

    lastmsgcount = countservermessages(mw_s->theaccount, 1);
    if(lastmsgcount > msgcount) {
      playsound(NEWMAIL);
      msgcount = lastmsgcount;
    }    
 
    replyMsg(RcvID,-1);
    setTimer(-1, mw_s->theaccount->mailcheck, 0, Channel, PMSG_Alarm);
  }
  
  free(mw_s->theaccount->username);
  free(mw_s->theaccount->password);
  free(mw_s->theaccount->address);
  
  free(mw_s->theaccount);
  free(mw_s);
}

int stopmailwatch(DOMElement * a) {
  int input = 'a';
  char *tempstr;
  activemailwatch *prevmw_s, * tempmw_s, * mw_s_toremove;

  tempstr = malloc(81);
  sprintf(tempstr,"Mailwatch is running on account '%s'.", XMLgetAttr(a,"name"));
  drawmessagebox(tempstr,"Do you want to stop automatic mail checking on this account now? (y/n)",0);   

  while(input != 'y' && input != 'n')
    input = con_getkey();

  if(input == 'n') {
    free(tempstr);
    return(-1);
  }  	

  sprintf(tempstr, "Stopping mailwatch on account '%s'.", XMLgetAttr(a, "name"));
  drawmessagebox(tempstr,"",0);

  mw_s_toremove = NULL;

  if(headmailwatch->servernode == a) {
    mw_s_toremove = headmailwatch;
    headmailwatch = headmailwatch->next;
  } else {
    tempmw_s = headmailwatch->next;
    prevmw_s = headmailwatch;
    while(tempmw_s) {
      if(tempmw_s->servernode == a) {
        mw_s_toremove = tempmw_s;
        prevmw_s->next = mw_s_toremove->next;
        break;
      }
      prevmw_s = tempmw_s;
      tempmw_s = tempmw_s->next;
    }
  }
  
  if(mw_s_toremove)
    mw_s_toremove->theaccount->cancelmailcheck = 1;

  XMLsetAttr(a, "mailwatch", "   ");

  free(tempstr);
  return(0);
}

int startmailwatch(DOMElement * a) {
  DOMElement * indexXML;
  int input;
  char * minutes, *path, *tempstr;
  accountprofile  *newprofile;
  activemailwatch *tempmw_s, *findend;

  tempstr = malloc(81);
  sprintf(tempstr, "Mailwatch is not running on '%s'.", XMLgetAttr(a, "name"));
  drawmessagebox(tempstr, "Do you want to start automatic mail checking on this account now? (y/n)",0);
  free(tempstr);

  while(input != 'y' && input != 'n')
    input = con_getkey();

  if(input == 'n')
    return(-1);

  drawmessagebox("How many minutes delay between new mail checks?", " ",0);
  minutes = getmylinen(9,16,13);

  if(!strlen(minutes) || !atoi(minutes))
    return(-1);

  //create a new mailwatch struct. This connects a servernode 
  //with a mailwatch accountprofile.

  tempmw_s = malloc(sizeof(activemailwatch));
  tempmw_s->next = NULL;

  //Check to see if a header node already exists, if not
  //make this new node the header node

  if(!headmailwatch)
    headmailwatch = tempmw_s;

  //start at the header node, and follow the next links to the
  //end of the singly linked list.

  findend = headmailwatch;
  while(findend->next)
    findend = findend->next;

  //if the last node is not the new node, then set the last
  //node's next to point to the new node

  if(findend != tempmw_s)
    findend->next = tempmw_s;

  //finally, use the new node to connect the servernode with 
  //the mailwatch accountprofile

  newprofile = malloc(sizeof(accountprofile));

  tempmw_s->theaccount = newprofile;
  tempmw_s->servernode = a;

  XMLsetAttr(a, "mailwatch", "*M*");

  path    = fpathname("data/servers/", getappdir(), 1);
  tempstr = (char *)malloc(strlen(path) + 16 + strlen("index.xml") + 1);
  sprintf(tempstr, "%s%s/index.xml", path, XMLgetAttr(a, "datadir"));

  //drawmessagebox("path:",tempstr,1);

  indexXML = XMLloadFile(tempstr);

  free(path);
  free(tempstr);

  newprofile->address   = strdup(XMLgetAttr(a, "address"));
  newprofile->username  = strdup(XMLgetAttr(a, "username"));
  newprofile->password  = strdup(XMLgetAttr(a, "password"));

  newprofile->lastmsg = atoi(XMLgetAttr(XMLgetNode(indexXML, "xml/messages"), "firstnum"))-1;
  newprofile->cancelmailcheck = 0;

  newprofile->mailcheck = strtoul(minutes,NULL,10);
  newprofile->mailcheck *= 60000; //make it in microseconds

  newThread(mailwatch,STACK_DFL,NULL);
  return(0);
}

void drawinboxselectscreen() {
  DOMElement * splashlogo;

  con_setfgbg(logofg_col,logobg_col);
  con_clrscr();
  
  // DRAW LOGO
  con_gotoxy(0,0);
  splashlogo = XMLgetNode(configxml, "xml/splashlogo");
  printf("%s", splashlogo->Node.Value);

  con_gotoxy(8,16);
  printf("________________________________________________________________");
  con_gotoxy(8,22);
  printf("|_______________________________________________________________|");

  colourheaderrow(con_ysize-1);
  con_gotoxy(0,con_ysize-1);
  printf(" (Q)uit, (n)ew account, (e)dit account, turn (m)ailwatch ");
  con_setfgbg(logofg_col,logobg_col);
}

int drawinboxselectlist(DOMElement * servernode) {
  int i = 0;
  int j;

  con_setscroll(17,22);

  while(i < 5) {
    con_gotoxy(8,17+i);
    printf("|   %s %s ",XMLgetAttr(servernode,"mailwatch"),XMLgetAttr(servernode,"name"));
    con_gotoxy(55,17+i);
    printf("(%s unread) ", XMLgetAttr(servernode, "unread"));
    con_gotoxy(72,17+i);
    printf("|");

    if(servernode->NextElem->FirstElem) {
      for(j=i+1;j<5;j++) {
        con_gotoxy(8,17+j);
        printf("|%62s|","");
      }
      return(i);
    }

    servernode = servernode->NextElem;
    i++;
  }

  if(!servernode->FirstElem) {
    con_gotoxy(37,22);
    printf("(more)");
  }

  return(i);
}

void mailwatchmenuitem(DOMElement * cserver) {
  con_setfgbg(COL_Blue,COL_Blue);
  if(!strcmp(XMLgetAttr(cserver,"mailwatch"),"*M*")) {
    con_gotoxy(57,con_ysize-1);
    printf("off");
  } else {
    con_gotoxy(57,con_ysize-1);
    printf("on ");
  }
  con_setfgbg(logofg_col,logobg_col);
}

void metallica() {
  con_setfgbg(COL_White,COL_Blue);
  con_setscroll(0,con_ysize-1);
  con_clrscr();
  con_gotoxy(0,0);
  printf("\n\n\n");
  printf("                  .-/                                      .-.\n");
  printf("              _.-~ /  _____  ______ __  _    _     _   ___ | ~-._\n");
  printf("              %c:/  -~||  __||_  __//  || |  | |  /| | / __/| .%c:/\n",'\\');
  printf("               /     ||  __|:| |%c:/ ' || |__| |_/:| || (:/:|   %c\n",'\\','\\');
  printf("              / /%c/| ||____|:|_|:/_/|_||____|____||_|:%c___%c| |%c %c\n",'\\','\\','\\','\\','\\');
  printf("             / /:::|.:%c::::%c:%c:%c:|:||:||::::|:::://:/:/:::/:.|:%c %c\n",'\\','\\','\\','\\','\\','\\');
  printf("            / /:::/ %c::%c::::%c|%c:%c|:/|:||::::|::://:/%c/:::/::/:::%c %c\n",'\\','\\','\\','\\','\\','\\','\\','\\');
  printf("           /  .::%c   %c-~~~~~~~ ~~~~  ~~ ~~~~~~~~ ~~  ~~~~~-/%c/:..  %c\n",'\\','\\','\\','\\');
  printf("          /..:::::%c                                         /:::::..%c\n",'\\','\\');
  printf("         /::::::::-                                         -::::::::%c\n",'\\');
  printf("         %c:::::-~                                              ~-:::::/\n",'\\');
  printf("          %c:-~                                                    ~-:/\n",'\\');

  con_update();
  con_getkey();
  con_clrscr();
}

void megadeth() {
  con_setfgbg(COL_White,COL_Red);
  con_setscroll(0,con_ysize-1);
  con_clrscr();
  con_gotoxy(0,0);
  printf("\n\n\n");
  printf("                            .AMMMMMMMMMMA.          \n");
  printf("                           .AV. :::.:.:.::MA.        \n");
  printf("                          A' :..        : .:`A       \n");
  printf("                         A'..              . `A.     \n");
  printf("                        A' :.    :::::::::  : :`A    \n");
  printf("                        M  .    :::.:.:.:::  . .M    \n");
  printf("                        M  :   ::.:.....::.:   .M    \n");
  printf("                        V : :.::.:........:.:  :V    \n");
  printf("                       A  A:    ..:...:...:.   A A   \n");
  printf("                      .V  MA:.....:M.::.::. .:AM.M   \n");
  printf("                     A'  .VMMMMMMMMM:.:AMMMMMMMV: A  \n");
  printf("                    :M .  .`VMMMMMMV.:A `VMMMMV .:M: \n");
  printf("                     V.:.  ..`VMMMV.:AM..`VMV' .: V  \n");
  printf("                      V.  .:. .....:AMMA. . .:. .V   \n");
  printf("                       VMM...: ...:.MMMM.: .: MMV    \n");
  printf("                           `VM: . ..M.:M..:::M'      \n");
  printf("                             `M::. .:.... .::M       \n");
  printf("                              M:.  :. .... ..M       \n");
  printf("                     VK       V:  M:. M. :M .V       \n");
  printf("                              `V.:M.. M. :M.V'       ");

  con_update();
  con_getkey();
  con_clrscr();
}

void inboxselect() {
  DOMElement *temp, *server, *cserver;
  int unread, arrowpos, arrowhpos, servercount, mdethpos = 0, mtlcapos = 0;
  char * inboxname;
  int input;
  int noservers = 0;
  soundsprofile * soundfiles;
  char mdeth[8] = {'M','E','G','A','D','E','T','H'};
  char mtlca[9] = {'M','E','T','A','L','L','I','C','A'};

  arrowhpos = 10;

  soundfiles = initsoundsettings();

  temp = XMLgetNode(configxml, "xml/servers");
  servercount = temp->NumElements;

  servercheck:

  server = XMLgetNode(temp, "server");

  if(server == NULL) {
    noservers = 1;
    server = XMLnewNode(NodeType_Element, "server", "");
    XMLsetAttr(server, "name", "No accounts configured.");
    XMLsetAttr(server, "address", "");
    XMLsetAttr(server, "username", "");
    XMLsetAttr(server, "password", "");
    XMLsetAttr(server, "unread", "0");
    XMLsetAttr(server, "mailwatch", "   ");
    server->FirstElem = 1;
    server->PrevElem = server;
    server->NextElem = server;
  } else {
    //initialize all "mailwatch" attributes to "   "
    
    //Possible future expansion... if mailwatch was read in from
    //disk already set to "*M*", you could start mailwatch... but 
    //there is currently nothing written out to remember the delay
    //in minutes. 
    
    cserver = server;
    do {
      XMLsetAttr(cserver, "mailwatch", "   ");
      cserver = cserver->NextElem;
    } while(cserver != server);
  }

  drawinboxselectscreen();
  drawinboxselectlist(server);

  if(!noservers)
    mailwatchmenuitem(server);    

  arrowpos = 17;
  con_gotoxy(arrowhpos,arrowpos);
  putchar('>');

  input = 0;
  while(input != 'Q') {
    con_update();
    input = con_getkey();  
  
    switch(input) {
      case CURD:
        if(!cserver->NextElem->FirstElem) {
          cserver = cserver->NextElem;

          if(arrowpos < 21) {
            mailwatchmenuitem(cserver);
            movechardown(arrowhpos, arrowpos, '>');
            arrowpos++;
          } else {
            printf("\n");
            con_gotoxy(8,21);
            printf("|   %s %s ",XMLgetAttr(cserver,"mailwatch"),XMLgetAttr(cserver,"name"));
            con_gotoxy(55,21);
            printf("(%s unread) ", XMLgetAttr(cserver, "unread"));
            con_gotoxy(72,21);
            printf("|");
            con_gotoxy(37,16);
            printf("(more)");
            if(cserver->NextElem->FirstElem) {
              con_gotoxy(37,22);
              printf("______");
            }

            mailwatchmenuitem(cserver);
            movechardown(arrowhpos, arrowpos-1, '>');
          }
        }
      break;
      case CURU:
        if(!cserver->FirstElem) {
          cserver = cserver->PrevElem;

          if(arrowpos > 17) {
            mailwatchmenuitem(cserver);
            movecharup(arrowhpos, arrowpos, '>');
            arrowpos--;
          } else {
            con_gotoxy(arrowhpos,17);
            printf("\x1b[1L");
            con_gotoxy(8,17);
            printf("|   %s %s ",XMLgetAttr(cserver,"mailwatch"),XMLgetAttr(cserver,"name"));
            con_gotoxy(55,17);
            printf("(%s unread) ", XMLgetAttr(cserver, "unread"));
            con_gotoxy(72,17);
            printf("|");
            if(cserver->FirstElem) {
              con_gotoxy(37,16);
              printf("______");
            }
            if(servercount > 5) {
              con_gotoxy(37,22);
              printf("(more)");
            }

            mailwatchmenuitem(cserver);
            movecharup(arrowhpos, arrowpos+1, '>');
          }
        }
      break;

      case 'n':
        if(addserver(temp)) {
          XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(), 1));
          servercount++;
          if(noservers) {
            noservers = 0;
            cserver = server = XMLgetNode(temp, "server");
          }
          system("sync");
        }
        drawinboxselectscreen();
        drawinboxselectlist(server);

        cserver = server;   
        mailwatchmenuitem(cserver);
        arrowpos = 17;
        con_gotoxy(arrowhpos,arrowpos);
        putchar('>');
        con_update();
      break;
      case 8:
        if(noservers)
          break;
        if(deleteserver(cserver)) {
          XMLremNode(cserver);
          XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(),1));
          system("sync");
          servercount--;
          if(!servercount)
            goto servercheck;
          else
            cserver = server = XMLgetNode(temp, "server");
        }
        drawinboxselectscreen();
        drawinboxselectlist(server);
   
        cserver = server;   
        mailwatchmenuitem(cserver);
        arrowpos = 17;
        con_gotoxy(arrowhpos,arrowpos);
        putchar('>');
        con_update();
      break;

      case 'e':
        if(noservers)
          break;
        if(editserver(cserver, soundfiles))
          XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(), 1));

        soundsettings = setupsounds(soundsettings);
        system("sync");

        drawinboxselectscreen();
        drawinboxselectlist(server);
   
        cserver = server;   
        mailwatchmenuitem(cserver);
        arrowpos = 17;
        con_gotoxy(arrowhpos,arrowpos);
        putchar('>');
        con_update();
      break;

      case 'm':
        if(noservers)
          break;

        if(!strcmp(XMLgetAttr(cserver,"mailwatch"),"*M*"))
          stopmailwatch(cserver);
        else 
          startmailwatch(cserver);

        drawinboxselectscreen();
        drawinboxselectlist(server);
   
        cserver = server;   
        mailwatchmenuitem(cserver);
        arrowpos = 17;
        con_gotoxy(arrowhpos,arrowpos);
        putchar('>');
        con_update();
      break;

      case CURR:
      case '\r':
      case '\n':
        if(noservers)
          break;
        openinbox(cserver);
        system("sync");

        drawinboxselectscreen();
        drawinboxselectlist(server);
   
        cserver = server;   
        mailwatchmenuitem(cserver);
        arrowpos = 17;
        con_gotoxy(arrowhpos,arrowpos);
        putchar('>');
        con_update();
      break;
      default:
        if(input != mdeth[mdethpos++])
          mdethpos = 0;
        if(input != mtlca[mtlcapos++])
          mtlcapos = 0;

        if(mdethpos > 7) {
          mdethpos = 0;
          megadeth();

          drawinboxselectscreen();
          drawinboxselectlist(server);

          arrowpos = 17;
          con_gotoxy(arrowhpos,arrowpos);
          putchar('>');
          con_update();
        }

        if(mtlcapos > 8) {
          mtlcapos = 0;
          metallica();

          drawinboxselectscreen();
          drawinboxselectlist(server);

          arrowpos = 17;
          con_gotoxy(arrowhpos,arrowpos);
          putchar('>');
          con_update();
        }
      break;
    }
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
    printf("Mail V%s for WiNGs will only run on\nan 80 column console or wider.  Press\nCommodore key and Backarrow together\nto switch to a higher console driver", VERSION);
    con_update();
    exit(EXIT_FAILURE);
  }

  path = fpathname("resources/mailconfig.xml", getappdir(), 1);
  configxml = XMLloadFile(path);

  soundsettings = initsoundsettings();
  setupcolors();

  //Establish connection to AddressBook Service. 

  if((abookfd = open("/sys/addressbook", O_PROC)) == -1) {
    system("addressbookd");
    if((abookfd = open("/sys/addressbook", O_PROC)) == -1)
      drawmessagebox("The addressbook service could not be started.","          Press a key to continue.           ",1);
  }

  playsound(HELLO);

  inboxselect(); //The program never leaves the inboxselect function, 'til
                 //the user is quitting the program.

  playsound(GOODBYE);

  con_end();
  con_reset();
  con_clrscr();
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

  getMutex(&exclservercon);

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

  relMutex(&exclservercon);
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
      } else {
        totalsize += msgsize;
      }
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
  int count, i, eom, returnvalue, gotfilename;
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

    subject = NULL;
    from    = NULL;

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

            gotfilename = 0;

            while(!((buf[0]=='\n')||(buf[0]=='\r'))) {
              progbarcounter += getline(&buf, &size, fp);
              setprogress(mb,progbarcounter);

              //The last boundary is right before the end of the message

              if((buf[0]=='.'&&buf[1]=='\n')||(buf[0]=='.'&&buf[1]=='\r')) {
                eom = 1;
                break;
              }
 
              //Get attached file's name, and create XML child. 

              if(strstr(buf, "name") && !gotfilename) {
                attachments++;
                gotfilename++;

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
    } else {
      subject = strdup("subject:  ");
    }

    XMLsetAttr(message, "status", "N");
    XMLsetAttr(message, "from", &from[6]);
    XMLsetAttr(message, "subject", &subject[9]);
    XMLsetAttr(message, "attachments", itoa(attachments));
    XMLsetAttr(message, "fileref", itoa(refnum));
    XMLinsert(messages, NULL, message);
    
    refnum++;
  } // Loop Back up to get next new message.

  terminateconnection();

  if(deletefromserver) {
    establishconnection(aprofile);

    fflush(fp);
    fprintf(fp, "LIST\r\n");

    fflush(fp);
    count = 0;

    getline(&buf, &size, fp);

    do {
      count++;
      getline(&buf, &size, fp);
    } while(buf[0] != '.');

    terminateconnection();

    //note count will end up one too high, which is fine, 
    //we just don't add one when we write it out to the xml file.

    XMLsetAttr(messages, "firstnum", itoa(count));
  } else {
    count++;
    XMLsetAttr(messages, "firstnum", itoa(count));
  }

  XMLsetAttr(messages, "refnum", itoa(refnum));

  return(returnvalue);
}

char * terminateheaderstr(char * string, int maxlen) {
  char * ptr;

  if(!maxlen)
    maxlen = con_xsize - strlen("Precedence: ");
  
  ptr = string;

  if(!ptr)
    return(strdup(""));

  while(*ptr != '\r' && *ptr != ';' && ptr != NULL) {
    ptr++;
    if(ptr-string == maxlen)
      break;
  }
  
  *ptr = 0;
  return(strdup(string));
}

void freedisplayheader(displayheader * killme) {
  
  free(killme->date);
  free(killme->from);
  free(killme->cc);
  free(killme->subject);
  free(killme->priority);
  free(killme->precedence);
  free(killme->xmailer);

  free(killme);
}

displayheader * getdisplayheader(FILE * fp) {
  displayheader * header;
  char *headerstr,* ptr;
  int bufsize, lastsize;

  header = malloc(sizeof(displayheader));

  header->date       = NULL;
  header->from       = NULL;
  header->cc         = NULL;
  header->subject    = NULL;
  header->priority   = NULL;
  header->precedence = NULL;
  header->xmailer    = NULL;

  bufsize = 1024;
  headerstr = ptr = malloc(bufsize);

  // Fetch complete header to a maximum of 8K

  while(!(ptr[-4] == 13 && ptr[-3] == 10 && ptr[-2] == 13 && ptr[-1] == 10)) {
    if(ptr - headerstr == bufsize) {
      lastsize = bufsize;
      bufsize *=2;
      headerstr = realloc(headerstr, bufsize);
      ptr = headerstr+lastsize;
    }
    *ptr++ = fgetc(fp);
  }

  fclose(fp);

  ptr = strcasestr(headerstr, "\r\nDate:");
  if(ptr)
    header->date = ptr+8;

  ptr = strcasestr(headerstr, "\r\nFrom:");
  if(ptr)
    header->from = ptr+8;

  ptr = strcasestr(headerstr, "\r\ncc:");
  if(ptr)
    header->cc = ptr+6;

  ptr = strcasestr(headerstr, "\r\nSubject:");
  if(ptr)
    header->subject = ptr+11;

  ptr = strcasestr(headerstr, "\r\nX-priority:");
  if(ptr)
    header->priority = ptr+14;

  ptr = strcasestr(headerstr, "\r\nPrecedence:");
  if(ptr)
    header->precedence = ptr+14;

  ptr = strcasestr(headerstr, "\r\nX-mailer:");
  if(ptr)
    header->xmailer = ptr+12;
  
  header->date       = terminateheaderstr(header->date,0);
  header->from       = terminateheaderstr(header->from,0);
  header->cc         = terminateheaderstr(header->cc,0);
  header->subject    = terminateheaderstr(header->subject,0);
  header->priority   = terminateheaderstr(header->priority,1);
  header->precedence = terminateheaderstr(header->precedence,0);
  header->xmailer    = terminateheaderstr(header->xmailer,0);

  free(headerstr);

  return(header);
}

void freemimeheader(mimeheader * killme) {

  free(killme->contenttype);
  free(killme->encoding);
  free(killme->boundary);
  free(killme->filename);
  free(killme->disposition);
  
  free(killme);
}

mimeheader * getmimeheader(FILE * msgfile) {
  mimeheader * header;
  char * headerstr, * ptr;
  int bufsize, lastsize;  

  header = malloc(sizeof(mimeheader));

  header->contenttype = NULL;
  header->encoding    = NULL;
  header->boundary    = NULL;
  header->filename    = NULL;
  header->disposition = NULL;

  bufsize = 1024;
  headerstr = ptr = malloc(bufsize);

  // 1k buffer grows if size exceeded.

  while(!(ptr[-4] == 13 && ptr[-3] == 10 && ptr[-2] == 13 && ptr[-1] == 10)) {
    if(ptr - headerstr == bufsize) {
      lastsize = bufsize;
      bufsize *=2;
      headerstr = realloc(headerstr, bufsize);
      ptr = headerstr+lastsize;
    }
    *ptr++ = fgetc(msgfile);
  }

  ptr = strcasestr(headerstr, "\r\ncontent-type:");
  if(ptr)
    header->contenttype = ptr+16;
  else {
    ptr = strcasestr(headerstr, "content-type:");
    if(ptr)
      header->contenttype = ptr+14;
  }

  ptr = strcasestr(headerstr, "\r\ncontent-transfer-encoding:");
  if(ptr)
    header->encoding = ptr+29;

  ptr = strcasestr(headerstr, "\r\ncontent-disposition:");
  if(ptr)
    header->disposition = ptr+23;


  if(header->disposition) {
    ptr = strcasestr(header->disposition, "filename=");
    if(ptr)
      header->filename = ptr+10;
  }

  if(!strncasecmp(header->contenttype,"multipart/", 10)) {
    ptr = strcasestr(header->contenttype, "boundary=");
    if(ptr)
      header->boundary = ptr+10;
  }
    
  header->contenttype = terminateheaderstr(header->contenttype,0);  
  header->encoding    = terminateheaderstr(header->encoding,0);  
  header->disposition = terminateheaderstr(header->disposition,0);  

  if(header->filename)
    *strchr(header->filename,'"') = 0;
  header->filename = strdup(header->filename);

  if(header->boundary)
    *strchr(header->boundary, '"') = 0;
  header->boundary = strdup(header->boundary);

  free(headerstr);

  return(header);
}

msgline * parsehtmlcontent(msgline * firstline) {
  int linelen, charcount, eom, c;
  msgline * thisline, * prevline;
  char * line, * lineptr;
  FILE * incoming;

  //Initial settings; 

  //Leave a space for Quote Character in a reply
  linelen  = con_xsize -1;
  prevline = NULL;
  eom      = 0;

  //Allocate for Null terminator
  line     = malloc(linelen + 1);

  pipe(writetowebpipe);
  pipe(readfromwebpipe);

  redir(writetowebpipe[0], STDIN_FILENO);
  redir(readfromwebpipe[1], STDOUT_FILENO);
  spawnlp(0, "web", NULL);
  close(readfromwebpipe[1]);
  close(writetowebpipe[0]);

  incoming = fdopen(readfromwebpipe[0], "r");  

  htmlfirstline = firstline;

  newThread(feedhtmltoweb, STACK_DFL, NULL);
	
  while(!eom) {
    charcount = 0;
	
    //Create a new line struct. setting the Prev and Next line pointers.
    thisline = malloc(sizeof(msgline));
    thisline->prevline = prevline;
    if(prevline)
      prevline->nextline = thisline;
	
    //Clear the line buffer;
    memset(line, 0, linelen + 1);
    lineptr = line;

    while(charcount < linelen) {
      c = fgetc(incoming);
		 
      switch(c) {
        case '\n':
          //end msgline struct here. 
          charcount = linelen;
        break;
        case '\r':
          //Do Nothing... simply leave it out.
        break;
        case EOF:
          eom = 1;
          charcount = linelen;
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

  thisline->nextline = NULL;
  while(thisline->prevline)
    thisline = thisline->prevline;

  return(thisline);
}

char * gettextfrommsg(FILE * msgfile) {
  char * buffer, * ptr;
  int c,lastplace,sizeofbuf = 2000;

  ptr = buffer = malloc(sizeofbuf);

  while((c = fgetc(msgfile)) != EOF) {
    if(ptr-buffer > sizeofbuf) {
      lastplace = sizeofbuf;
      sizeofbuf *= 2;
      buffer = realloc(buffer,sizeofbuf);
      ptr = buffer+lastplace;
    }
    *ptr++ = c;
  }

  //terminate the buffer like it's one Gi-Normous string
  *ptr = 0;

  return(buffer); 
}

char * gettextfrommime(char * boundary, FILE * msgfile) {
  char * buffer, * ptr;
  int c,lastplace,sizeofbuf = 2000;
  int blen;

  blen = strlen(boundary);

  //drawmessagebox(boundary,itoa(blen), 1);

  ptr = buffer = malloc(sizeofbuf);

  while((c = fgetc(msgfile)) != EOF) {
    if(ptr-buffer > sizeofbuf) {
      lastplace = sizeofbuf;
      sizeofbuf *= 2;
      buffer = realloc(buffer,sizeofbuf);
      ptr = buffer+lastplace;
    }
    *ptr++ = c;
    //*ptr = 0;
    //drawmessagebox("looking for the boundary", ptr-blen, 1);
    if(!strncmp(boundary,ptr-blen,blen)) {
      ptr = ptr-blen;
      break;
    }
  }

  //terminate the buffer like it's one Gi-Normous string
  *ptr = 0;

  return(buffer); 
}

//ASSEMBLE A Text Only Message (text may contain HTML)

msgline * assembletextmessage(mimeheader * mainmimehdr, char * buffer) {
  int linelen, charcount, eom, qp, c;
  msgline * thisline, * prevline;
  char * line, * lineptr, *bufferptr;
  char qphexbuf[3];

  //Initial settings; 

  //Leave a space for Quote Character in a reply
  linelen  = con_xsize -1;
  prevline = NULL;
  eom      = 0;

  //Allocate for Null terminator
  line     = malloc(linelen + 1);

  //Set Quoted Printable Flag
  if(strcasestr(mainmimehdr->encoding,"quoted-printable"))
    qp = 1;
  else
    qp = 0;

  if(buffer)
    bufferptr = buffer;

  while(!eom) {
    charcount = 0;

    //Create a new line struct. setting the Prev and Next line pointers.
    thisline = malloc(sizeof(msgline));
    thisline->prevline = prevline;
    if(prevline)
      prevline->nextline = thisline;

    //Clear the line buffer;
    memset(line, 0, linelen + 1);
    lineptr = line;

    while(charcount < linelen) {
      c = *bufferptr++;

      switch(c) {
        case '\n':
          //end msgline struct here. 
          charcount = linelen;
        break;
        case '\r':
          //Do Nothing... simply leave it out.
        break;
        case 0:
          eom = 1;
          charcount = linelen;
        break;
        case '=':
          if(qp) {
            c = *bufferptr++;
            if(c == 0) {
              eom = 1;
              charcount = linelen;
              break;
            }
            if(c == '\n' || c == '\r') 
              break; 
              //do nothing. don't store the = or the \n
			
            //Check for valid character.
            if((c < '0' || c > '9') && (c < 'A' || c > 'F')) { 
              charcount++;
              *lineptr = '=';
              lineptr++;
            } else {
              qphexbuf[0] = c;
              c = *bufferptr++;
              if(c == 0) {
                eom = 1;
                charcount = linelen;
                break;
              }
              qphexbuf[1] = c;
              qphexbuf[2] = 0;

              c = (int)strtoul(qphexbuf, NULL, 16);
							
              charcount++;
              *lineptr = c;
              lineptr++;
            }
            break;
          }
          //if !qp let fall through to default;
        default:
          //Any other character is added to the line
          charcount++;
          *lineptr = c;
          lineptr++;
        break;
      }
    }
    thisline->line = strdup(line);
    prevline = thisline;
  }
  thisline->nextline = NULL;

  while(thisline->prevline)
    thisline = thisline->prevline;

  if(strcasestr(mainmimehdr->contenttype, "html"))
    thisline = parsehtmlcontent(thisline);

  return(thisline);
}

//ASSEMBLE A MultiPart Message

msgline * assemblemultipartmessage(mimeheader * mainmimehdr, FILE * msgfile) {
  mimeheader * submimeheader;
  msgline * firstline = NULL;
  char * contentbuffer = NULL, *bufptr;
  int contentbuffersize, boundarylen;

  //skip blurb before first mimesection
  buf  = NULL;
  size = 0;
  while(!strstr(buf,mainmimehdr->boundary)) {
    drawmessagebox("searching for boundary","",1);
    getline(&buf, &size, msgfile);
  }

  submimeheader = getmimeheader(msgfile);

  drawmessagebox("submime header contenttype", submimeheader->contenttype,1);

  if(strstr(submimeheader->contenttype,"text/"))
    firstline = assembletextmessage(submimeheader,gettextfrommime(mainmimehdr->boundary, msgfile));

  return(firstline);
}

void feedhtmltoweb() {
  FILE * output;  
  msgline * templine;

  output = fdopen(writetowebpipe[1], "w");

  while(htmlfirstline) {  
    templine = htmlfirstline->nextline;
    fprintf(output, "%s", htmlfirstline->line);
    free(htmlfirstline->line);
    free(htmlfirstline);
    htmlfirstline = templine;
  }

  fclose(output);
}

int prepforview(DOMElement * server, int fileref, char * serverpath){
  displayheader * displayhdr;
  mimeheader * mainmimehdr;
  msgline * firstline;
  int returnval;
  char * tempstr;

  tempstr = malloc(strlen(serverpath)+17);
  sprintf(tempstr, "%s%d", serverpath, fileref);

  msgfile = fopen(tempstr, "r");
    if(!msgfile) {
      drawmessagebox("An internal error has occurred. File Not Found.", "",1);
      return(0);
    } 
    displayhdr = getdisplayheader(msgfile);
  fclose(msgfile);
  
  msgfile = fopen(tempstr, "r");
  mainmimehdr = getmimeheader(msgfile);

  free(tempstr);

  drawmessagebox("contenttype", mainmimehdr->contenttype,1);
  drawmessagebox("encoding",mainmimehdr->encoding,1);

  //May be text/plain or text/html; Handled in assembletextmessage();

  if(!strncasecmp(mainmimehdr->contenttype,"text/", 5))
    firstline = assembletextmessage(mainmimehdr,gettextfrommsg(msgfile));

  //May be multipart/mixed or multipart/alternative	

  else if(!strncasecmp(mainmimehdr->contenttype,"multipart/", 10))
    firstline = assemblemultipartmessage(mainmimehdr,msgfile);

  //Default to assume text/plain

  else
    firstline = assembletextmessage(mainmimehdr,gettextfrommsg(msgfile));

  fclose(msgfile);

  if(!firstline) {
    firstline = malloc(sizeof(msgline));
    firstline->prevline = NULL;
    firstline->nextline = NULL;
    firstline->line = strdup("  This email has no readable text portions.");
  }

  returnval = view(firstline, displayhdr, server, serverpath);

  freedisplayheader(displayhdr);
  freemimeheader(mainmimehdr);

  while(firstline->nextline) {
    firstline = firstline->nextline;
    free(firstline->prevline->line);
    free(firstline->prevline);
  }
  free(firstline->line);
  free(firstline);

  return(returnval);
}

void colourheaderrow(int rownumber) {
  con_gotoxy(0,rownumber);
  con_setfgbg(COL_Blue, COL_Blue);
  con_clrline(LC_End);
}

int drawviewheader(displayheader * header, int bighdr) {
  int i,row = 0;

  if(strlen(header->date) && bighdr) {
    colourheaderrow(row);
    printf("      Date: %s", header->date);
    row++;
  }

  if(strlen(header->from)) {
    colourheaderrow(row);
    printf("      From: %s", header->from);
    row++;    
  }
  
  if(strlen(header->cc) && bighdr) {
    colourheaderrow(row);
    printf("        CC: %s", header->cc);
    row++;
  }

  if(strlen(header->subject)) {
    colourheaderrow(row);
    printf("   Subject: %s", header->subject);
    row++;
  }
  
  if(strlen(header->priority) && bighdr) {
    colourheaderrow(row);
    printf("  Priority: (%s) ", header->priority);
    switch(atoi(header->priority)) {
      case 1:
        printf("Highest");
      break;
      case 2:
        printf("High");
      break;
      case 3:
        printf("Normal");
      break;
      case 4:
        printf("Low");
      break;
      case 5:
        printf("Lowest");
      break;
    }
    row++;
  } else if(bighdr) {
    colourheaderrow(row);
    printf("  Priority: (3) Normal");
    row++;
  }
   
  if(strlen(header->precedence) && bighdr) {
    colourheaderrow(row); 
    printf("Precedence: %s", header->precedence);
    row++;
  }

  if(strlen(header->xmailer) && bighdr) {
    colourheaderrow(row);
    printf(" Sent with: %s", header->xmailer);
    row++;
  }

  colourheaderrow(row);
  for(i=0;i<con_xsize;i++) {
    con_gotoxy(i,row);
    putchar('_');
  }
  row++;

  return(row);
}

void drawviewmenu(int bighdr) {

  colourheaderrow(con_ysize-1);

  con_gotoxy(0,con_ysize-1);
  if(!bighdr)
    printf(" (Q)uit to list, (r)eply, full (h)eader");
  else
    printf(" (Q)uit to list, (r)eply, condensed (h)eader");
}

int view(msgline * firstline, displayheader * displayhdr, DOMElement * server, char * serverpath) {
  int input, i, msgstartrow, bighdr = 0;
  msgline *topofview, *thisline;

  FILE * replyfile;
  char *tempstr, * replyto, *replysubject;
  int replied = 0;

  topofview = firstline;
  thisline  = firstline;

  cleandisplay:

  con_clrscr();

  msgstartrow = drawviewheader(displayhdr, bighdr);
  drawviewmenu(bighdr);

  con_setfgbg(COL_Cyan, COL_Black);

  for(i=msgstartrow;i<con_ysize-1;i++) {
    con_gotoxy(0, i);
    printf("%s", thisline->line);
    if(thisline->nextline)
      thisline = thisline->nextline;
    else
      break;
  }  
  if(thisline->nextline)
    thisline = thisline->prevline;

  con_update();
  con_setscroll(msgstartrow,con_ysize-1);

  input = 'a';
  while(input != 'Q' && input != CURL) {

    input = con_getkey();

    switch(input) {
      case CURD:
        if(thisline->nextline) {
          topofview = topofview->nextline;
          thisline = thisline->nextline;
          con_gotoxy(0, con_ysize-1);
          putchar('\n');
          con_gotoxy(0, con_ysize-2);
          printf("%s", thisline->line);
          con_gotoxy(0, con_ysize-2);
          con_update();
        }
      break;
      case CURU:
        if(topofview->prevline) {
          topofview = topofview->prevline;
          thisline = thisline->prevline;
          con_gotoxy(0,msgstartrow);
          printf("\x1b[1L");
          printf("%s", topofview->line);
          con_gotoxy(0,msgstartrow);
          con_update();
        }
      break;
      case 'h':
        if(!bighdr)
          bighdr = 1;
        else
          bighdr = 0;

        thisline = topofview;
        goto cleandisplay;
      break;
      case 'r':
        //Write message lines to drafts/temporary.txt with quote markers.
        tempstr = malloc(strlen(serverpath)+strlen("drafts/temporary.txt")+1);

        sprintf(tempstr, "%sdrafts/temporary.txt", serverpath);
          
        replyfile = fopen(tempstr, "w");
        if(!replyfile) {
          drawmessagebox("ERROR: Could not create temporary reply file.","Press any key.",1);
          break;
        }

        free(tempstr);
   
        fprintf(replyfile, ">%s\n", firstline->line);
        while(firstline->nextline) {
          firstline = firstline->nextline;
          fprintf(replyfile, ">%s\n", firstline->line);
        }

        fclose(replyfile);

        //Reset firstline to the actual firstline.
        while(firstline->prevline)
          firstline = firstline->prevline;

        //call compose() with REPLY as the compose type. 

        replyto = strdup(displayhdr->from);

        if(strchr(replyto,'<') && strchr(replyto, '>')) {
          replyto = strchr(replyto, '<');
          replyto++;
          *strchr(replyto, '>') = 0;
        } else {
          if(strchr(replyto, ' '))
            *strchr(replyto, ' ') = 0;
          else if(strchr(replyto, '\r'))
            *strchr(replyto, '\r') = 0;
          else
            *strchr(replyto, '\n') = 0;
        }

        if(strncasecmp("re:",displayhdr->subject,3)) {
          replysubject = (char *)malloc(strlen(displayhdr->subject)+strlen("Re: ")+1);
          sprintf(replysubject,"Re: %s", displayhdr->subject);
        } else 
          replysubject = strdup(displayhdr->subject);

        compose(server,NULL, serverpath, strdup(replyto), replysubject, NULL, 0, NULL, 0, NULL, 0, REPLY);
        replied = 1;
 
        thisline = topofview;
        goto cleandisplay;

      break;
    }
  }
  return(replied);  
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

void drawattachedlist(DOMElement * firstdisplay, DOMElement * curattach, int maxnum,int numofattachs) {
  int i;
  DOMElement * temp = firstdisplay;

  con_clrscr();
  colourheaderrow(0);
  colourheaderrow(1);
  colourheaderrow(2);

  con_gotoxy(0,1);
  printf(" Attachments: %d",numofattachs);
  con_gotoxy(0,2);
  for(i=0;i<con_xsize;i++)
    putchar('_');

  colourheaderrow(con_ysize-1);
  con_gotoxy(0,con_ysize-1);
  printf(" (Q)uit to inbox");

  con_setfgbg(listfg_col,listbg_col);
  con_setscroll(3,con_ysize-2);
  
  for(i=0;i<maxnum;i++) {  
    con_gotoxy(0,i+3);
    if(temp == curattach) 
      printf(" > %s", XMLgetAttr(temp, "filename"));
    else
      printf("   %s", XMLgetAttr(temp, "filename"));
    temp = temp->NextElem;
  }

  con_update();
}

void viewattachedlist(char * serverpath, DOMElement * message) {
  DOMElement * attachment, * firstattach, *currentattach, *firstdisplayattach;
  int i, maxnum, numofattachs, input, curpos, decodedbyte,refresh;
  char * tempstr, * filename, * originalfilename, * boundary, * fileref;
  FILE * savefile, *incoming;

  attachment  = XMLgetNode(message, "attachment");
  firstattach = firstdisplayattach = currentattach = attachment;

  fileref = XMLgetAttr(message, "fileref");
  maxnum  = con_ysize-5;
  curpos  = 3;

  numofattachs = message->NumElements;
  if(numofattachs < maxnum)
    maxnum = numofattachs;   

  drawattachedlist(firstdisplayattach,currentattach,maxnum,numofattachs);

  input = 0;
  while(input != 'Q' && input != CURL) {
    input = con_getkey();

    refresh = 1;

    switch(input) {
      case CURD:
        if(!currentattach->NextElem->FirstElem) {
          currentattach = currentattach->NextElem;
          if(curpos == con_ysize-3) {
            firstdisplayattach = firstdisplayattach->NextElem;
            con_gotoxy(1,con_ysize-2);
            putchar('\n');
            con_gotoxy(1,con_ysize-4);
            putchar(' ');
            con_gotoxy(0,con_ysize-3);
            printf(" > %s", XMLgetAttr(currentattach, "filename"));
            con_gotoxy(2,con_ysize-3);
          } else {
            con_gotoxy(1,curpos);
            putchar(' ');
            curpos++;
            con_gotoxy(1,curpos);
            putchar('>');
          }
        }
        refresh = 0;
      break;
      case CURU:
        if(!currentattach->FirstElem) {
          currentattach = currentattach->PrevElem;
          if(curpos == 3) {
            firstdisplayattach = firstdisplayattach->PrevElem;
            con_gotoxy(0,3);
            printf("\x1b[1L");
            con_gotoxy(0,3);
            printf(" > %s", XMLgetAttr(currentattach, "filename"));
            con_gotoxy(1,curpos+1);
            putchar(' ');
            con_gotoxy(2,3);
          } else {
            con_gotoxy(1,curpos);
            putchar(' ');
            curpos--;
            con_gotoxy(1,curpos);
            putchar('>');
          }
        }
        refresh = 0;
      break;

      case '\n':
      case '\r':
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
              filename = getmyline(strdup(""),16,20,15,0);
            break;
          }
        }
        tempstr = (char *)malloc(strlen(serverpath)+strlen(fileref)+2);        
        sprintf(tempstr, "%s%s", serverpath, fileref);
        msgfile = fopen(tempstr, "r");
        free(tempstr);

	//Initialize Boundary to NULL, 
        boundary = NULL;

        drawmessagebox("Searching message for file attachment...","",0);

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

                  drawmessagebox("Decoding Base64 encoded attachment.","Saving file...",0);

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
    if(refresh)
      drawattachedlist(firstdisplayattach,currentattach,maxnum,numofattachs);
    else
      con_update();
  }
  con_setscroll(0,con_ysize-1);
}

