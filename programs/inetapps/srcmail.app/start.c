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
int sounds = 0;  // 1 = sounds on, 0 = sounds off. 

int writetowebpipe[2];
int readfromwebpipe[2];

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
int  fixreturn();
char * GetReturnAddy(int num);
char * extractfilename();
char * getfilenames();

int getnewmail(char * username, char * password, char * address, DOMElement * messages, char * serverpath);

int view(int fileref, char * serverpath);
msgline * parsehtmlcontent(msgline * prevline);
void givedatatoweb();
void viewattachedlist(DOMElement * message);

int reply(int messagei, char * type);//type="reply" or "forward"
int choosereturnaddy();
void drawreturnaddylist();
int compose();

int dealwithmime(int messagei);
int dealwithmimetext();
int dealwithmimebin(char * filename);

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
    getline(&buf, &size, stdin);
    password = strdup(buf);
    password[strlen(password)-1] = 0;

    putchar('\n');
    putchar('\n');
    printf("       --** Information Overview **--\n\n");
    printf("          Server Name: %s\n", name);
    printf("              Address: %s\n", address);
    printf("             Username: %s\n", username);
    printf("             Password: %s\n", password);

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
      case '\r':
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
      case '\r':
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

  tio.flags &= ~TF_ECHO;
  tio.MIN = 1;
  settio(STDOUT_FILENO, &tio);

  path = fpathname("resources/mailconfig.xml", getappdir(), 1);
  configxml = XMLloadFile(path);

  sounds = setupsounds();
  setupcolors();

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
    fprintf(outfile, "%s", fp);
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
  char * subject, * from, * date, * bstart, * name;
  char * bodytext, * headertext, * tempstr, * line, * lineptr;
  msgline * thisline, * prevline, * topofview;
  int charcount, eom, i, html;
  char c, input;
  
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
    }

    getline(&buf, &size, msgfile);      
  }

  //We now have subject, from, date and Possibly a boundary. 

  //IF no boundary, then no attachments. Put all of following text into
  //char * bodytext; 
  if(!boundary) {

    charcount = 0;
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
    fclose(msgfile);


  //If there IS a boundary... read the file character at a time, filling 
  //lines, but after each line is filled, check to see if it contains a 
  //boundary. if it does, remove that line from the linked list of lines. 
  //and deal with the next several lines as mimeheaders. if you encounter
  //a content-type header that is not text, close the file and move on.
  //if you find text that is HTML pipe it through web in a seperate thread.

  } else {

    charcount = 0;
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
          }
          getline(&buf, &size, msgfile);
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

void givedatatoweb() {
  FILE * output;  

  output = fdopen(writetowebpipe[1], "w");

  while(EOF != getline(&buf,&size, msgfile)) {
    if(strstr(buf, boundary))
      break;
    fprintf(stderr,"looping\n");
    con_update();
  }
  fprintf(stderr,"out of loop\nfound this!\n%s", buf);
  con_update();
}

msgline * parsehtmlcontent(msgline * prevline) {
  FILE * incoming;
  msgline * thisline;
  char * line, * lineptr;
  char c;
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
  prevline  = NULL;
  thisline  = NULL;
  line      = (char *)malloc(81);
  lineptr   = line;

  newThread(givedatatoweb, STACK_DFL, NULL);

  while(!eom) {
    charcount = 0;

    //Create a new line struct. setting the Prev and Next line pointers.
    thisline = (msgline *)malloc(sizeof(msgline));
    thisline->prevline = prevline;
    if(prevline)
      prevline->nextline = thisline;

    memset(line, 0, 81);
    lineptr = line;

    fprintf(stderr, "I'm in the first loop of main thread\n");
    con_update();
    exit(1);

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
    fprintf(stderr, "%s\n", thisline->line);
    con_update();
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

int dealwithmime(int messagei) {
  char * filename  = NULL;
  int  attachments = 0;
  int  i           = 0;
  int  text        = 0;
  int  binary      = 0;
  int  base64      = 0;

  fflush(fp);

  //Call the whole message... 

  fprintf(fp, "RETR %d\r\n", messagei);
  fflush(fp);

  if(buf[0] == '-'){
    printf("Error Message doesn't exist\n");
    return(0);
  }

  //printf("ripping through mail header...\n"); 
  do{
    getline(&buf, &size, fp);
  }while(strlen(buf) > 3);


  getline(&buf, &size, fp);  //First line of Message Content. 

  while(!(buf[0] == '.' && buf[1] == '\r' && buf[2] == '\n')) {

    text   = 0;
    binary = 0;
    base64 = 0;

    if(strstr(buf, boundary)) {
      getline(&buf, &size, fp);

      if((buf[0] == '.' && buf[1] == '\r' && buf[2] == '\n')) 
        break;

      while(strlen(buf) > 3) {
        if(strstr(buf, "Content-Type:")) {
          if(strstr(buf, "text") || strstr(buf, "Text") || strstr(buf, "TEXT")){
            text    = 1;
          } else {
            binary  = 1;
          }
        } 

        if(strstr(buf, "name")) {
          filename = extractfilename();  
        }
 
        if(strstr(buf, "Content-Transfer-Encoding:")) {
          if(strstr(buf, "base64") || strstr(buf, "BASE64") || strstr(buf, "Base64"))
            base64 = 1;
        }

        getline(&buf, &size, fp); 
      }

      if(text) {
        if(base64) {
          dealwithmimebin(filename);
        } else 
          dealwithmimetext();
      } else if(binary) {
        if(base64) {
          dealwithmimebin(filename);
        } else {
          printf("Found a Binary file, that is not base64 encoded.\n");
          printf("I can't decode it... skipping.\n");

          do{
            getline(&buf, &size, fp);
          }while(!strstr(buf, boundary));

        } 
      }
    } else 
      getline(&buf, &size, fp);
  }
  return(1);
}

int dealwithmimetext(){
  FILE * dlfile;

  printf("Text Segment Found.  Do you want to Save it? ");
  fflush(stdout);

  if(getchar() == 'y') {
    printf("\nWhat filename do you want to save it as?\n");
  
    fflush(stdin);
    lineeditmode();

    getline(&buf, &size, stdin);
    buf[strlen(buf)-1]=0;

    fflush(stdin);
    onecharmode();

    dlfile = fopen(buf, "w");
    if(!dlfile) {
      printf("Error: The file could not be created.\n");
      printf("Press A Key.\n");
      getchar();
      getline(&buf, &size, fp);
      return(0);
    }

    getline(&buf, &size, fp);

    while(!strstr(buf, boundary)) {
      fprintf(dlfile, "%s", buf);
      getline(&buf, &size, fp);
    }
   
    fflush(dlfile);
    fclose(dlfile);   
    printf("Text Segment Saved. Press A Key.\n");
    getchar();
  } else {
    do{
      getline(&buf, &size, fp);
    }while(!strstr(buf, boundary));
  }
  return(1);
}

int dealwithmimebin(char * filename){
  FILE * dlfile;
  char * path     = NULL;
  char * encodestr= NULL;
  int  i          = 0;

  printf("\nBinary File Found. '%s', Do you want to Download it? ", filename);
  fflush(stdout);
  if(getchar() == 'y') {

    path   = fpathname("data/temp", getappdir(), 1);
    dlfile = fopen(path, "w");

    if(!dlfile) {
      printf("Could not create mimefile. Press return.\n");
      getchar();
      getline(&buf, &size, fp);
      return(0);
    }

    printf("Saving...\n");

    i = fgetc(fp);

    while((i != '') && (i != '-')) {
      fprintf(dlfile, "%c", i);
      i = fgetc(fp);
    }

    getline(&buf, &size, fp);

    fflush(dlfile);
    fclose(dlfile);

    encodestr = (char *)malloc(strlen("cat  |base64 d >")+strlen(path)+strlen(filename)+4);
    if(encodestr == NULL)
      memerror();
    sprintf(encodestr, "cat %s |base64 d >%s", path, filename);
    system(encodestr);
    free(encodestr);

    printf("%s successfully downloaded. Press Any Key.\n", filename);
    getchar();
  } else {
    do{
      getline(&buf, &size, fp);
    }while(!strstr(buf, boundary));
  }
  return(1);
}

char * extractfilename(){
  char * filename = NULL;
  char * newstr   = NULL;
  int  j          = 0;
  int  k          = 0;
  int  start      = 0;

  filename = (char *)malloc(17);
  if(filename == NULL)
    memerror();

  //extract a max of 16 chars for filename from between the quotes        
  if(newstr = strstr(buf, "name=")) {

    if(newstr = strstr(buf, "name=\""))
      newstr += 6;
    else if(newstr = strstr(buf, "name= "))
      newstr += 7;
    else
      newstr += 5;

  } else if(newstr = strstr(buf, "name =")) {

    if(newstr = strstr(buf, "name = \""))
      newstr += 8;
    else if(newstr = strstr(buf, "name =\""))
      newstr += 7;
    else 
      newstr += 6;
  }

  j = 0;

  for(k = 0; k < 17; k++) {
    if((newstr[k] == '"') || (newstr[k] == '\r') || (newstr[k] == '\n')) 
      k = 17;
    else {
      filename[j] = newstr[k];
      j++;
    }
  }

  filename[j] = 0;

  return(filename);
}

/*
int reply(int messagei, char * type){
  FILE * tempfile;
  char * subject;
  char * newsubject;
  char * path = NULL;
  int  i      = 0;
  int  j      = 0;
  int  eot    = 0;
  char * boundary = NULL;
  char * tempptr;
  char * qsendstr = NULL;
  char * retraddy = NULL;
  char * ccstring = NULL;

  numofaddies = 0; //Global 

  //Request the Message Header.
  fflush(fp);
  fprintf(fp,      "TOP %d 0\r\n", messagei);
  fflush(fp);

  //Get the first line... this has either + or - in the first element

  //if the first element is - then the pop3 request failed. 
  if(buf[0] == '-')
   return(0);

  //count the number of addresses in the header and check for a boundary
  do {
    getline(&buf, &size, fp);

    if(strstr(buf, "boundary"))
      boundary = strdup(buf);

    if(strstr(buf, "@"))
      numofaddies++;

  } while(strlen(buf) > 2);

  if(boundary != NULL) {
    tempptr = boundary;
    boundary = extractboundary(boundary);
    free(tempptr);
  }

  if((address = (addressST *)malloc( sizeof(addressST) * numofaddies)) == NULL)
    memerror();

  //Request the Whole Message
  fflush(fp);
  fprintf(fp, "RETR %d\r\n", messagei);
  fflush(fp);

  getline(&buf, &size, fp);

  //if the first element is - then the pop3 request failed. 
  if(buf[0] == '-')
   return(0);

  i = 0;

  while(strlen(buf) > 2) {

    //convert windows line endings to Wings/Linux line endings.

    if(buf[strlen(buf)-2] == '\r')
      buf[strlen(buf)-2] = 0;

    else if(buf[strlen(buf)-1] == '\n') 
      buf[strlen(buf)-1] = 0;


    if(strstr(buf, "@") && i < numofaddies) {

      address[i].addy = strdup(buf);
      i++;

    } else if(strstr(buf, "ubject")) {

      subject = strdup(buf);

    }
    getline(&buf, &size, fp);
  } 

  if(numofaddies) 
    fixreturn();

  choosereturnaddy();
  //if(!choosereturnaddy()) 
    //return(0);

  con_clrscr();

  // Fix the Subject line...
  newsubject = subject;
  newsubject += 9;

  printf("Subject: %s\n\n", newsubject);
  printf("Modify the subject line? ");
  fflush(stdout);

  onecharmode();

  if(getchar() == 'y') {

    lineeditmode();    

    printf("\n");
    printf("Enter New Subject: ");
    fflush(stdout);
    getline(&buf, &size, stdin);
    free(subject);
    subject = strdup(buf);
    newsubject = subject;
  }
    
  // Create Body Text... Quoted if type = "reply"

  path     = fpathname("data/reply.tmp", getappdir(), 1);
  tempfile = fopen(path, "w");
  if(!tempfile)
    return(0);

  getline(&buf, &size, fp);
  while(buf[0] != '.'){
    if(buf[strlen(buf)-2] == '\r'){
      buf[strlen(buf)-2] = '\n';
      buf[strlen(buf)-1] = 0;
    }

    if(boundary != NULL) { 
      if(strstr(buf, boundary)) {
        getline(&buf, &size, fp);
        if(!(strstr(buf, "TEXT") || strstr(buf, "text")))
          eot = 1;
      }
    }

    if(!eot) {
      if(type == "reply")
        fprintf(tempfile, ">%s", buf);
      else
        fprintf(tempfile, "%s", buf);
    }
    getline(&buf, &size, fp);
  }
  fclose(tempfile);

  onecharmode();

//  if(type == "reply") {
    gettio(STDOUT_FILENO, &tio);  
    spawnlp(S_WAIT, "ned", path, NULL);
    settio(STDOUT_FILENO, &tio);
//  }

  setcolors(messagefg_col, messagebg_col);
  con_clrscr();
 
  if(type == "reply")
    printf("Would you like to send this message? ");
  else
    printf("Would you like to Forward this message? ");

  fflush(stdout);  

  if(getchar() == 'y'){

    j = 0;
    for(i = 0; i<numofaddies; i++){
      if(address[i].use == 'R')
        j++;
    }

    printf("Sending with QuickSend.\n");
    if(j < 2) {
      retraddy = GetReturnAddy(0);
      qsendstr = (char *)malloc(strlen("qsend -v -s '' -t  -m ")+1+strlen(newsubject)+1+strlen(retraddy)+1+strlen(path)+2);
      if(qsendstr == NULL)
        memerror();
      sprintf(qsendstr, "qsend -v -s \"%s\" -t %s -m %s", newsubject, retraddy, path);
    } else {
      retraddy = GetReturnAddy(0);
      ccstring = GetReturnAddy(j-1);      
      qsendstr = (char *)malloc(strlen("qsend -v -s '' -t  -C  -m ")+1+strlen(newsubject)+1+strlen(retraddy)+1+strlen(ccstring)+1+strlen(path)+2);
      if(qsendstr == NULL)
        memerror();
      sprintf(qsendstr, "qsend -v -s \"%s\" -t %s -C %s -m %s", newsubject, retraddy, ccstring, path);
    }
    system(qsendstr);

    free(qsendstr);

    playsound(MAILSENT);
  }

  printf("\nWould you like to save this message? ");
  fflush(stdout);

  if(getchar()== 'y'){

    lineeditmode();

    printf("\nWhat filename do you want for the saved message?\n");

    getline(&buf, &size, stdin);
    buf[strlen(buf)-1] = 0;

    spawnlp(S_WAIT, "mv" , path, buf, NULL);

  } else {
    spawnlp(S_WAIT, "rm", path, NULL);
  }

  free(subject);
  for(i = 0; i < numofaddies; i++) 
    free(address[i].addy);
  free(address);

  return(1);
}

*/

/*
char * GetReturnAddy(int num) {
  int  first = 1;
  int  i;
  char *ccstring = NULL;
  char *tempptr  = NULL;

  //if num is 0, it returns the first Address with an R flag set.

  //if num is 1 it returns addresses whose R flag is
  //set, skipping the first one found, and seperating the returned addies
  //with commas. These are for the Carbon Copy String given to qsend.

  if(num == 0) {
    for(i = 0; i<numofaddies; i++)
      if(address[i].use == 'R')
        return(address[i].addy);
  } else if(num != 0) {

    ccstring = strdup("");

    for(i = 0; i<numofaddies; i++) {
      if(address[i].use == 'R') {
        if(first) {
          first = 0;
        } else {
          tempptr = ccstring;
          ccstring = (char *)malloc(strlen(tempptr)+1+strlen(address[i].addy)+2);

          if(ccstring == NULL)
            memerror();

          sprintf(ccstring, "%s%s,", tempptr, address[i].addy);
          free(tempptr);
        }
      }
    }
    //strip the trailing comma.
    ccstring[strlen(ccstring)-1] = 0;
    return(ccstring);
  }
  return("gregnacu@kingston.net");
}

int fixreturn() {
  char * newstring;
  int i = 0;
  int j = 0;
  int k = 0;
  addressST *addr = address; //addr initialized to address[0];
  unsigned int lenofaddy;
  char * addy;

  //forloop through every address, extracting the proper part from each.

  for(i = 0;i<numofaddies;i++) {

    addy      = addr->addy;
    lenofaddy = strlen(addy);

    // Remove Spaces
    
    if((newstring = (char *)malloc(lenofaddy+1)) == NULL)
      memerror();

    for(k = 0; k < lenofaddy; k++) {
      if(addy[k] != ' ') {
        newstring[j] = addy[k];
        j++;
      }
    }
    newstring[j] = 0;
      
    free(addr->addy);
    addr->addy = newstring;
    // -----------------------------------

    addy = newstring;

    if((newstring = (char *)malloc(strlen(addy)+1)) == NULL)
      memerror();

    if(strstr(addy, "<") && strstr(addy, ">")) {
      //Extract the address from between the < and >

      for(k = 0; k < strlen(addy); k++) {
        if(addy[k] == '<') {
          j = k+1;
          while((addy[j] != '>')&&(addy[j] != 0)){
            newstring[j-k-1] = addy[j];
            j++;
          }
          newstring[j-k-1] = 0;
        }
      }

      free(addr->addy);
      addr->addy = newstring;

    } else if(strstr(addy, "(") && strstr(addy, ")")) {
      //Extract the address from between the ( and )

      for(k = 0; k < strlen(addy); k++) {
        if(addy[k] == '(') {
          j = k+1;
          while((addy[j] != ')')&&(addy[j] != 0)){
            newstring[j-k-1] = addy[j];
            j++;
          }
          newstring[j-k-1] = 0;
        }
      }

      free(addr->addy);
      addr->addy = newstring;

    } else if(strstr(addy, ":") && strstr(addy, ";")) {
      //Extract the address from between the : and ;

      for(k = 0; k < strlen(addy); k++) {
        if(addy[k] == ':') {
          j = k+1;
          while((addy[j] != ';')&&(addy[j] != 0)){
            newstring[j-k-1] = addy[j];
            j++;
          }
          newstring[j-k-1] = 0;
        }
      }

      free(addr->addy);
      addr->addy = newstring;

    } else if(strstr(addy, ":")) {
      //Extract the address from after the : to the end

      for(k = 0; k < strlen(addy); k++) {
        if(addy[k] == ':') {
          j = k+1;
          while(addy[j] != 0){
            newstring[j-k-1] = addy[j];
            j++;
          }
          newstring[j-k-1] = 0;
        }
      }

      free(addr->addy);
      addr->addy = newstring;

    } else {  
      //Otherwise... the address slips through unchanged.
      free(newstring);
    }
    ++addr;
    j = 0;
  }

  return(1);
}

*/

/*
void drawreturnaddylist() {
  int i = 0;
  con_clrscr();

  onecharmode();

  printf("\n");

  for(i = 0; i < numofaddies; i++) {
    address[i].use = '-';
    printf("   - %s\n", address[i].addy);
  }

  con_gotoxy(0,22);
//  printf("  +/-/[return]  (s)top (t)ake address (r)eply flag toggle (a)dd address\n");
  printf("  +/-/[return]  (t)ake address (r)eply flag toggle (a)dd address\n");

  //first draw the arrow beside the first list item.
  //Then move the cursor to home. we know where it was with arrowpos

  con_gotoxy(1,1);
  putchar('>');
  con_gotoxy(0, 23);
  con_update();
}

int choosereturnaddy() {
  int  currentpos = 0;
  int  arrowpos   = 1;
  int  gotoeditor = 0;
  int  i          = 0;
  char choice     = '-';
  char * tempstr  = NULL;

  drawreturnaddylist();

    while(!gotoeditor) {
      choice = getchar();

      switch(choice) {

        case '\r':
          for(i = 0; i<numofaddies; i++) {
            if(address[i].use == 'R') 
              gotoeditor = 1;
          }
        break;

//        case 's':
//          return(0);
//        break;

        case '+':
          if(currentpos == 0)
            break;
          currentpos--;
          //erase arrow
          con_gotoxy(1,arrowpos);
          putchar(' ');
          arrowpos--;
          con_gotoxy(1,arrowpos);
          putchar('>');       
          con_gotoxy(0,24);
          con_update();
        break;

        case '-':
          if(currentpos == numofaddies-1)
            break;
          currentpos++;
          //erase arrow
          con_gotoxy(1,arrowpos);
          putchar(' ');
          arrowpos++;
          con_gotoxy(1,arrowpos);
          putchar('>');       
          con_gotoxy(0,24);
          con_update();
        break;

        case 'r':
          if(address[currentpos].use == 'R') {
            address[currentpos].use = '-';
            con_gotoxy(3,arrowpos);
            putchar('-');
            con_gotoxy(0,24);
            con_update();
          } else {
            address[currentpos].use = 'R';
            con_gotoxy(3,arrowpos);
            putchar('*');
            con_gotoxy(0,24);
            con_update();
          }

          if(currentpos == numofaddies-1)
            break;
          currentpos++;
          //erase arrow
          con_gotoxy(1,arrowpos);
          putchar(' ');
          arrowpos++;
          con_gotoxy(1,arrowpos);
          putchar('>');
          con_gotoxy(0,24);
          con_update();

        break;

        case 'a':
          lineeditmode();
          printf(" Add address or nick to Options List:\n");
          getline(&buf, &size, stdin);

          //The User has added a new address to the list. So, 
          //Allocate space for a new list, plus one more...
          //free the old list, and move the pointers. 
        
          if(buf[0] != '\n') {
            if((tempreplylist = (addressST *)malloc(sizeof(addressST)*(numofaddies+1))) == NULL)
              memerror();

            memcpy(tempreplylist, address, sizeof(addressST)*numofaddies);

            free(address);
            address = tempreplylist;            

            buf[strlen(buf)-1] = 0;
            address[numofaddies].addy = buf;
            buf = NULL;            
            size = 0;            

            numofaddies++;
          }

          drawreturnaddylist();
          arrowpos = 2;
          currentpos = 0;
        break;

        case 't':
          lineeditmode();
          printf("What nick do you want to make this address?\n");
          getline(&buf, &size, stdin);
          if(buf[0] != '\n') {
            if(buf[strlen(buf)-1] == '\n')
              buf[strlen(buf)-1] = 0;
            tempstr = (char *)malloc(strlen("echo   >> /wings/programs/net/qsend.app/resources/nicks.rc")+1+strlen(buf)+1+strlen(address[currentpos].addy)+2);
            if(tempstr == NULL)
              memerror();
            sprintf(tempstr, "echo %s %s >> /wings/programs/net/qsend.app/resources/nicks.rc", buf, address[currentpos].addy); 
            system(tempstr);
            free(tempstr);
          }
          
          drawreturnaddylist();
          arrowpos = 2;
          currentpos = 0;
        break;
      }
    }

  return(1);
}

*/

int compose(){
  FILE * tempfile;
  char * sendaddress = NULL;
  char * subject = NULL;
  char * attach  = NULL;
  char * path    = NULL;
  char * qsendstr= NULL;
  char * filename= NULL;

  fflush(stdin);
  lineeditmode();

  printf("\nTo (address/nick): ");
  fflush(stdout);

  getline(&buf, &size, stdin);
  sendaddress = strdup(buf);
  sendaddress[strlen(sendaddress)-1] = 0;

  printf("Subject: ");
  fflush(stdout);

  getline(&buf, &size, stdin);
  subject = strdup(buf);  
  subject[strlen(subject)-1] = 0;

  fflush(stdin);
  onecharmode();

  printf("Do you want to attach a file or files? (y/n) ");
  fflush(stdout);

  if(getchar() == 'y') {

    printf("\nAttachments: (ie /path/path/filename.jpg,/path/filename.wav,...)\n");

    fflush(stdin);
    lineeditmode();

    getline(&buf, &size, stdin);  
    attach = strdup(buf);  
    attach[strlen(attach)-1] = 0;

    fflush(stdin);
    onecharmode();
  }

  path     = fpathname("data/compose.tmp", getappdir(), 1);
  tempfile = fopen(path, "w");
  if(!tempfile){
    printf("Couldn't open local temp file for Composing.\nPress a Key to return to the list\n");
    getchar();
    return(0);
  }
  fclose(tempfile);

  gettio(STDOUT_FILENO, &tio);

  spawnlp(S_WAIT, "ned", path, NULL);

  settio(STDOUT_FILENO, &tio);

  con_clrscr();
  printf("Would you like to send this message? ");
  fflush(stdout);

  if(getchar()=='y'){
    printf("working\n");

    if(attach) {
      qsendstr = (char *)malloc(strlen("qsend -v -s '' -t  -a  -m ")+1+strlen(subject)+1+strlen(sendaddress)+1+strlen(attach)+1+strlen(path)+2);
      if(qsendstr == NULL)
        memerror();
      sprintf(qsendstr, "qsend -v -s \"%s\" -t %s -a %s -m %s", subject, sendaddress, attach, path);	
      system(qsendstr);
      free(qsendstr);
    } else {
      qsendstr = (char *)malloc(strlen("qsend -v -s '' -t  -m ")+1+strlen(subject)+1+strlen(sendaddress)+1+strlen(path)+2);
      if(qsendstr == NULL)
        memerror();
      sprintf(qsendstr, "qsend -v -s \"%s\" -t %s -m %s", subject, sendaddress, path);
      system(qsendstr);
      free(qsendstr);
    }
    playsound(MAILSENT);
  }
  
  printf("would you like to save this message? ");
  fflush(stdout);

  if(getchar()=='y'){

    fflush(stdin);
    lineeditmode();

    printf("What filename do you want for the saved message?\n");

    getline(&buf, &size, stdin);
    filename = strdup(buf);
    filename[strlen(filename)-1] = 0;

    spawnlp(S_WAIT, "mv" , path, filename, NULL);
  } else {
    spawnlp(S_WAIT, "rm", path, NULL);
  }

  fflush(stdin);
  onecharmode();

  return(1);
}

