//Mail V2.x for Wings
#include "mailheader.h"

extern int listfg_col,listbg_col;
extern FILE * fp;
extern char *scrollup;
extern int size;
extern char * buf;

FILE *msgfile; //global to be piped through web or base64 in a thread


//Multithreaded HTML parsing globals 
int writetowebpipe[2]; 
int readfromwebpipe[2]; 
msgline * htmlfirstline;
int writetobase64pipe[2];
int readfrombase64pipe[2];

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
  header->replyto    = NULL;
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

  ptr = strcasestr(headerstr, "\r\nreply-to:");
  if(ptr)
    header->replyto = ptr+12;

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
  header->replyto    = terminateheaderstr(header->replyto,0);
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
  headerstr = ptr = calloc(bufsize,1);

  // 1k buffer grows if size exceeded.

  while(!(ptr[-4]=='\r' && ptr[-3]=='\n' && ptr[-2]=='\r' && ptr[-1]=='\n')) {
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
    if(ptr) {
      header->filename = ptr+9;
      if(header->filename[0] == '"')
        header->filename++;
    }
  }

  if(!strncasecmp(header->contenttype,"multipart/", 10)) {
    ptr = strcasestr(header->contenttype, "boundary=");
    if(ptr) {
      header->boundary = ptr+9;
      if(header->boundary[0] == '"')
        header->boundary++;
    }
  }
    
  header->contenttype = terminateheaderstr(header->contenttype,0);  
  header->encoding    = terminateheaderstr(header->encoding,0);  
  header->disposition = terminateheaderstr(header->disposition,0);  

  if(header->filename) {
    if(strchr(header->filename,'"'))
      *strchr(header->filename,'"') = 0;
  }
  header->filename = strdup(header->filename);

  if(header->boundary) {

    //TOTAL hack, I have no idea why this works... but it does.
    if(strlen(header->boundary) > 80)
      header->boundary[80] = 0;

    if(strchr(header->boundary, '"')) {
      *strchr(header->boundary, '"') = 0;
      //drawmessagebox("there is a quote",header->boundary,1);
    } else if(strchr(header->boundary, '\r')) {
      *strchr(header->boundary, '\r') = 0;
      //drawmessagebox("there is a slash r",header->boundary,1);
    } else if(strchr(header->boundary, '\n')) {
      *strchr(header->boundary, '\n') = 0;
      //drawmessagebox("there is a slash n",header->boundary,1);
    }
  }
  header->boundary = strdup(header->boundary);

  //drawmessagebox("final boundary... ",header->boundary,1);

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
  getline(&buf, &size, msgfile);
//  if(strlen(buf) > 2) buf[strlen(buf)-2] = 0;
//  drawmessagebox(buf,mainmimehdr->boundary,1);

  while(!strstr(buf,mainmimehdr->boundary)) {
    getline(&buf, &size, msgfile);
//    if(strlen(buf) > 2) buf[strlen(buf)-2] = 0;
//    drawmessagebox(buf,mainmimehdr->boundary,1);
  }
  submimeheader = getmimeheader(msgfile);

  //drawmessagebox("submime header contenttype", submimeheader->contenttype,1);

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

int prepforview(int fileref, mailboxobj * thisbox){
  displayheader * displayhdr;
  mimeheader * mainmimehdr;
  msgline * firstline;
  int returnval;
  char * tempstr;

  tempstr = malloc(strlen(thisbox->path)+17);
  sprintf(tempstr, "%s%d", thisbox->path, fileref);

  if(!(msgfile = fopen(tempstr, "r"))) {
    drawmessagebox("An internal error has occurred. File Not Found.", tempstr,1);
    free(tempstr);
    return(0);
  } 
  displayhdr = getdisplayheader(msgfile);
  fclose(msgfile);

  //Open the file again... 
  msgfile = fopen(tempstr, "rb");
  free(tempstr);
  mainmimehdr = getmimeheader(msgfile);

  //drawmessagebox("contenttype", mainmimehdr->contenttype,1);
  //drawmessagebox("encoding",mainmimehdr->encoding,1);
  //drawmessagebox("main boundary",mainmimehdr->boundary,1);

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

  returnval = view(firstline, displayhdr, thisbox);

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
  
  if(strlen(header->replyto) && bighdr) {
    colourheaderrow(row);
    printf("  Reply To: %s", header->replyto);
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

int view(msgline * firstline, displayheader * displayhdr, mailboxobj * thisbox) {
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
        tempstr = malloc(strlen(thisbox->draftspath)+strlen("/temporary.txt")+1);

        sprintf(tempstr, "%s/temporary.txt", thisbox->draftspath);
          
        replyfile = fopen(tempstr, "w");
        if(!replyfile) {
          drawmessagebox("ERROR: Could not create temporary reply file.",tempstr,1);
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

        if(strlen(displayhdr->replyto))
          replyto = strdup(displayhdr->replyto);
        else
          replyto = strdup(displayhdr->from);

        tempstr = replyto;

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
          replysubject = malloc(strlen(displayhdr->subject)+strlen("Re: ")+1);
          sprintf(replysubject,"Re: %s", displayhdr->subject);
        } else 
          replysubject = strdup(displayhdr->subject);

        replied = compose(thisbox,strdup(replyto),replysubject,NULL,0,NULL,0,NULL,0,REPLY);
        free(tempstr);
 
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

  fileref = strdup(XMLgetAttr(message, "fileref"));
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

