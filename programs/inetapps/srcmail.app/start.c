// Mail V2.0 for Wings 

//Note: Find the XML based pageable list code in drawinboxselectlist() 
//      and inboxselect(). 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>
#include <console.h>
#include <termio.h>
#include <xmldom.h>
extern char *getappdir();

// Sound Event Defines
#define HELLO     1
#define NEWMAIL   2
#define NONEWMAIL 3
#define MAILSENT  4
#define REFRESH   5
#define GOODBYE   6

typedef struct msgline_s {
  struct msgline_s * prevline;
  struct msgline_s * nextline;
  char * line;
} msgline;

// ***** GLOBAL Variables ***** 

DOMElement * configxml; //the root element of the config xml element.

FILE *fp;        // Main Server connection.
FILE *msgfile;   // only global so it can be piped through web in a thread
char *server;    // Server name as text

int writetowebpipe[2];
int readfromwebpipe[2];
int eom = 0;
int pq = 0; //printed-quotable encoding
char * pqhexbuf;

int sounds = 0;  // 1 = sounds on, 0 = sounds off. 
char *sound1, *sound2, *sound3, *sound4, *sound5, *sound6;

int  size       = 0;
char * buf      = NULL;
char * boundary = NULL;

struct termios tio;

int logofg_col,     logobg_col,     serverselectfg_col, serverselectbg_col;
int listfg_col,     listbg_col,     listheadfg_col,     listheadbg_col;
int listmenufg_col, listmenubg_col, messagefg_col,      messagebg_col;

// *** FUNCTION DECLARATIONS ***

int setupsounds();
int setupcolors();

int playsound(int soundevent);

void memerror();

int getnewmail(char * username, char * password, char * address, DOMElement * messages, char * serverpath);

int view(int fileref, char * serverpath);
msgline * parsehtmlcontent(msgline * prevline); 
void givesomedatatoweb(); //fprintf data to a pipe until you hit a boundary
void givealldatatoweb();  //printf all data (regardless of mime) to a pipe

void viewattachedlist(DOMElement * message);


void curup(int num) {
  printf("\x1b[%dA", num);
  con_update();
}
void curdown(int num) {
  printf("\x1b[%dB", num);
  con_update();
}
void curleft(int num) {
  printf("\x1b[%dD", num);
  con_update();
}
void curright(int num) {
  printf("\x1b[%dC", num);
  con_update();
}

void drawongrid(int x, int y, char item) {
  con_gotoxy(x, y);
  putchar(item);
  con_update();
}

void drawmessagebox(char * string) {
  int width, startcolumn, i;

  width       = strlen(string)+6;
  startcolumn = (con_xsize - width)/2;

  con_gotoxy(startcolumn, 10);
  printf(" ");
  for(i = 0; i < width-2; i++) 
    printf("_");
  printf(" ");

  con_gotoxy(startcolumn, 11);
  printf(" |");
  for(i = 0; i < width-4; i++)
    printf(" ");
  printf("| ");

  con_gotoxy(startcolumn, 12);
  printf(" | %s | ", string);

  con_gotoxy(startcolumn, 13);
  printf(" |");
  for(i = 0; i < width-4; i++)
    printf(" ");
  printf("| ");

  con_gotoxy(startcolumn, 14);
  printf(" ");
  for(i = 0; i < width-2; i++) 
    printf("-");
  printf(" ");

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

void lineeditmode() {
  tio.flags |= TF_ICANON;
  settio(STDOUT_FILENO, &tio);
}

void onecharmode() {
  tio.flags &= ~TF_ICANON;
  settio(STDOUT_FILENO, &tio);
}

void drawlogo() {
  DOMElement * splashlogo;
  con_clrscr();
  con_update();

  splashlogo = XMLgetNode(configxml, "xml/splashlogo");

  printf("%s", splashlogo->Node.Value);
}

void setcolors(int fg_col, int bg_col) {
  con_setfg(fg_col);
  con_setbg(bg_col);
  con_update();
}

int makeserverinbox(char * server) {
  char * path;
  char * tempstr = NULL;

  if(strlen(server) > 16)
    server[16] = 0;
 
  path    = fpathname("data/servers/", getappdir(), 1);
  tempstr = (char *)malloc(strlen(path)+strlen(server)+2);

  if(tempstr == NULL)
    memerror();

  sprintf(tempstr, "%s%s", path, server);
  return(mkdir(tempstr, 0));
}

void makenewmessageindex(char * server) {
  FILE * messageindex;
  char * tempstr = NULL;

  if(strlen(server) > 16)
    server[16] = 0;
 
  tempstr = (char *)malloc(strlen("data/servers//index.xml")+strlen(server)+2);
  if(tempstr == NULL)
    memerror();

  sprintf(tempstr, "data/servers/%s/index.xml", server);
  messageindex = fopen(fpathname(tempstr, getappdir(), 1), "w");

  fprintf(messageindex, "<xml><messages firstnum=\"1\" refnum=\"0\"></messages></xml>");
  fclose(messageindex);
}

int addserver(DOMElement * servers) {
  DOMElement * newserver;
  char * name,* address,* username,* password;
  char input = 'n';

  con_clrscr();
  con_update();
  lineeditmode();

  while(!(input == 'y'||input == 'Y')) {
    putchar('\n');
    putchar('\n');
    printf("                      Email account setup assistant\n\n");
    printf("    Display Name for the server: ");
    con_update();
    getline(&buf, &size, stdin);
    name = strdup(buf);
    name[strlen(name)-1] = 0;

    printf("    Address of this server: ");
    con_update();
    getline(&buf, &size, stdin);
    address = strdup(buf);
    address[strlen(address)-1] = 0;

    printf("    Username for this server: ");
    con_update();
    getline(&buf, &size, stdin);
    username = strdup(buf);
    username[strlen(username)-1] = 0;

    printf("    Password for this server: ");
    con_update();
    tio.flags &= ~TF_ECHO;
    settio(STDOUT_FILENO, &tio);
    getline(&buf, &size, stdin);
    tio.flags |= TF_ECHO;
    settio(STDOUT_FILENO, &tio);
    password = strdup(buf);
    password[strlen(password)-1] = 0;

    putchar('\n');
    putchar('\n');
    printf("       --** Information Overview **--\n\n");
    printf("          Server Name: %s\n", name);
    printf("              Address: %s\n", address);
    printf("             Username: %s\n", username);
    printf("             Password: *********\n");

    con_gotoxy(0,23);
    printf(" Correct? (y/n), (a)bort");
    con_update();
    onecharmode();
    input = getchar();
    lineeditmode();
    if(input == 'a')
      return(0);
    if(!(input == 'y'||input == 'Y')) {
      free(name);
      free(address);
      free(username);
      free(password);
      con_clrscr();
    }
  }

  if(makeserverinbox(strdup(address)) == -1) {
    printf("\nAn error occurred while trying to create the inbox.\n");
    printf("You may have an inbox for '%s' setup already.\n", address);
    printf("Press any key to continue.\n");
    onecharmode();
    getchar();
    return(0);
  } else {
    makenewmessageindex(strdup(address));
    newserver = XMLnewNode(NodeType_Element, "server", "");
    XMLsetAttr(newserver, "name", name);
    XMLsetAttr(newserver, "address", address);
    XMLsetAttr(newserver, "username", username);
    XMLsetAttr(newserver, "password", password);
    XMLsetAttr(newserver, "unread", "0");

    //insert the new element as a child of "servers"
    XMLinsert(servers, NULL, newserver);
  }
  return(1);
}

void drawinboxheader() {
  con_gotoxy(1,0);
  printf("Mail v2.0 for WiNGs                                             By DAC in 2003");
  con_gotoxy(0,1);
  printf("< S >--< FROM >--------------------< SUBJECT >-----------------------------< A >");
}

void drawinboxmenu() {
  con_gotoxy(1,24);
  printf("(+/-), (Q)uit to inbox select, get (N)ew mail, (v)iew attached, (d)elete");
}

int drawinboxlist(DOMElement * message, int direction, int first) {
  //direction 0 = down, 1 = up

  char * subject, * from, * status, * attachments;
  int i;
  con_clrscr();

  drawinboxheader();
  drawinboxmenu();

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
      from = XMLgetAttr(message, "from");
      if(strlen(from) > 20)
        from[20] = 0;
      printf("%s", from);

      con_gotoxy(25, i);
      subject = XMLgetAttr(message, "subject");
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
      from = XMLgetAttr(message, "from");
      if(strlen(from) > 20)
        from[20] = 0;
      printf("%s", from);

      con_gotoxy(25, i);
      subject = XMLgetAttr(message, "subject");
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

void openinbox(DOMElement * inboxinfo) {
  DOMElement * inboxindex, * messages, * message, *reference;
  int unread, direction, first, newmessages, i;
  char * name, * username, * password, * address;
  char * inboxdir, * tempstr, * serverpath;
  int fileref;
  char input = ' ';
  int lastline, more;
  int arrowpos;
  int nomessages = 0;

  unread   = atoi(XMLgetAttr(inboxinfo, "unread"));
  name     = XMLgetAttr(inboxinfo, "name");
  username = XMLgetAttr(inboxinfo, "username");
  password = XMLgetAttr(inboxinfo, "password");
  address  = XMLgetAttr(inboxinfo, "address");

  inboxdir = strdup(address);
  if(strlen(inboxdir) > 16)
    inboxdir[16] = 0;  

  tempstr = (char *)malloc(strlen(inboxdir)+strlen("data/servers//index.xml")+2);
  if(tempstr == NULL)
    memerror();
  
  sprintf(tempstr, "data/servers/%s/index.xml", inboxdir);
  inboxindex = XMLloadFile(fpathname(tempstr, getappdir(), 1));
  free(tempstr);

  tempstr = (char *)malloc(strlen(inboxdir)+strlen("data/servers//")+2);
  if(tempstr == NULL)
    memerror();

  sprintf(tempstr, "data/servers/%s/", inboxdir);
  serverpath = fpathname(tempstr, getappdir(), 1);
  free(tempstr);

  messages = XMLgetNode(inboxindex, "xml/messages");
  message  = XMLgetNode(messages, "message");
  
  if(message == NULL) {
    nomessages = 1;
    message = XMLnewNode(NodeType_Element, "message", "");
    XMLsetAttr(message, "from", "");
    XMLsetAttr(message, "status", "");
    XMLsetAttr(message, "attachments", "");
    XMLsetAttr(message, "fileref", "");
    XMLsetAttr(message, "subject", "Inbox is currently empty.");
    message->FirstElem = 1;
    message->PrevElem = message;
    message->NextElem = message;
  }

  onecharmode();

  con_clrscr();

  lastline = drawinboxlist(message, 0, 1);
  reference = message;
  direction = 0;
  first = 1;

  arrowpos = 2;
  con_gotoxy(0,arrowpos);
  putchar('>');

  con_update();
  input = -1;

  while(input != 'Q') {
    input = getchar();

    switch(input) {
      case '-':
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
            lastline = drawinboxlist(message, 0, 0);
            con_update();
            arrowpos = 2;
            con_gotoxy(0,arrowpos);
            putchar('>');
            con_update();
          }
        }
      break;
      case '+':
        if(arrowpos > 2) {
          movecharup(0, arrowpos, '>');
          arrowpos--;
          message = message->PrevElem;
        } else if(!message->FirstElem) {
          message = message->PrevElem;
          reference = message;
          direction = 1;
          first = 0;
          lastline = drawinboxlist(message, 1, 0);
          con_update();
          arrowpos = 22;
          con_gotoxy(0, arrowpos);
          putchar('>');
          con_update();
        }
      break;
      case '\n':
        if(!strcmp(XMLgetAttr(message, "fileref"), ""))
          break;
        fileref = atoi(XMLgetAttr(message, "fileref"));
        if(!strcmp(XMLgetAttr(message, "status"),"N")) {
          unread--;
          XMLsetAttr(message, "status", " ");
        }         
        view(fileref, serverpath);
        lastline = drawinboxlist(reference, direction, first);
        con_gotoxy(0, arrowpos);
        putchar('>');
        con_update();
      break;
      case 'v':
        if(atoi(XMLgetAttr(message, "attachments"))) {
          viewattachedlist(message);
          lastline = drawinboxlist(reference, direction, first);
          con_gotoxy(0, arrowpos);
          putchar('>');
          con_update();
        }
      break;
      case 'N':
        drawmessagebox("Downloading new Mail and attachments");

        newmessages = getnewmail(username, password, address, messages, serverpath);

        if(newmessages) {
          playsound(NEWMAIL);
          unread += newmessages;
          if(nomessages) {
            nomessages = 0;
            message = XMLgetNode(messages, "message");
            reference = message;
          }

          tempstr = (char *)malloc(strlen(serverpath)+strlen("index.xml")+2);

          sprintf(tempstr, "%sindex.xml", serverpath);
          XMLsaveFile(inboxindex, tempstr);
          free(tempstr);
  
          tempstr = (char *)malloc(7);        

          sprintf(tempstr, "%d", unread);
          XMLsetAttr(inboxinfo, "unread", tempstr);

          XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(), 1));
        } else {
          playsound(NONEWMAIL);
        }

        lastline = drawinboxlist(reference, direction, first);
        con_gotoxy(0, arrowpos);
        putchar('>');
        con_update();
      break;
      case 'd':
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

        message = XMLgetNode(messages, "message");
        for(i = 0; i<messages->NumElements; i++) {
          if(XMLfindAttr(message, "delete")) {

            reference = message->NextElem;

            if(!strcmp(XMLgetAttr(message, "status"), "N"))
              unread--;

            tempstr = (char *)malloc(strlen(serverpath)+15);
            sprintf(tempstr, "%s%s", serverpath, XMLgetAttr(message, "fileref"));
            unlink(tempstr);

            XMLremNode(message);
            message = reference;  
          } else 
            message = message->NextElem;
        }

      //save inbox xmlfile

        tempstr = (char *)malloc(strlen(serverpath)+strlen("index.xml")+2);
        if(tempstr == NULL)
          memerror();
  
        sprintf(tempstr, "%sindex.xml", serverpath);
        XMLsaveFile(inboxindex,tempstr);
        free(tempstr);
        
      //set inbox unread attribute

        tempstr = (char *)malloc(7);        

        sprintf(tempstr, "%d", unread);
        XMLsetAttr(inboxinfo, "unread", tempstr);

      //save mailconfig.xml

        XMLsaveFile(configxml, fpathname("resources/mailconfig.xml", getappdir(), 1));
      break;
    }    
  }
}

int drawinboxselectlist(DOMElement * server, int direction, int first) {

  char * servername, * unread;
  int i;
  con_clrscr();

  drawlogo();

  con_gotoxy(1,23);
  printf("(+/-), (a)dd new account, (Q)uit");
  con_update();

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
     
      con_gotoxy(45, i);
      unread = XMLgetAttr(server, "unread");
      printf("(%s unread)", unread);

      server = server->NextElem;
    } 
  } else {
    for(i = 21; i>16; i--) {

      con_gotoxy(15, i);
      servername = XMLgetAttr(server, "name"); 
      printf("%s", servername);

      if(server->FirstElem)
        return(21);

      server = server->PrevElem;
    }
  }
  return(21);
}

void inboxselect() {
  DOMElement *temp, *server, *reference;
  int first, direction, unread, lastline, arrowpos;
  char * inboxname;
  char input;
  int noservers = 0;

  temp = XMLgetNode(configxml, "xml/servers");

  server = XMLgetNode(temp, "server");

  if(server == NULL) {
    noservers = 1;
    server = XMLnewNode(NodeType_Element, "server", "");
    XMLsetAttr(server, "name", "No servers configured.");
    XMLsetAttr(server, "address", "");
    XMLsetAttr(server, "username", "");
    XMLsetAttr(server, "password", "");
    XMLsetAttr(server, "unread", "0");
    server->FirstElem = 1;
    server->PrevElem = server;
    server->NextElem = server;
  }

  lastline = drawinboxselectlist(server, 0, 1);
  reference = server;
  direction = 0;
  first = 1;

  arrowpos = 17;
  con_gotoxy(0,arrowpos);
  putchar('>');

  onecharmode();
  con_update();
  input = getchar();

  while(input != 'Q') {

    switch(input) {
      case '-':
        if(arrowpos < lastline) {
          movechardown(0, arrowpos, '>');
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
            lastline = drawinboxselectlist(server, 0, 0);
            con_update();
            arrowpos = 17;
            con_gotoxy(0,arrowpos);
            putchar('>');
            con_update();
          }
        }
      break;
      case '+':
        if(arrowpos > 17) {
          movecharup(0, arrowpos, '>');
          arrowpos--;
          server = server->PrevElem;
        } else if(!server->FirstElem) {
          server = server->PrevElem;
          reference = server;
          direction = 1;
          first = 0;
          lastline = drawinboxselectlist(server, 1, 0);
          con_update();
          arrowpos = 21;
          con_gotoxy(0, arrowpos);
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
          }
        }
        lastline = drawinboxselectlist(reference, direction, first);
        con_gotoxy(0,arrowpos);
        putchar('>');
        con_update();
      break;
      case '\n':
        if(noservers)
          break;
        openinbox(server);
        lastline = drawinboxselectlist(reference, direction, first);
        con_gotoxy(0,arrowpos);
        putchar('>');
        con_update();
      break;
    }
    onecharmode();
    input = getchar();  
  }
}

void helptext() {
  printf("USAGE: mail [-h]\n");
  printf("       -h this help text.\n");
  exit(1);
}

void memerror() {
  printf("Memory Allocation Error.\n");
  exit(1);
}

// *** MAIN ***

void main(int argc, char *argv[]){
  char * path    = NULL;
  char * tempstr = NULL;
  int ch;
  
  while((ch = getopt(argc, argv, "h")) != EOF) {
    switch(ch) {
      case 'h':
        helptext();
      break;
    }
  }

  con_init();

  if(con_xsize < 80) {
    con_end();
    printf("Mail V2.0 for WiNGs will only run on an 80 column console\n");
    exit(1);
  }

  gettio(STDOUT_FILENO, &tio);

  tio.flags |= TF_ECHO|TF_ICRLF;
  tio.MIN = 1;
  settio(STDOUT_FILENO, &tio);

  path = fpathname("resources/mailconfig.xml", getappdir(), 1);
  configxml = XMLloadFile(path);

  sounds = setupsounds();
  setupcolors();

  //allocated 2 chars and \n for the quoted-printable Hex encoding.
  pqhexbuf = (char *)malloc(3);

  con_clrscr();
  con_update();

  playsound(HELLO);

  inboxselect(); //The program never leaves the inboxselect function, til
                 //the user is quitting the program.

  playsound(GOODBYE);

  con_end();

  printf("\x1b[0m"); //reset the terminal.
  con_clrscr();
  con_update();
}

int setupsounds(){
  DOMElement * temp;
  char * active;

  temp = XMLgetNode(configxml, "xml/sounds");

  active = XMLgetAttr(temp,"active");

  if(!strcmp(active, "no")) {
    return(0);
  } else if(!strcmp(active, "yes")){
    temp   = XMLgetNode(configxml, "xml/sounds/hello");
    sound1 = strdup(XMLgetAttr(temp, "file"));

    temp   = XMLgetNode(configxml, "xml/sounds/newmail");
    sound2 = strdup(XMLgetAttr(temp, "file"));

    temp   = XMLgetNode(configxml, "xml/sounds/nonewmail");
    sound3 = strdup(XMLgetAttr(temp, "file"));

    temp   = XMLgetNode(configxml, "xml/sounds/mailsent");
    sound4 = strdup(XMLgetAttr(temp, "file"));

    temp   = XMLgetNode(configxml, "xml/sounds/goodbye");
    sound5 = strdup(XMLgetAttr(temp, "file"));

    return(1);
  }
  return(0);
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

  if (!sounds) 
    return(0);

  part1 = strdup("wavplay ");
  part2 = strdup(" 2>/dev/null >/dev/null &");
  
  switch(soundevent) {
   case HELLO:
     tempstr = (char *)malloc(strlen(part1)+strlen(sound1)+strlen(part2)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "%s%s%s", part1, sound1, part2);
   break;
   case NEWMAIL:
     tempstr = (char *)malloc(strlen(part1)+strlen(sound2)+strlen(part2)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "%s%s%s", part1, sound2, part2);
   break;
   case NONEWMAIL:
     tempstr = (char *)malloc(strlen(part1)+strlen(sound3)+strlen(part2)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "%s%s%s", part1, sound3, part2);
   break;
   case MAILSENT:
     tempstr = (char *)malloc(strlen(part1)+strlen(sound4)+strlen(part2)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "%s%s%s", part1, sound4, part2);
   break;
   case GOODBYE:
     tempstr = (char *)malloc(strlen(part1)+strlen(sound5)+strlen(part2)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "%s%s%s", part1, sound5, part2);
   break;
  }

  system(tempstr);

  free(tempstr);
  free(part1);
  free(part2);

  return(1);
}

int getnewmail(char * username, char * password, char * address, DOMElement * messages, char * serverpath){
  DOMElement * message, * attachment;
  char * tempstr;
  FILE * outfile;
  int count, i, eom;
  unsigned long firstnum, refnum;
  char * subject, * from, * boundary, * bstart, * name;
  int attachments;

  refnum   = atoi(XMLgetAttr(messages, "refnum"));
  firstnum = atoi(XMLgetAttr(messages, "firstnum"));

  tempstr = (char *)malloc(strlen("/dev/tcp/:110")+strlen(address)+2);
  sprintf(tempstr, "/dev/tcp/%s:110", address);
  fp = fopen(tempstr, "r+");
  free(tempstr);

  if(!fp){
    printf("The server '%s' could not be connected to\n", address);
    exit(-1);
  }

  fflush(fp);
  getline(&buf, &size, fp);

  fflush(fp);
  fprintf(fp, "USER %s\r\n", username);

  fflush(fp);
  getline(&buf, &size, fp);

  fflush(fp);
  fprintf(fp, "PASS %s\r\n", password);

  fflush(fp);
  getline(&buf, &size, fp);

  if(buf[0] == '-'){
    printf("Error: Username and/or Password incorrect\n");
    exit(-1);
  }
  
  fflush(fp);
  fprintf(fp, "LIST\r\n");

  fflush(fp);
  count = 0;

  getline(&buf, &size, fp);

  do {
    getline(&buf, &size, fp);
    count++;
  } while(buf[0] != '.');
  count--;  

  for(i = firstnum; i<=count; i++) {
    refnum++;
    attachments = 0;
    eom = 0;
    bstart   = NULL;
    boundary = NULL;
    name     = NULL;

    message = XMLnewNode(NodeType_Element, "message", "");

    //request the whole message. 
    fflush(fp);
    fprintf(fp, "RETR %d\r\n", i);
    
    fflush(fp);
    getline(&buf, &size, fp);

    tempstr = (char *)malloc(strlen(serverpath)+8);
    sprintf(tempstr, "%s%ld", serverpath, refnum);
    outfile = fopen(tempstr, "w");

    getline(&buf, &size, fp);
    while(!(buf[0] == '\n' || buf[0] == '\r')) {
      fprintf(outfile, "%s", buf);
      if(!strncasecmp("subject:", buf, 8)) 
        subject = strdup(buf);
      else if(!strncasecmp("from:", buf, 5))
        from = strdup(buf);
      else if(!strncasecmp("content-type: multipart/", buf, 24)) {

        //Get the Next line presumably with the boundary... 
        if(!strstr(buf, "oundary")) {
          getline(&buf, &size, fp);
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

      getline(&buf, &size, fp);      
    }

    //this saves the blank space between the header and body
    fprintf(outfile, "%s", buf);
    getline(&buf, &size, fp);

    if(boundary) {
      while(!((buf[0] == '.' && buf[1] == '\r')||(buf[0] == '.' && buf[1] == '\n'))) {
        fprintf(outfile, "%s", buf);

        if(!strncmp(buf, "--", 2)) {
          if(!strncmp(&buf[2], boundary, strlen(boundary))) {

            while(!((buf[0]=='\n')||(buf[0]=='\r'))) {
              getline(&buf, &size, fp);

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
  
        if(!eom)
          getline(&buf, &size, fp);      
      } 
    } else {
      while(!((buf[0] == '.' && buf[1] == '\r')||(buf[0] == '.' && buf[1] == '\n'))) {
        fprintf(outfile, "%s", buf);
        getline(&buf, &size, fp);      
      } 
    }

    fclose(outfile);
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
  }

  fclose(fp);

  tempstr = (char *)malloc(15);
  sprintf(tempstr, "%d", count+1);
  XMLsetAttr(messages, "firstnum", tempstr);

  tempstr = (char *)malloc(15);
  sprintf(tempstr, "%d", refnum);
  XMLsetAttr(messages, "refnum", tempstr);
  return(count - firstnum + 1);
}

int view(int fileref, char * serverpath){
  FILE * incoming;
  char * subject, * from, * date, * bstart, * name;
  char * bodytext, * headertext, * tempstr, * line, * lineptr;
  msgline * thisline, * prevline, * topofview;
  int charcount, i, html, c;
  char input;
  
  tempstr = (char *)malloc(strlen(serverpath)+15);
  if(!tempstr)
    memerror();
  
  sprintf(tempstr, "%s%d", serverpath, fileref);
  msgfile = fopen(tempstr, "r");

  if(!msgfile) {
    drawmessagebox("An internal error has occurred. File Not Found.");
    getchar();
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

      if(!incoming) {
        printf("Failed to open readfromwebpipe\n");con_update();
        exit(1);
      }

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
//          printf("%d\n", c); con_update();
         
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

        while(charcount < 80) {
          c = fgetc(msgfile);

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

      while(charcount < 80) {
        c = fgetc(msgfile);

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
          case '=':
            if(pq) {
              c = fgetc(msgfile);
              if(c == EOF) {
                eom = 1;
                charcount = 80;
                break;
              }
              if(c == '\n' || c == '\r') 
                break; 
                //do nothing. don't store the = or the \n

              //Check for valid character. if not, could be a boundary
              if(
                 c != '1' &&
                 c != '2' &&
                 c != '3' &&
                 c != '4' &&
                 c != '5' &&
                 c != '6' &&
                 c != '7' &&
                 c != '8' &&
                 c != '9' &&
                 c != '0' &&
                 c != 'A' &&
                 c != 'B' &&
                 c != 'C' &&
                 c != 'D' &&
                 c != 'E' &&
                 c != 'F' 
                ) { 
                                  
                charcount++;
                *lineptr = '=';
                lineptr++;

              } else {
                pqhexbuf[0] = c;
             
                c = fgetc(msgfile);
                if(c == EOF) {
                  eom = 1;
                  charcount = 80;
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

      if(strstr(line, boundary) || html) {
        html = 0;
        pq   = 0;
        if(strstr(line, boundary))
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

  thisline = prevline;
  thisline->nextline = NULL;
  while(thisline->prevline) {
    thisline = thisline->prevline;
  }

  topofview = thisline;

  con_clrscr();

  for(i = 4; i<22; i++) {
    con_gotoxy(0, i);
    printf("%s", thisline->line);
    if(thisline->nextline)
      thisline = thisline->nextline;
    else
      break;
  }  

  thisline = thisline->prevline;

  con_setscroll(4,22);

  con_gotoxy(0,0);
  printf("%s%s%s", date, from, subject);

  con_gotoxy(2,24);
  printf("(+/-), (Q)uit to list");

  con_update();

  input = 'A';
  while(input != 'Q') {
    input = getchar();
    switch(input) {
      case '+' :
        if(topofview->prevline) {
          topofview = topofview->prevline;
          thisline = thisline->prevline;
          con_gotoxy(0,4);
          printf("\x1b[1L");
          printf("%s", topofview->line);
          con_update();
        }
      break;
      case '-':
        if(thisline->nextline) {
          topofview = topofview->nextline;
          thisline = thisline->nextline;
          con_gotoxy(0, 22);
          putchar('\n');
          con_gotoxy(0, 21);
          printf("%s", thisline->line);
          con_update();
        }
      break;
    }
  }
  return(1);  
}

void givealldatatoweb() {
  FILE * output;  
  int eomlocal, i;
  char * tempstr, * ptr;

  output = fdopen(writetowebpipe[1], "w");

  if(!output) {
    printf("Not able to open writetowebpipe\n");
   con_update();
   exit(1);
  }

  if(pq) {
    //Might want to convert this to Char at a time.

    while(eomlocal = getline(&buf,&size, msgfile)) {
      if(eomlocal == EOF) {
        eom = 1;
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
        eom = 1;
        break;
      }
      fprintf(output, "%s", buf);
      printf("%s\n", buf);
      con_update();
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
      if(strstr(buf, boundary))
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
      if(strstr(buf, boundary))
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

void viewattachedlist(DOMElement * message) {
  DOMElement * attachment;
  int i;

  con_clrscr();

  con_gotoxy(0,2);
  printf("                     --** Email Attachments List **--\n");
  putchar('\n');  
  putchar('\n');  

  attachment = XMLgetNode(message, "attachment");
  for(i = 0; i < message->NumElements; i++) {  
    printf("  %s\n", XMLgetAttr(attachment, "filename"));
    attachment = attachment->NextElem;
  }

  con_gotoxy(1,23);
  printf("press any key");
  con_update();
  onecharmode();
  getchar();
}

