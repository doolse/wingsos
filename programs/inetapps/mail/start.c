//Mail V2.x for Wings
#include "mailheader.h"

char * VERSION = "2.5";

extern DOMElement * configxml; //the root element of the config xml element.
extern soundsprofile * soundsettings;

extern int logofg_col,     logobg_col,     serverselectfg_col, serverselectbg_col;
extern int listfg_col,     listbg_col,     listheadfg_col,     listheadbg_col;
extern int listmenufg_col, listmenubg_col, messagefg_col,      messagebg_col;

int  size = 0;
char *buf = NULL;

char *server;           // Server name as text
FILE *fp;               // Main Server connection.
int abookfd;            // addressbook filedescriptor
namelist *abook = NULL; // ptr to array of AddressBook info
char *abookbuf  = NULL; // ptr to Raw AddressBook data buffer

char *scrollup = "\x1b[1L"; //scroll up esc sequence. should be in console.h

//single linked list of all mailwatches active
activemailwatch * headmailwatch = NULL; 
struct wmutex exclservercon = {-1, -1}; //exclusive server connection

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

void strtolower(char * str) {
  int i;
  for(i=0;i<strlen(str);i++)
    if(str[i] > 64 && str[i] < 91)
      str[i] = str[i] + 32;
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

void changesortorder(mailboxobj * box,int sortnum) {
  if(sortnum == 0)
    box->sortorder = ORD_DATE;
  else if(sortnum == 1)
    box->sortorder = ORD_STATUS;
  else if(sortnum == 2) {
    if(!strcasecmp(box->columns[0],"from"))
      box->sortorder = ORD_FROM;
    if(!strcasecmp(box->columns[0],"to"))
      box->sortorder = ORD_TO;
    if(!strcasecmp(box->columns[0],"subject"))
      box->sortorder = ORD_SUBJECT;
  } else if(sortnum == 3) {
    if(!strcasecmp(box->columns[1],"subject"))
      box->sortorder = ORD_SUBJECT;
    if(!strcasecmp(box->columns[1],"from"))
      box->sortorder = ORD_FROM;
    if(!strcasecmp(box->columns[1],"to"))
      box->sortorder = ORD_TO;
  }
}

int cmp(msgorderlist *a, msgorderlist *b, int ordertype) {

  //fileref number order is the order they were received in.
  if(ordertype == ORD_DATE)
    return(atoi(XMLgetAttr(a->message,"fileref")) - atoi(XMLgetAttr(b->message,"fileref")));

  else if(ordertype == ORD_STATUS)
    return(strcasecmp(XMLgetAttr(a->message,"status"),XMLgetAttr(b->message,"status")));

  else if(ordertype == ORD_FROM)
    return(strcasecmp(XMLgetAttr(a->message,"from"),XMLgetAttr(b->message,"from")));

  else if(ordertype == ORD_TO)
    return(strcasecmp(XMLgetAttr(a->message,"to"),XMLgetAttr(b->message,"to")));

  else if(ordertype == ORD_SUBJECT)
    return(strcasecmp(XMLgetAttr(a->message,"subject"),XMLgetAttr(b->message,"subject")));

  return(0);
}

msgorderlist *listsort(msgorderlist *list, int ordertype) {
    msgorderlist *p, *q, *e, *tail, *oldhead;
    int insize, nmerges, psize, qsize, i;

    /*
     * Silly special case: if `list' was passed in as NULL, return
     * NULL immediately.
     */
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
		    e = q; q = q->next; qsize--;
		    if (q == oldhead) 
			q = NULL;
		} else if (qsize == 0 || !q) {
		    /* q is empty; e must come from p. */
		    e = p; p = p->next; psize--;
		    if (p == oldhead) 
			p = NULL;
		} else if (cmp(p,q,ordertype) <= 0) {
		    /* First element of p is lower (or same);
		     * e must come from p. */
		    e = p; p = p->next; psize--;
		    if (p == oldhead) 
			p = NULL;
		} else {
		    /* First element of q is lower; e must come from q. */
		    e = q; q = q->next; qsize--;
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

void composescreendraw(mailboxobj * thisbox, char * to, char * subject, msgline * cc, int cccount, msgline * bcc, int bcccount, msgline * attach, int attachcount, int typeofcompose) {
  int i, bodylines;
  int row = 0;  
  char * ptr, * headerstr;
  msgline * ccptr, *bccptr, *attachptr;

  switch(typeofcompose) {
    case COMPOSENEW:
      headerstr = strdup("COMPOSE NEW");
    break;
    case REPLY:
      headerstr = strdup("REPLY");
    break;
    case REPLYCONTINUED:
      headerstr = strdup("REPLY CONT");
    break;
    case COMPOSECONTINUED:
      headerstr = strdup("COMPOSE CONT");
    break;
    case RESEND:
      headerstr = strdup("RESEND");
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
  printf(" I'm finished composing... (Q)uit to %s, (a)ddressbook", thisbox->thisboxname);

  free(headerstr);

  con_setfgbg(listfg_col,listbg_col);
}

void freemsgpreview(msgline * msglinenode) {

  while(msglinenode)
    msglinenode = remQueue(msglinenode,msglinenode);
}

void freeccbccattachs(msgline * curcc,msgline* curbcc,msgline * curattach) {
  msgline * temp;

  while(curcc) {
    temp = curcc;
    curcc = remQueue(curcc,curcc);
    free(temp->line);
    free(temp);
  }

  while(curbcc) {
    temp = curbcc;
    curbcc = remQueue(curbcc,curbcc);
    free(temp->line);
    free(temp);
  }

  while(curattach) {
    temp = curattach;
    curattach = remQueue(curattach,curattach);
    free(temp->line);
    free(temp);
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
    ccstring = malloc(tempstrlen+1);

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
    bccstring = malloc(tempstrlen+1);

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
    attachstring = malloc(tempstrlen+1);

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

  //drawmessagebox("messagefile:",messagefile,1);

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

void savetosent(mailboxobj * thisbox, char * to, char * subject, msgline * firstcc, int cccount, msgline * firstbcc, int bcccount, msgline * firstattach, int attachcount, int typeofcompose) {
  char * indexfilepath = NULL;
  char * tempfilestr, * destfilestr, *tempstr;
  DOMElement * activeelemptr, * tempelemptr, * sentxml;
  int tempint;
  msgline * msglineptr;

  if(strcmp(thisbox->path,thisbox->sentpath)) { 
    indexfilepath = malloc(strlen(thisbox->sentpath) + strlen("index.xml") +1);
    sprintf(indexfilepath, "%sindex.xml", thisbox->sentpath);
    sentxml = XMLloadFile(indexfilepath);
  } else
    sentxml = thisbox->index;

  activeelemptr = XMLgetNode(sentxml, "xml/messages");

  tempint = atoi(XMLgetAttr(activeelemptr, "refnum")) +1;

  destfilestr = malloc(strlen(thisbox->sentpath) + 17);
  tempfilestr = malloc(strlen(thisbox->draftspath) + 17);

  sprintf(destfilestr, "%s%d", thisbox->sentpath, tempint);
  sprintf(tempfilestr, "%stemporary.txt", thisbox->draftspath);
            
  spawnlp(S_WAIT,"mv", "-f", tempfilestr, destfilestr, NULL);            
    
  free(tempfilestr);
  free(destfilestr);

  tempstr = malloc(17);
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

  XMLsetAttr(tempelemptr, "status", "S");
        
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

void savetodrafts(mailboxobj * thisbox, char * to, char * subject, msgline * firstcc, int cccount, msgline * firstbcc, int bcccount, msgline * firstattach, int attachcount, int typeofcompose) {
  char * indexfilepath = NULL;
  char * tempfilestr, * destfilestr, *tempstr;
  DOMElement * activeelemptr, * tempelemptr, * draftxml;
  int tempint;
  msgline * msglineptr;

  if(strcmp(thisbox->path,thisbox->draftspath)) {
    indexfilepath = malloc(strlen(thisbox->draftspath) + strlen("index.xml") +1);
    sprintf(indexfilepath, "%sindex.xml", thisbox->draftspath);
    draftxml = XMLloadFile(indexfilepath);
  } else
    draftxml = thisbox->index;

  activeelemptr = XMLgetNode(draftxml, "xml/messages");

  tempint = atoi(XMLgetAttr(activeelemptr, "refnum")) +1;

  tempfilestr = malloc(strlen(thisbox->draftspath) + strlen("temporary.txt")+1);
  destfilestr = malloc(strlen(thisbox->draftspath) + 20);

  sprintf(tempfilestr, "%stemporary.txt",thisbox->draftspath);
  sprintf(destfilestr, "%s%d", thisbox->draftspath, tempint);
            
  spawnlp(S_WAIT,"mv","-f",tempfilestr,destfilestr,NULL);            
    
  free(tempfilestr);
  free(destfilestr);

  tempstr = malloc(17);
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

  /* this will have to be done later when I have more time */

  /*
  indexfilepath = malloc(strlen(thisbox->draftspath) + 17);
  strcpy(indexfilepath,thisbox->draftspath);
  indexfilepath[strlen(indexfilepath) - strlen("drafts/")] = 0;
  indexfilepath = strcat(indexfilepath,"dirs.xml");

  draftxml = XMLloadFile(indexfilepath);
  activeelemptr = XMLgetNode(draftxml, "/xml/directories/directory");
  while(strcmp(XMLgetAttr(activeelemptr,"filename"),"DRAFTS")) 
    activeelemptr = activeelemptr->NextElem;

  tempint = atoi(XMLgetAttr(activeelemptr,"howmany"));
  tempint++;

  tempstr = malloc(10);
  sprintf(tempstr,"%d", tempint);
  XMLsetAttr(activeelemptr,"howmany",tempstr);
  XMLsaveFile(draftxml,indexfilepath);

  free(tempstr);
  free(indexfilepath);
  */
}

int compose(mailboxobj * thisbox,char * to, char * subject, msgline * firstcc, int cccount, msgline * firstbcc, int bcccount, msgline * firstattach, int attachcount, int typeofcompose) {
  FILE * incoming;

  msgline * headline, * firstline, * lastline, * msglineptr;
  msgline * curcc     = firstcc;
  msgline * curbcc    = firstbcc;
  msgline * curattach = firstattach;

  char * tempfilestr, * templine;
  int tempint, refresh;

  int arrowxpos, arrowypos, upperscrollrow, input;
  int bonuslines = 0;
  int section = 0;

  tempfilestr = malloc(strlen(thisbox->draftspath) + strlen("temporary.txt") + 1);
  sprintf(tempfilestr,"%stemporary.txt", thisbox->draftspath);

  headline = firstline = buildmsgpreview(tempfilestr);

  composescreendraw(thisbox,to,subject,firstcc,cccount,firstbcc,bcccount,firstattach,attachcount,typeofcompose);
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
            if(!curcc || curcc->nextline == firstcc)
              section++;
            else
              curcc = curcc->nextline;
          break;
          case BCC:
            movechardown(arrowxpos,arrowypos, '>');
            arrowypos++;
            if(!curbcc || curbcc->nextline == firstbcc)
              section++;
            else
              curbcc = curbcc->nextline;
          break;
          case ATTACH:
            if(!curattach || curattach->nextline == firstattach) {
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
            if(!curcc || curcc == firstcc)
              section--;
            else
              curcc = curcc->prevline;
          break;
          case BCC:
            movecharup(arrowxpos,arrowypos, '>');
            arrowypos--;
            if(!curbcc || curbcc == firstbcc)
              section--;
            else
              curbcc = curbcc->prevline;
          break;
          case ATTACH:
            movecharup(arrowxpos,arrowypos, '>');
            arrowypos--;
            if(!curattach || curattach == firstattach)
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
              msglineptr = malloc(sizeof(msgline));

              if(curcc != NULL) {
                arrowypos++;
                bonuslines++;
                firstcc = addQueueB(firstcc,curcc->nextline,msglineptr);
              } else {
                firstcc = addQueueB(firstcc,firstcc,msglineptr);
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

              if(curbcc != NULL) {
                arrowypos++;
                bonuslines++;
                firstbcc = addQueue(firstbcc,curbcc,msglineptr);
              } else {
                firstbcc = addQueueB(firstbcc,firstbcc,msglineptr);
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
              msglineptr = malloc(sizeof(msgline));
              firstcc = addQueueB(firstcc,firstcc,msglineptr);

              curcc = msglineptr;              
              drawmessagebox("Edit new CC:","                                ",0);
              curcc->line = getmylinerestrict(NULL,64,32,24,13,"",0);
              cccount++;
            }
          break;
          case BCC:
            if(curbcc != NULL) {
              drawmessagebox("Edit this BCC:","                                ",0);
              curbcc->line = getmylinerestrict(curbcc->line,64,32,24,13,"",0);
            } else {
              msglineptr = malloc(sizeof(msgline));
              firstbcc = addQueueB(firstbcc,firstbcc,msglineptr);

              curbcc = msglineptr;              
              drawmessagebox("Edit new BCC:","                                ",0);
              curbcc->line = getmylinerestrict(NULL,64,32,24,13,"",0);
              bcccount++;
            }
          break;
          case ATTACH:
            if(bonuslines > 12 && curattach != NULL)
              break;
            msglineptr = malloc(sizeof(msgline));

            if(curattach != NULL) {
              bonuslines++;
              arrowypos++;
              firstattach = addQueueB(firstattach,curattach->nextline,msglineptr);
            } else {  
              firstattach = addQueueB(firstattach,firstattach,msglineptr);
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
            if(curcc == NULL)
              break;

            if(curcc == firstcc && curcc->nextline == firstcc) {
              msglineptr = curcc;
              curcc = NULL;
              firstcc = NULL;
              //Arrowposition does not move. 
            } else if(curcc != firstcc && curcc->nextline == firstcc) {
              msglineptr = curcc;
              curcc = msglineptr->prevline;
              firstcc = remQueue(firstcc,msglineptr);
              //arrowposition moves up one row.
              arrowypos--;
            } else if(curcc != firstcc && curcc->nextline != firstcc) {
              msglineptr = curcc;
              curcc = curcc->nextline;
              firstcc = remQueue(firstcc,msglineptr);
              //arrowposition does not move.
            } else if(curcc == firstcc && curcc->nextline != firstcc) {
              msglineptr = curcc;
              curcc = curcc->nextline;
              firstcc = remQueue(firstcc,msglineptr);
              //arrowposition does not move.
            }

            free(msglineptr->line);
            free(msglineptr);
           
            if(curcc)
              bonuslines--;
            cccount--;
          break;
          case BCC:
            if(curbcc == NULL)
              break;

            if(curbcc == firstbcc && curbcc->nextline == firstbcc) {
              msglineptr = curbcc;
              curbcc = NULL;
              firstbcc = NULL;
              //Arrowposition does not move. 
            } else if(curbcc != firstbcc && curbcc->nextline == firstbcc) {
              msglineptr = curbcc;
              curbcc = msglineptr->prevline;
              firstbcc = remQueue(firstbcc,msglineptr);
              //arrowposition moves up one row.
              arrowypos--;
            } else if(curbcc != firstbcc && curbcc->nextline != firstbcc) {
              msglineptr = curbcc;
              curbcc = curbcc->nextline;
              firstbcc = remQueue(firstbcc,msglineptr);
              //arrowposition does not move.
            } else if(curbcc == firstbcc && curbcc->nextline != firstbcc) {
              msglineptr = curbcc;
              curbcc = curbcc->nextline;
              firstbcc = remQueue(firstbcc,msglineptr);
              //arrowposition does not move.
            }

            free(msglineptr->line);
            free(msglineptr);
           
            if(curbcc)
              bonuslines--;
            bcccount--;
          break;

          case ATTACH:
            if(curattach == NULL)
              break;

            if(curattach == firstattach && curattach->nextline == firstattach) {
              msglineptr = curattach;
              curattach = NULL;
              firstattach = NULL;
              //Arrowposition does not move. 
            } else if(curattach != firstattach && curattach->nextline == firstattach) {
              msglineptr = curattach;
              curattach = msglineptr->prevline;
              firstattach = remQueue(firstattach,msglineptr);
              //arrowposition moves up one row.
              arrowypos--;
            } else if(curattach != firstattach && curattach->nextline != firstattach) {
              msglineptr = curattach;
              curattach = curattach->nextline;
              firstattach = remQueue(firstattach,msglineptr);
              //arrowposition does not move.
            } else if(curattach == firstattach && curattach->nextline != firstattach) {
              msglineptr = curattach;
              curattach = curattach->nextline;
              firstattach = remQueue(firstattach,msglineptr);
              //arrowposition does not move.
            }

            free(msglineptr->line);
            free(msglineptr);
           
            if(curattach)
              bonuslines--;
            attachcount--;
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
 
      composescreendraw(thisbox, to, subject, firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, typeofcompose);
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
        sendmail(firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, to, subject, tempfilestr, thisbox->aprofile->fromname, thisbox->aprofile->returnaddress);
        savetosent(thisbox, to, subject, firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, typeofcompose);
      break;
      case 's':
        savetodrafts(thisbox, to, subject, firstcc, cccount, firstbcc, bcccount, firstattach, attachcount, typeofcompose);
      break;
      case 'A':
        //just delete the file (thisbox->drafts)temporary.txt
        remove(tempfilestr);
      break;
    }
  } 

  freeccbccattachs(curcc,curbcc,curattach);
  free(tempfilestr);

  if(input == 'd' || input == 's')
    return(1);
  return(0);
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

  msgstr = malloc(strlen("Add '' to your address book?")+strlen(ptr)+1);
  sprintf(msgstr,"Add '%s' to your addressbook?", ptr);

  drawmessagebox(msgstr,"  (y)es, or (n)o",0);
  input = 0;
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

void drawmsglistboxheader(mailboxobj * thisbox) {
  int i,j;
  char * tempstr;

  colourheaderrow(0);
  con_gotoxy(0,0);

  if(thisbox->howmanymessages != 1)
    printf(" Mail v%s for WiNGs    %s    (%d Messages Total)          Copyright 2004", VERSION, thisbox->thisboxname, thisbox->howmanymessages);
  else
    printf(" Mail v%s for WiNGs    %s    (%d Message)                 Copyright 2004", VERSION, thisbox->thisboxname, thisbox->howmanymessages);

  colourheaderrow(1);
  con_gotoxy(0,1);

  printf("[ S ]");

  //calculate the number of underscores on either side of the column title
  j = thisbox->columnwidths[0] - (5 + strlen(thisbox->columns[0]));
  i = j/2;

  tempstr = malloc(con_xsize+1);
  memset(tempstr,'_',con_xsize);
  tempstr[i] = 0;

  printf("%s[ %s ]%s",tempstr,thisbox->columns[0],tempstr);
  if(i * 2 != j)
    putchar('_');

  //spacer between columns
  putchar(' ');

  //calculate the number of underscores on either side of the column title
  j = thisbox->columnwidths[1] - (5 + strlen(thisbox->columns[1]));
  i = j/2;

  memset(tempstr,'_',con_xsize);
  tempstr[i] = 0;

  printf("%s[ %s ]%s",tempstr,thisbox->columns[1],tempstr);
  if(i * 2 != j)
    putchar('_');

  printf("[ A ]");

  free(tempstr);

  con_setfgbg(listfg_col,listbg_col);
}

void drawmsglistboxmenu(mailboxobj * thisbox) {
  colourheaderrow(con_ysize-1);
  con_gotoxy(0,con_ysize-1);

  if(!strcasecmp(thisbox->thisboxname,"INBOX")) {
    printf(" (Q)uit, (N)ew Mail, (c)ompose, (a)ttached, (o)ther boxes");
    if(abookfd != EOF)
      printf(", (t)ake address");
  } else
    printf(" (Q)uit to %s, (c)ompose, (a)ttached, (o)ther boxes", thisbox->parent);

  con_setfgbg(listfg_col,listbg_col);
}

void drawlistline(int row, msgorderlist * drawnode, mailboxobj * thisbox, char * col0, char * col1) {
  char * tmpstr = calloc(con_xsize,1);
  int attaches;

  con_gotoxy(2,row);

  if(XMLfindAttr(drawnode->message, "delete"))
    putchar('D');

  else if(XMLfindAttr(drawnode->message, "flag"))
    putchar('*');

  else
    printf("%s",XMLgetAttr(drawnode->message, "status"));

  strncpy(tmpstr,XMLgetAttr(drawnode->message,col0),thisbox->columnwidths[0]);
  con_gotoxy(4,row);
  printf("%s", tmpstr);

  memset(tmpstr,0,con_xsize);

  strncpy(tmpstr,XMLgetAttr(drawnode->message,col1),thisbox->columnwidths[1]);
  con_gotoxy(5+thisbox->columnwidths[0],row);
  printf("%s", tmpstr);        

  free(tmpstr);

  attaches = atoi(XMLgetAttr(drawnode->message, "attachments"));
  if(attaches != 0) {
    con_gotoxy(5+thisbox->columnwidths[0]+thisbox->columnwidths[1]+1, row);
    printf("%d", attaches);
  }
}

void redrawlist(mailboxobj * thisbox) {
  char * col0, * col1;
  msgorderlist * drawnode;
  int i;

  //Clear screen and draw header and footer
  con_setfgbg(listfg_col,listbg_col);
  con_clrscr();

  drawmsglistboxheader(thisbox);
  drawmsglistboxmenu(thisbox);

  //move drawnode to the top of the screen for this section of the list.
  drawnode = thisbox->currentmsg;
  for(i=thisbox->cursoroffset;i>0;i--) {
    if(drawnode == thisbox->headmsg) {
      thisbox->cursoroffset -= i;
      break;
    }
    drawnode = drawnode->prev;
  }

  col0 = strdup(thisbox->columns[0]);
  col1 = strdup(thisbox->columns[1]);
  strtolower(col0);
  strtolower(col1);

  for(i=thisbox->toprow;i<thisbox->toprow+thisbox->numofrows+1;i++) {
    drawlistline(i, drawnode, thisbox, col0, col1);
    if(drawnode->next == thisbox->headmsg)
      break;
    drawnode = drawnode->next;
  }

  con_setscroll(thisbox->toprow,thisbox->toprow+thisbox->numofrows+1);
  con_gotoxy(0,thisbox->toprow+thisbox->cursoroffset);
  putchar('>');

  con_update();
}

void freeoldmsglist(mailboxobj * thisbox) {
  msgorderlist * tempnode;

  while(thisbox->headmsg) {
    tempnode = thisbox->headmsg;
    thisbox->headmsg = remQueue(thisbox->headmsg,thisbox->headmsg);
    free(tempnode);
  }
}

void buildmsglist(mailboxobj * thisbox) {

  freeoldmsglist(thisbox);
  
  if(!thisbox->howmanymessages) {
    thisbox->message = XMLnewNode(NodeType_Element, "message", "");
    XMLsetAttr(thisbox->message, "from", "");
    XMLsetAttr(thisbox->message, "to", "");
    XMLsetAttr(thisbox->message, "status", " ");
    XMLsetAttr(thisbox->message, "attachments", "");
    XMLsetAttr(thisbox->message, "fileref", "");
    XMLsetAttr(thisbox->message, "subject", "Box is currently empty.");
    thisbox->message->FirstElem = 1;
    thisbox->message->PrevElem  = thisbox->message;
    thisbox->message->NextElem  = thisbox->message;
  }

  //Build a sortable list which points to the message domelements

  thisbox->currentmsg = malloc(sizeof(msgorderlist));
  thisbox->currentmsg->message = thisbox->message;

  thisbox->headmsg = addQueue(thisbox->headmsg,NULL,thisbox->currentmsg);

  while(1) {
    thisbox->message = thisbox->message->NextElem;

    if(thisbox->message->FirstElem)
      break;

    thisbox->currentmsg = malloc(sizeof(msgorderlist));
    thisbox->currentmsg->message = thisbox->message;
    thisbox->headmsg = addQueueB(thisbox->headmsg,thisbox->headmsg,thisbox->currentmsg);    
  }

  thisbox->headmsg = listsort(thisbox->headmsg,thisbox->sortorder);  

  //find position of message last left on.
  thisbox->currentmsg = thisbox->headmsg;
  while(atoi(XMLgetAttr(thisbox->currentmsg->message,"fileref")) != thisbox->lastmsgpos) {
    thisbox->currentmsg = thisbox->currentmsg->next;
    if(thisbox->currentmsg == thisbox->headmsg)
      break;
  }
  thisbox->cursoroffset = thisbox->toprow + (thisbox->numofrows/2);
}

void openmessage(mailboxobj * thisbox) {

  char *tempstr, *to, *subject;
  int bcccount, cccount, attachcount, tempint;
  DOMElement *vop;
  msgline *hbcc, *bcc, *hcc, *cc, *hattach, *attach, *tempmsgline;
  msgorderlist * tempmol;

  if(!strcmp(XMLgetAttr(thisbox->currentmsg->message, "status"),"N")) {
    thisbox->unread--;
    XMLsetAttr(thisbox->currentmsg->message, "status", " ");
  }         
        
  if(XMLfindAttr(thisbox->currentmsg->message,"from")) {

    //prepview returns a 1 if the message was replied to.
    if(prepforview(atoi(XMLgetAttr(thisbox->currentmsg->message, "fileref")), thisbox))
      XMLsetAttr(thisbox->currentmsg->message, "status", "R");

  } else if(XMLfindAttr(thisbox->currentmsg->message,"to")) {

    /*
    compose(mailboxobj * thisbox,
            char * to, char * subject, 
            msgline * firstcc,     int cccount, 
            msgline * firstbcc,    int bcccount, 
            msgline * firstattach, int attachcount, 
            int typeofcompose)
    */

    if(strpbrk(XMLgetAttr(thisbox->currentmsg->message, "status"),"CRS")) {
    
      hcc = hbcc = hattach = NULL;
      cccount = bcccount = attachcount = 0;

      if(thisbox->currentmsg->message->NumElements) {
        vop = thisbox->currentmsg->message->Elements;
        do {
          if(!strcmp(vop->Node.Name,"cc")) {
            cccount++;
            tempmsgline = malloc(sizeof(msgline));
            tempmsgline->line = strdup(XMLgetAttr(vop,"address"));
            hcc = addQueueB(hcc,hcc,tempmsgline);
          } else if(!strcmp(vop->Node.Name,"bcc")) {
            bcccount++;
            tempmsgline = malloc(sizeof(msgline));
            tempmsgline->line = strdup(XMLgetAttr(vop,"address"));
            hbcc = addQueueB(hbcc,hbcc,tempmsgline);
          } else { 
            attachcount++;
            tempmsgline = malloc(sizeof(msgline));
            tempmsgline->line = strdup(XMLgetAttr(vop,"file"));
            hattach = addQueueB(hattach,hattach,tempmsgline);
          }
          vop = vop->NextElem;
        } while(!vop->FirstElem);
      }

      to      = strdup(XMLgetAttr(thisbox->currentmsg->message,"to"));
      subject = strdup(XMLgetAttr(thisbox->currentmsg->message,"subject"));
	
      tempstr = XMLgetAttr(thisbox->currentmsg->message,"status");
      if(tempstr[0] == 'C' || tempstr[0] == 'R') {
        if(tempstr[0] == 'C')
          tempint = COMPOSECONTINUED;
        else
          tempint = REPLYCONTINUED;

        tempstr = malloc(strlen(thisbox->path)+strlen(thisbox->draftspath)+strlen("mv -f   temporary.txt")+17);
        sprintf(tempstr,"mv -f %s%s %stemporary.txt",thisbox->path,XMLgetAttr(thisbox->currentmsg->message,"fileref"),thisbox->draftspath);
        system(tempstr);
        free(tempstr);

        XMLremNode(thisbox->currentmsg->message);

        if(thisbox->currentmsg->next != thisbox->headmsg)
          tempmol = thisbox->currentmsg->next;
        else
          tempmol = thisbox->currentmsg->next;

        thisbox->headmsg = remQueue(thisbox->headmsg,thisbox->currentmsg);
        thisbox->currentmsg = tempmol;
        thisbox->howmanymessages--;
      } else {
        tempint = RESEND;

        tempstr = malloc(strlen(thisbox->path)+strlen(thisbox->draftspath)+strlen("cp -f   temporary.txt")+17);
        sprintf(tempstr,"cp -f %s%s %stemporary.txt",thisbox->path,XMLgetAttr(thisbox->currentmsg->message,"fileref"),thisbox->draftspath);
        system(tempstr);
        free(tempstr);
      }

      compose(thisbox,to,subject,hcc,cccount,hbcc,bcccount,hattach,attachcount,tempint);
      buildmsglist(thisbox);
    }
  }
}

int drawdirectory(DOMElement * dirs, int parent) {
  int i,y=3,x;

  x = (con_xsize - 21)/2;
  i = 15;

  //con_setfgbg(HDRFGCOL,HDRBGCOL);

  con_gotoxy(x,y++);
  printf(" ___________________ ");
  if(parent) {
    con_gotoxy(x,y++);
    printf(" | Parent Mailbox  | ");
  }

  do {
    con_gotoxy(x,y);
    i--;
    printf(" |                 | ");
    con_gotoxy(x+3,y++);
    printf("%s",XMLgetAttr(dirs,"filename"));
    dirs = dirs->NextElem;
  } while(!dirs->FirstElem);

  while(i) {
    con_gotoxy(x,y++);
    i--;
    printf(" |                 | ");
  }

  if(!parent) {
    con_gotoxy(x,y++);
    printf(" |                 | ");
  }
  con_gotoxy(x,y);
  printf(" |_________________| ");

  return(x+2);
}

void movemessagesnow(mailboxobj * frombox, char * topath) {
  DOMElement * toindex, * messages, * newmessage, * fsubnode, *tsubnode;
  DOMElement * frommsg;
  char * fromfileref, * tofileref, * tempstr;
  int i;

  tempstr = malloc(strlen("mv '' ''") + +strlen(frombox->path) + 17 + strlen(topath) + 17);
  sprintf(tempstr,"%sindex.xml",topath);
  toindex = XMLloadFile(tempstr);
  
  tofileref = malloc(10);  

  messages = XMLgetNode(toindex,"/xml/messages");
  i = atoi(XMLgetAttr(messages,"refnum"));
  
  frommsg = XMLgetNode(frombox->messages,"message");
  while(1) {
    if(XMLfindAttr(frommsg,"flag")) {
      i++;
      sprintf(tofileref,"%d",i);
      newmessage = XMLnewNode(NodeType_Element,"message","");

      if(XMLfindAttr(frommsg,"to"))
        XMLsetAttr(newmessage,"to",XMLgetAttr(frommsg,"to"));
      else if(XMLfindAttr(frommsg,"from"))
        XMLsetAttr(newmessage,"from",XMLgetAttr(frommsg,"from"));

      XMLsetAttr(newmessage,"subject",XMLgetAttr(frommsg,"subject"));
      XMLsetAttr(newmessage,"status",XMLgetAttr(frommsg,"status"));
      XMLsetAttr(newmessage,"attachments",XMLgetAttr(frommsg,"attachments"));

      if(frommsg->NumElements) {
        fsubnode = frommsg->Elements;
        do {
          if(!strcmp(fsubnode->Node.Name,"cc")) {
            tsubnode = XMLnewNode(NodeType_Element,"cc","");
            XMLsetAttr(tsubnode,"address",XMLgetAttr(fsubnode,"address"));
          } else if(!strcmp(fsubnode->Node.Name,"bcc")) {
            tsubnode = XMLnewNode(NodeType_Element,"bcc","");
            XMLsetAttr(tsubnode,"address",XMLgetAttr(fsubnode,"address"));
          } else if(!strcmp(fsubnode->Node.Name,"attach")) {
            tsubnode = XMLnewNode(NodeType_Element,"attach","");
            XMLsetAttr(tsubnode,"address",XMLgetAttr(fsubnode,"file"));
          } else if(!strcmp(fsubnode->Node.Name,"attachment")) {
            tsubnode = XMLnewNode(NodeType_Element,"attachment","");
            XMLsetAttr(tsubnode,"address",XMLgetAttr(fsubnode,"filename"));
          }
      
          XMLinsert(newmessage,NULL,tsubnode);

          fsubnode = fsubnode->NextElem;
        } while(!fsubnode->FirstElem);
      }

      XMLsetAttr(newmessage,"fileref",tofileref);

      sprintf(tempstr,"mv \"%s%s\" \"%s%d\"",frombox->path,XMLgetAttr(frommsg,"fileref"),topath,i);
      system(tempstr);
 
      XMLinsert(messages,NULL,newmessage);
    
      frommsg = frommsg->PrevElem;
      XMLremNode(frommsg->NextElem);
    } else
      frommsg = frommsg->PrevElem;

    if(!frombox->messages->NumElements)
      break;
    if(frommsg->FirstElem && !XMLfindAttr(frommsg,"flag"))
      break;
  }

  XMLsetAttr(messages,"refnum",tofileref);
  frombox->howmanymessages = frombox->messages->NumElements;


  sprintf(tempstr,"%sindex.xml",topath);
  XMLsaveFile(toindex,tempstr);

  sprintf(tempstr,"%sindex.xml",frombox->path);
  XMLsaveFile(frombox->index,tempstr);

  system("sync");
}

int movemessages(mailboxobj * frombox, char * path) {
  DOMElement * dirs,* firstdir, * curdir = NULL;
  char * tempstr;
  int input,curpos,x,parent;

  tempstr = malloc(strlen(path) + 17 + 17);
  sprintf(tempstr,"%sdirs.xml",path);
  dirs = XMLloadFile(tempstr);

  if(!strcmp(frombox->inboxpath,path))
    parent = 0;
  else
    parent = 1;

  firstdir = XMLgetNode(dirs,"/xml/directories/directory");
  if(parent)
    curdir = NULL;
  else
    curdir = firstdir;

  curpos = 4;
  x = drawdirectory(firstdir,parent);
  con_gotoxy(x,curpos);
  putchar('>');
  con_update();

  while(1) {
    input = con_getkey();
    switch(input) {
      case CURD:
        if(curdir && !curdir->NextElem->FirstElem) {
          curdir = curdir->NextElem;
          movechardown(x,curpos++,'>');
        } else if(!curdir && firstdir) {
          curdir = firstdir;
          movechardown(x,curpos++,'>');
        }
      break;  
      case CURU:
        if(curdir && curdir != firstdir) {
          curdir = curdir->PrevElem;
          movecharup(x,curpos--,'>');
        } else if(curdir && parent) {
          curdir = NULL;
          movecharup(x,curpos--,'>');
        }
      break;
      case '\n':
      case '\r':
        if(!curdir) {
          sprintf(tempstr,"%s",path);
          for(x=strlen(path)-2;x>(strlen(path)-18);x--) {
            if(tempstr[x] == '/') {
              tempstr[x+1] = 0;
              break;
            }
          }
        } else
          sprintf(tempstr,"%s%s/",path,XMLgetAttr(curdir,"filename"));

        XMLremNode(XMLgetNode(dirs,"/xml"));
        input = movemessages(frombox,tempstr);
        free(tempstr);
        return(input);
      break;
      case ESC:
        XMLremNode(XMLgetNode(dirs,"/xml"));
        free(tempstr);
        return(0);
      break;
      case ' ':
        if(!strcmp(frombox->path,path) && !curdir) {
          XMLremNode(XMLgetNode(dirs,"/xml"));
          free(tempstr);
          return(0);
        }

        drawmessagebox("Moving messages between mailboxes...","",0);

        if(curdir) {
          sprintf(tempstr,"%s%s/",path,XMLgetAttr(curdir,"filename"));
          movemessagesnow(frombox,tempstr);
        } else 
          movemessagesnow(frombox,path);

        XMLremNode(XMLgetNode(dirs,"/xml"));
        free(tempstr);
        return(1);
      break;
    }
  }
  return(0);
}

void openmailbox(mailboxobj * thisbox) {
  int input, newmessages = 0, tempint;
  char * tempstr, * col0, * col1;
  dataset * returndataset;
  msgboxobj * newmailmsgbox;

  tempstr = malloc(strlen(thisbox->path)+strlen("index.xml")+1);
  sprintf(tempstr,"%sindex.xml",thisbox->path);

  thisbox->index = XMLloadFile(tempstr);

  free(tempstr);
  if(!thisbox->parent)
    getsubdirs(thisbox);

  thisbox->messages = XMLgetNode(thisbox->index, "xml/messages");
  thisbox->message  = XMLgetNode(thisbox->messages, "message");
  
  thisbox->howmanymessages = thisbox->messages->NumElements;
  thisbox->sortorder  = atoi(XMLgetAttr(thisbox->messages,"sortorder"));
  thisbox->lastmsgpos = atoi(XMLgetAttr(thisbox->messages,"lastmsgpos"));

  thisbox->toprow = 2;
  thisbox->numofrows = con_ysize - thisbox->toprow - 2;

  col0 = strdup(thisbox->columns[0]);
  col1 = strdup(thisbox->columns[1]);
  strtolower(col0);
  strtolower(col1);

  buildmsglist(thisbox);

  redrawlist(thisbox);

  input=0;
  while(input != 'Q') {
    input = con_getkey();

    forcenextaction:

    switch(input) {
      case '0':
        changesortorder(thisbox,0);
        buildmsglist(thisbox);
        redrawlist(thisbox);
      break;
      case '1':
        changesortorder(thisbox,1);
        buildmsglist(thisbox);
        redrawlist(thisbox);
      break;
      case '2':
        changesortorder(thisbox,2);
        buildmsglist(thisbox);
        redrawlist(thisbox);
      break;
      case '3':
        changesortorder(thisbox,3);
        buildmsglist(thisbox);
        redrawlist(thisbox);
      break;
      case '<':
        if(thisbox->columnwidths[0] > strlen(thisbox->columns[0]) +5) {
          thisbox->columnwidths[0] -= 1;
          thisbox->columnwidths[1] += 1;
          redrawlist(thisbox);
        }
      break;
      case '>':
        if(thisbox->columnwidths[1] > strlen(thisbox->columns[1]) +5) {
          thisbox->columnwidths[0] += 1;
          thisbox->columnwidths[1] -= 1;
          redrawlist(thisbox);
        }
      break;
      case CURD:
        if(thisbox->currentmsg->next != thisbox->headmsg) {
          thisbox->currentmsg = thisbox->currentmsg->next;
          thisbox->lastmsgpos = atoi(XMLgetAttr(thisbox->currentmsg->message,"fileref"));

          if(thisbox->cursoroffset < thisbox->numofrows) {
            movechardown(0,thisbox->toprow+thisbox->cursoroffset,'>');
            thisbox->cursoroffset++;
          } else {
            con_gotoxy(0,thisbox->toprow+thisbox->numofrows);
            putchar('\n');
            con_gotoxy(0,thisbox->toprow+thisbox->cursoroffset-1);
            putchar(' ');
            con_gotoxy(0,thisbox->toprow+thisbox->cursoroffset);
            putchar('>');
            drawlistline(thisbox->toprow+thisbox->cursoroffset,thisbox->currentmsg,thisbox, col0,col1);
            con_gotoxy(1,thisbox->toprow+thisbox->cursoroffset);
            con_update();
          }
        }
      break;
      case CURU:
        if(thisbox->currentmsg->prev != thisbox->headmsg->prev) {
          thisbox->currentmsg = thisbox->currentmsg->prev;
          thisbox->lastmsgpos = atoi(XMLgetAttr(thisbox->currentmsg->message,"fileref"));

          if(thisbox->cursoroffset) {
            movecharup(0, thisbox->toprow+thisbox->cursoroffset, '>');
            thisbox->cursoroffset--;
          } else {
            con_gotoxy(0,thisbox->toprow);
            printf("%s",scrollup);
            con_gotoxy(0,thisbox->toprow+thisbox->cursoroffset+1);
            putchar(' ');
            con_gotoxy(0,thisbox->toprow+thisbox->cursoroffset);
            putchar('>');
            drawlistline(thisbox->toprow+thisbox->cursoroffset,thisbox->currentmsg,thisbox,col0,col1);
            con_gotoxy(1,thisbox->toprow+thisbox->cursoroffset);
            con_update();
          }
        }
      break;

      case ' ':
        if(!thisbox->howmanymessages) 
          break;

        if(XMLfindAttr(thisbox->currentmsg->message,"flag")) {
          XMLremNode(XMLfindAttr(thisbox->currentmsg->message,"flag"));
          curright(1);
          printf("%s", XMLgetAttr(thisbox->currentmsg->message, "status"));
          curleft(2);
        } else if(!XMLfindAttr(thisbox->currentmsg->message,"delete")) {
          XMLsetAttr(thisbox->currentmsg->message,"flag","*");
          curright(1);
          putchar('*');
          curleft(2);
        }
        con_update();
        input = CURD;
        goto forcenextaction;
      break;

      case 'm':
        if(movemessages(thisbox,thisbox->path))
          buildmsglist(thisbox);
        redrawlist(thisbox);
      break;

      //cursor right or return will view the message

      case CURR:
      case '\r':
      case '\n':
        if(!thisbox->howmanymessages)
          break;
        openmessage(thisbox);
        redrawlist(thisbox);
      break;

      case 'c':
        //overwrite residual temporary.txt with a new blank file

        tempstr = malloc(strlen(thisbox->draftspath)+strlen("echo >/temporary.txt")+1);
        sprintf(tempstr, "echo >%s/temporary.txt", thisbox->draftspath);
        system(tempstr);
        free(tempstr);

        if(compose(thisbox,NULL,NULL,NULL,0,NULL,0,NULL,0,COMPOSENEW) && (!strcmp(thisbox->path,thisbox->sentpath) || !strcmp(thisbox->path,thisbox->draftspath)))
          buildmsglist(thisbox);
           
        redrawlist(thisbox);
      break;

      case 'o':
        if(thisbox->parent)
          return;
        viewsubdirs(thisbox);
        redrawlist(thisbox);
      break;

      case 18: // C= r  stands for view raw source
        if(!thisbox->howmanymessages)
          break;

        if(!strcmp(XMLgetAttr(thisbox->currentmsg->message, "fileref"), "")) {
          drawmessagebox("Error: The message file doesn't exist","",1);
        } else {
          tempstr = malloc(strlen(thisbox->path)+strlen(XMLgetAttr(thisbox->currentmsg->message,"fileref"))+1);
          sprintf(tempstr, "%s%s", thisbox->path,XMLgetAttr(thisbox->currentmsg->message,"fileref"));
          spawnlp(S_WAIT,"ned",tempstr,NULL);
          free(tempstr);
        }

        redrawlist(thisbox);
      break;

      case 'a':
        if(atoi(XMLgetAttr(thisbox->currentmsg->message, "attachments"))) {
          viewattachedlist(thisbox->path, thisbox->currentmsg->message);
          redrawlist(thisbox);
        }
      break;

      case 't':
        if((!thisbox->howmanymessages) || abookfd == EOF)
          break;
        takeaddress(thisbox->currentmsg->message);
        redrawlist(thisbox);
      break;

      case 'N':
        if(thisbox->parent)
          break;

        returndataset = getnewmsgsinfo(thisbox);
        
        if(returndataset) {
          playsound(NEWMAIL);

          newmailmsgbox = initmsgboxobj(returndataset->string,"","",1,returndataset->number);
          drawmsgboxobj(newmailmsgbox);

          tempint = thisbox->howmanymessages;
          newmessages = getnewmail(thisbox->aprofile, thisbox->messages, thisbox->path, newmailmsgbox, strtoul(XMLgetAttr(thisbox->server, "skipsize"), NULL, 10), atoi(XMLgetAttr(thisbox->server, "deletemsgs")));

          playsound(DOWNLOADDONE);

          if(!tempint)
            thisbox->message = XMLgetNode(thisbox->messages, "message");

          thisbox->unread          += newmessages;
          thisbox->howmanymessages += newmessages;

          free(returndataset->string);
          free(returndataset);

          tempstr = malloc(strlen(thisbox->path)+strlen("index.xml")+1);
          sprintf(tempstr, "%sindex.xml", thisbox->path);
          XMLsaveFile(thisbox->index, tempstr);
          free(tempstr);

          buildmsglist(thisbox);

        } else {
          newmessages = 0;
          playsound(NONEWMAIL);
          drawmessagebox("     No new mail.    ","    Press any key.   ",1);
        }

        redrawlist(thisbox);
      break;

      case DEL:
        if(!thisbox->howmanymessages) 
          break;

        if(XMLfindAttr(thisbox->currentmsg->message, "delete")) {
          XMLremNode(XMLfindAttr(thisbox->currentmsg->message, "delete"));
          curright(1);
          printf("%s", XMLgetAttr(thisbox->currentmsg->message, "status"));
          curleft(2);
        } else if(!XMLfindAttr(thisbox->currentmsg->message,"flag")) {
          XMLsetAttr(thisbox->currentmsg->message, "delete", "true");
          curright(1);
          putchar('D');
          curleft(2);
        }
        con_update();
        input = CURD;
        goto forcenextaction;
      break;
    }    
  }
}

void drawsubdirsheader(char * boxname) {
  int width;  

  con_clrscr();

  colourheaderrow(0);
  con_gotoxy(0,0);
  printf(" Sub-mailboxes of '%s' /",boxname);
  colourheaderrow(1);
  con_gotoxy(0,1);
  
  width = strlen(boxname);
  printf("____________________");
  while(width) {
    putchar('_');
    width--;
  }
  putchar('/');
}

void drawsubdirsmenu(char * boxname) {
  colourheaderrow(con_ysize-1);
  con_gotoxy(0,con_ysize-1);
  printf(" (s)how messages in '%s', (n)ew mailbox", boxname);
}

DOMElement * drawsubdirlist(int offset, mailboxobj * box) {
  DOMElement * headsubdir=NULL, * drawsubdir;
  int drawrow = offset;
  char *temp;

  if(box->parent) {
    con_gotoxy(3,drawrow);
    printf("Move up to %s",box->parent);
    drawrow++;
  }

  if(box->subdirs && box->subdirs->NumElements)  {
    headsubdir = drawsubdir = XMLgetNode(box->subdirs,"directory");
    do {
      con_gotoxy(3,drawrow);
      printf("%16s (%s)",XMLgetAttr(drawsubdir,"filename"),XMLgetAttr(drawsubdir,"howmany"));
      drawsubdir = drawsubdir->NextElem;
      drawrow++;
    } while(drawsubdir != headsubdir);
  }  

  return(headsubdir);
}

void getsubdirs(mailboxobj * thisbox) {
  DOMElement * dirs;
  char * tempstr;

  tempstr = malloc(strlen(thisbox->path) + strlen("dirs.xml") +1);
  sprintf(tempstr,"%sdirs.xml",thisbox->path);
  dirs = XMLloadFile(tempstr);
  free(tempstr);
  thisbox->dirs = dirs;
  thisbox->subdirs = XMLgetNode(dirs,"/xml/directories");
}

void freenewbox(mailboxobj * newbox) {

  free(newbox->path);
  free(newbox->parent);
  free(newbox->thisboxname);
  free(newbox->columns[0]);
  free(newbox->columns[1]);
     
  free(newbox);
}

int redrawsubdirs(mailboxobj * thisbox,int rowoffset, DOMElement **headsubdir, DOMElement **cursubdir) {
  drawsubdirsheader(thisbox->thisboxname);
  drawsubdirsmenu(thisbox->thisboxname);
  con_setfgbg(listfg_col,listbg_col);
  *headsubdir = *cursubdir = drawsubdirlist(rowoffset,thisbox);

  con_gotoxy(1,rowoffset);
  putchar('>');

  if(thisbox->parent)
    *cursubdir = NULL;

  return(rowoffset);
}

void addsubdirectory(mailboxobj * thisbox,char * tempstr) {
  DOMElement * newsubdir;
  char * pathstr;
  int input;
  DIR * dir;
  struct dirent * entry;
  FILE * xmlfile;
  
  dir = opendir(thisbox->path);
  while(entry = readdir(dir)) {
    if(!strcasecmp(entry->d_name,tempstr)) {
      drawmessagebox("A directory entry with that name already exists.","",1);
      closedir(dir);
      return;
    }
  }
  closedir(dir);

  drawmessagebox("New mailbox should have the columns:","1) FROM and SUBJECT, or 2) TO and SUBJECT",0);
  input = 0;
  while(input != '1' && input != '2')
    input = con_getkey();

  drawmessagebox("Creating new mailbox...","",0);

  newsubdir = XMLnewNode(NodeType_Element,"directory","");
  XMLinsert(thisbox->subdirs,NULL,newsubdir);

  XMLsetAttr(newsubdir,"filename",tempstr);
  XMLsetAttr(newsubdir,"howmany","0");
  if(input == '1') {
    XMLsetAttr(newsubdir,"column0","FROM");
    XMLsetAttr(newsubdir,"column1","SUBJECT");
  } else {
    XMLsetAttr(newsubdir,"column0","TO");
    XMLsetAttr(newsubdir,"column1","SUBJECT");
  }
  XMLsetAttr(newsubdir,"col0width","20");
  XMLsetAttr(newsubdir,"col1width","50");
  XMLsetAttr(newsubdir,"unread","0");

  pathstr = malloc(strlen(thisbox->path) + 17 + 16 + 1);
  sprintf(pathstr,"%sdirs.xml",thisbox->path);
  XMLsaveFile(thisbox->dirs,pathstr);

  sprintf(pathstr,"%s%s/",thisbox->path,tempstr);
  mkdir(pathstr,0);

  sprintf(pathstr,"%sindex.xml",pathstr);
  xmlfile = fopen(pathstr,"w");
  fprintf(xmlfile,"<xml>\n <messages refnum=\"0\" lastmsgpos=\"0\" sortorder=\"0\"/>\n</xml>");
  fclose(xmlfile);

  sprintf(pathstr,"%s%s/dirs.xml",thisbox->path,tempstr);
  xmlfile = fopen(pathstr,"w");
  fprintf(xmlfile,"<xml>\n <directories/>\n</xml>");
  fclose(xmlfile);

  free(pathstr);
}

int viewsubdirs(mailboxobj * thisbox) {
  DOMElement * cursubdir, * headsubdir;
  mailboxobj * newbox;
  int rowoffset=3,curpos=0, input, changes=0;
  char * tempstr;

  curpos = redrawsubdirs(thisbox, rowoffset, &headsubdir, &cursubdir);

  input = 0;
  while(1) {
    con_update();
    input = con_getkey();
    switch(input) {
      case CURD:
        if(!headsubdir)
          break;
        if(!cursubdir) {
          cursubdir = headsubdir;
          movechardown(1,curpos,'>');
          curpos++;
        } else if(cursubdir->NextElem != headsubdir) {
          cursubdir = cursubdir->NextElem;
          movechardown(1,curpos,'>');
          curpos++;
        }
      break;
      case CURU:
        if(cursubdir && (thisbox->parent || cursubdir != headsubdir)) {
          if(cursubdir == headsubdir)
            cursubdir = NULL;
          else
            cursubdir = cursubdir->PrevElem;          
          movecharup(1,curpos,'>');
          curpos--;
        }
      break;
      case '\n':
      case '\r':
        if(!cursubdir)
          return(changes);
        newbox = initmailboxobj();
        
        newbox->aprofile = thisbox->aprofile;
        newbox->server   = thisbox->server;
        newbox->path     = malloc(strlen(thisbox->path) + strlen(XMLgetAttr(cursubdir,"filename")) + 2);
        sprintf(newbox->path,"%s%s/",thisbox->path,XMLgetAttr(cursubdir,"filename"));
        
        newbox->parent   = strdup(thisbox->thisboxname);
        newbox->thisboxname = strdup(XMLgetAttr(cursubdir,"filename"));
        newbox->draftspath = thisbox->draftspath;
        newbox->sentpath = thisbox->sentpath;
        newbox->inboxpath = thisbox->inboxpath;
        newbox->unread   = atoi(XMLgetAttr(cursubdir,"unread"));
        newbox->columns[0] = strdup(XMLgetAttr(cursubdir,"column0"));
        newbox->columns[1] = strdup(XMLgetAttr(cursubdir,"column1"));
        newbox->columnwidths[0] = atoi(XMLgetAttr(cursubdir,"col0width"));
        newbox->columnwidths[1] = atoi(XMLgetAttr(cursubdir,"col1width"));

        newbox->cursoroffset = thisbox->toprow;     

        getsubdirs(newbox);

        if(viewsubdirs(newbox)) {
          //changes were made ... save the newbox settings to thisbox
          tempstr = malloc(strlen(thisbox->path) + strlen("dirs.xml") +1);
          sprintf(tempstr,"%d",newbox->howmanymessages);
          XMLsetAttr(cursubdir,"howmany",tempstr);
          sprintf(tempstr,"%d",newbox->unread);
          XMLsetAttr(cursubdir,"unread",tempstr);
          
          sprintf(tempstr,"%d",newbox->columnwidths[0]);
          XMLsetAttr(cursubdir,"col0width",tempstr);
          sprintf(tempstr,"%d",newbox->columnwidths[1]);
          XMLsetAttr(cursubdir,"col1width",tempstr);

          sprintf(tempstr,"%sdirs.xml",thisbox->path);
          XMLsaveFile(thisbox->dirs,tempstr);

          free(tempstr);
        }
        freenewbox(newbox);   

        curpos = redrawsubdirs(thisbox, rowoffset, &headsubdir, &cursubdir);
      break;
      case 's':
        if(thisbox->parent) {
          openmailbox(thisbox);
          closemailbox(thisbox);

          changes = 1;

          curpos = redrawsubdirs(thisbox, rowoffset, &headsubdir, &cursubdir);
        } else {
          // var 'changes' should be irrelevant here because
          // it's just going back to the INBOX... 
          return(changes);
        }
      break;
      case 'n':
        if(thisbox->subdirs->NumElements >= 15) {
          drawmessagebox("Each mailbox may only have 15 sub-mailboxes.","",1);
          curpos = redrawsubdirs(thisbox, rowoffset, &headsubdir, &cursubdir);
          break;
        }

        drawmessagebox("New mailbox name:  "," ",0);
        tempstr = getmyline(NULL,16,30,13,0);

        if(tempstr[0] == 0) {
          curpos = redrawsubdirs(thisbox, rowoffset, &headsubdir, &cursubdir);
          free(tempstr);
          break;
        }

        addsubdirectory(thisbox,tempstr);
        free(tempstr);
        curpos = redrawsubdirs(thisbox, rowoffset, &headsubdir, &cursubdir);
      break;
      case DEL:
        if(XMLfindAttr(cursubdir,"lock")) {
          drawmessagebox("This mailbox cannot be deleted","Press any key",1);
          curpos = redrawsubdirs(thisbox, rowoffset, &headsubdir, &cursubdir);
          break;
        }

        drawmessagebox("All messages and mailboxes inside this one will be lost.","Delete anyway? (Y)es or (n)o",0);
        input = 0;
        while(input != 'Y' && input != 'n')
          input = con_getkey();

        if(input == 'n') {
          curpos = redrawsubdirs(thisbox, rowoffset, &headsubdir, &cursubdir);
          break;
        } else {
          drawmessagebox("Deleting mailbox...","",0);
          tempstr = malloc(strlen("rm -rf ") + strlen(thisbox->path) + 17);
          sprintf(tempstr,"rm -rf %s%s",thisbox->path,XMLgetAttr(cursubdir,"filename"));
          system(tempstr);

          if(cursubdir->NextElem == cursubdir) {
            XMLremNode(cursubdir);
            cursubdir = headsubdir = NULL;
          } else if(cursubdir->NextElem == headsubdir) {
            cursubdir = cursubdir->PrevElem;
            XMLremNode(cursubdir->NextElem);
          } else {
            cursubdir = cursubdir->NextElem;
            XMLremNode(cursubdir->PrevElem);
          }
          sprintf(tempstr,"%sdirs.xml",thisbox->path);
          XMLsaveFile(thisbox->dirs,tempstr);

          free(tempstr);
          curpos = redrawsubdirs(thisbox, rowoffset, &headsubdir, &cursubdir);
        }
      break;
    }
  }

  return(SUCCESS);
}

void closemailbox(mailboxobj * thisbox) {
  DOMElement * delmsg, * lastmsg;
  char * tempstr;
  int i,j;

  //handle expunging
  drawmessagebox("        Cleaning up mail boxes...","Expunging messages marked for deletion...",0);
 
  //lastmsg tracks message user was last at.
  lastmsg = thisbox->currentmsg->message;
  delmsg = lastmsg;

  while(!delmsg->FirstElem)
    delmsg = delmsg->NextElem;

  tempstr = malloc(strlen(thisbox->path)+17);

  thisbox->unread = 0;

  while(1) {
    if(XMLfindAttr(delmsg, "delete")) {

      if(delmsg == lastmsg && delmsg->NextElem->FirstElem)
        lastmsg = lastmsg->PrevElem;
      else if(delmsg == lastmsg)
        lastmsg = lastmsg->NextElem;
                        
      sprintf(tempstr, "%s%s", thisbox->path, XMLgetAttr(delmsg, "fileref"));
      remove(tempstr);

      delmsg = delmsg->PrevElem;
      XMLremNode(delmsg->NextElem);
    } else {
      delmsg = delmsg->PrevElem;
      if(XMLgetAttr(delmsg, "status")[0] == 'N')
        thisbox->unread++;
    }

    if(!thisbox->messages->NumElements)
      break;
    else if(delmsg->FirstElem && !XMLfindAttr(delmsg,"delete"))
      break;
  }

  thisbox->howmanymessages = thisbox->messages->NumElements;

  //store lastmsgpos and sort order
  XMLsetAttr(thisbox->messages, "lastmsgpos", XMLgetAttr(lastmsg,"fileref"));

  sprintf(tempstr,"%d",thisbox->sortorder);
  XMLsetAttr(thisbox->messages,"sortorder",tempstr);

  //save inbox xmlfile
  sprintf(tempstr, "%sindex.xml", thisbox->path);
  XMLsaveFile(thisbox->index,tempstr);

  XMLremNode(XMLgetNode(thisbox->index,"/xml"));

  freeoldmsglist(thisbox);
  free(tempstr);
}

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

mailboxobj * initmailboxobj() {
  mailboxobj * newbox;

  newbox = malloc(sizeof(mailboxobj));
  newbox->path = NULL;
  newbox->parent = NULL;
  newbox->thisboxname = NULL;
  
  newbox->sortorder = ORD_DATE;
  
  newbox->headmsg = NULL;
  newbox->currentmsg = NULL;

  newbox->aprofile = NULL;
  newbox->columns[0] = NULL;
  newbox->columns[1] = NULL;

  return(newbox);
}

void prepinboxforopen(DOMElement *cserver) {
  char * tempstr;
  mailboxobj * thisbox;

  //compose the inbox mailboxobject... 
  thisbox = initmailboxobj();

  //Setup the accountprofile 
  thisbox->aprofile = malloc(sizeof(accountprofile));
  thisbox->aprofile->username = strdup(XMLgetAttr(cserver, "username"));
  thisbox->aprofile->password = strdup(XMLgetAttr(cserver, "password"));
  thisbox->aprofile->address  = strdup(XMLgetAttr(cserver, "address"));

  //Setup persistent mailboxobj settings
  tempstr = malloc(strlen("data/servers//drafts/") + 16 + 1);

  sprintf(tempstr,"data/servers/%s/DRAFTS/",XMLgetAttr(cserver,"datadir"));
  thisbox->draftspath = fpathname(tempstr,getappdir(),1);
   
  tempstr[strlen(tempstr) - strlen("drafts/")] = 0;
  tempstr = strcat(tempstr,"SENT/");
  thisbox->sentpath = fpathname(tempstr,getappdir(),1);

  thisbox->server = cserver;

  //Setup INBOX specific settings
  sprintf(tempstr,"data/servers/%s/", XMLgetAttr(cserver,"datadir"));
  thisbox->path = fpathname(tempstr,getappdir(),1);
  thisbox->inboxpath = strdup(thisbox->path);
  free(tempstr);

  thisbox->unread = atoi(XMLgetAttr(cserver, "unread"));
  thisbox->thisboxname = strdup("INBOX");

  thisbox->columns[0] = strdup(XMLgetAttr(cserver,"column0"));
  thisbox->columns[1] = strdup(XMLgetAttr(cserver,"column1"));

  thisbox->columnwidths[0] = atoi(XMLgetAttr(cserver,"col0width"));
  thisbox->columnwidths[1] = atoi(XMLgetAttr(cserver,"col1width"));

  //Open the INBOX!
  openmailbox(thisbox);
  closemailbox(thisbox);

  //In this instance we are closing the INBOX... 
  tempstr = malloc(10);
  sprintf(tempstr,"%d",thisbox->columnwidths[0]);
  XMLsetAttr(cserver,"col0width",tempstr);
  sprintf(tempstr,"%d",thisbox->columnwidths[1]);
  XMLsetAttr(cserver,"col1width",tempstr);

  sprintf(tempstr,"%d",thisbox->unread);
  XMLsetAttr(cserver,"unread",tempstr);
  sprintf(tempstr,"%d",thisbox->howmanymessages);
  XMLsetAttr(cserver,"howmany",tempstr);

  XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(), 1));
  system("sync");
}

void inboxselect() {
  DOMElement *temp, *server, *cserver;
  int unread, arrowpos, arrowhpos, servercount, mdethpos = 0, mtlcapos = 0;
  char * tempstr;
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
    cserver = server;
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
  drawinboxselectlist(cserver);

  if(!noservers)
    mailwatchmenuitem(cserver);    

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
        drawinboxselectlist(cserver);
   
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

        prepinboxforopen(cserver);

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
    printf("Mail V%s for WiNGs will only run on\nan 80 column console or wider.  Press\nCommodore key and Backarrow together\nto switch to screen modes", VERSION);
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
     tempstr = malloc(partslen + strlen(soundsettings->hello));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->hello, part2);
   break;
   case NEWMAIL:
     tempstr = malloc(partslen + strlen(soundsettings->newmail));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->newmail, part2);
   break;
   case NONEWMAIL:
     tempstr = malloc(partslen + strlen(soundsettings->nonewmail));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->nonewmail, part2);
   break;
   case DOWNLOADDONE:
     tempstr = malloc(partslen + strlen(soundsettings->downloaddone));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->downloaddone, part2);
   break;
   case MAILSENT:
     tempstr = malloc(partslen + strlen(soundsettings->mailsent));
     sprintf(tempstr, "%s%s%s", part1, soundsettings->mailsent, part2);
   break;
   case GOODBYE:
     tempstr = malloc(partslen + strlen(soundsettings->goodbye));
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

  tempstr = malloc(strlen("/dev/tcp/:110") + strlen(aprofile->address)+1);
  sprintf(tempstr, "/dev/tcp/%s:110", aprofile->address);
  fp = fopen(tempstr, "r+");
  free(tempstr);

  if(!fp){
    tempstr = malloc(strlen("The server '' could not be connected to.") + strlen(aprofile->address) +1);
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

dataset * getnewmsgsinfo(mailboxobj * thisbox) {
  int count, skipped;
  ulong firstnum, totalsize, msgsize, skipsize;
  char * ptr;
  dataset * ds;

  if(!establishconnection(thisbox->aprofile))
    return(NULL);

  firstnum = atol(XMLgetAttr(thisbox->messages, "firstnum"));

  fflush(fp);
  fprintf(fp, "LIST\r\n");
  
  fflush(fp);

  count = 0;
  totalsize = 0;
  skipped = 0;
  skipsize = strtoul(XMLgetAttr(thisbox->server, "skipsize"),NULL,10);

  getline(&buf, &size, fp); //Gets the +OK message
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

  ptr = malloc(80);

  ds = malloc(sizeof(dataset));    

  //progressbar measures in increments of 1k
  ds->number = totalsize; 

  if(totalsize > 1048576) {
    totalsize /= 1048576;
    if(totalsize > 1)
      sprintf(ptr, "%d New messages. %d skipped. %lu Megabytes.",count-skipped,skipped,totalsize);
    else
      sprintf(ptr, "%d New messages. %d skipped. 1 Megabyte.",count-skipped, skipped);
  } else if(totalsize > 1024) {
    totalsize /= 1024;
    if(totalsize > 1)
      sprintf(ptr, "%d New messages. %d skipped. %lu Kilobytes.",count-skipped,skipped,totalsize);
    else
      sprintf(ptr, "%d New messages. %d skipped. 1 Kilobyte.",count-skipped,skipped);
  } else {
    sprintf(ptr, "%d New messages. %d skipped. %lu bytes.",count-skipped,skipped,totalsize);
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
  char * subject, * from, *replyto;
  char * boundary, * bstart, * name, * freename, *ptr;
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

    //printf("getting msg %u\n",i);con_update();

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

    tempstr = malloc(strlen(serverpath)+8);
    sprintf(tempstr, "%s%ld", serverpath, refnum);
    outfile = fopen(tempstr, "w");

    progbarcounter += getline(&buf, &size, fp);
    setprogress(mb,progbarcounter);

    //Get message header

    subject = NULL;
    from    = NULL;
    replyto = NULL;

    while(!(buf[0] == '\n' || buf[0] == '\r')) {
      fprintf(outfile, "%s", buf);

      if(!strncasecmp("subject:", buf, 8)) 
        subject = strdup(buf);
      else if(!strncasecmp("from:", buf, 5))
        from = strdup(buf);
      else if(!strncasecmp("reply-to:", buf, 9))
        replyto = strdup(buf);
      else if(!strncasecmp("content-type: multipart/", buf, 24)) {

        //Check for boundary on the content-type line... 
        //if not there, look to get it on the next line...  

        if(!strcasestr(buf, "boundary")) {
          progbarcounter += getline(&buf, &size, fp);
          setprogress(mb,progbarcounter);
          fprintf(outfile, "%s", buf);
        } else {
          //efficiency thing... bstart is used as a temporary flag here.
          bstart++;
        }

        //Check for and extract the boundary

        if(bstart || strcasestr(buf,"boundary")) {
          bstart   = strdup(buf);

          //set boundary to 1st char after the 'y'
          boundary = strcasestr(bstart, "boundary") + strlen("boundary");

          //set boundary to 1st char after the = sign
          while(boundary[0] != '=' && boundary[0] != '\r')
            boundary++;
          boundary++;

          //trim any unwanted spaces... shouldn't be any anyway.
          while(boundary[0] == ' ')
            boundary++;

          if(boundary[0] == '\'' || boundary[0] == '"') {
            //Boundary has quotes around it as it should.

            boundary++;
            if(strpbrk(boundary, "\"'"))
              *strpbrk(boundary, "\"'") = 0;
            else
              *strpbrk(boundary, "\r\n") = 0;

          } else {
            *strpbrk(boundary,"\r\n") = 0;
          }
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

                freename = name = strdup(strstr(buf, "name"));
                name += 4;
                while(name[0] == ' ')
                  name++;
                while(name[0] == '=')
                  name++;
                while(name[0] == ' ')
                  name++;

                if(strchr(name,'"'))
                  name = strchr(name,'"') +1;
                if(strchr(name,'"'))
                  *strchr(name,'"') = 0;
                else
                  *strpbrk(name,"\"'") = 0;

                attachment = XMLnewNode(NodeType_Element, "attachment", "");
                XMLsetAttr(attachment, "filename", name);
                XMLinsert(message, NULL, attachment);
                free(freename);
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

    if(replyto) {
      if(replyto[strlen(replyto)-2] == '\r')
        replyto[strlen(replyto)-2] = 0;
      else if(replyto[strlen(replyto)-1] == '\n')
        replyto[strlen(replyto)-1] = 0;
    }

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
    if(replyto)
      XMLsetAttr(message, "replyto", &replyto[10]);
    tempstr = itoa(attachments);
    XMLsetAttr(message, "attachments", tempstr);
    free(tempstr);
    tempstr = itoa(refnum);
    XMLsetAttr(message, "fileref", tempstr);
    free(tempstr);
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

    tempstr = itoa(count);
    XMLsetAttr(messages, "firstnum", tempstr);
  } else {
    count++;
    tempstr = itoa(count);
    XMLsetAttr(messages, "firstnum", tempstr);
  }
  free(tempstr);

  tempstr = itoa(refnum);
  XMLsetAttr(messages, "refnum", tempstr);
  free(tempstr);

  system("sync");

  return(returnvalue);
}
