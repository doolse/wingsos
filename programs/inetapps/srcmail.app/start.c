// A Simple Mail Client. 

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

// The all important Globals. 

DOMElement * configxml; //the root element of the config xml element.

FILE *fp;             //THE Main Server connection.
char history[200];    //keeps track of most recent request to server
int max;              //maximum number of message headers to get for  the list
int currentserv = 0;  //Server selected when program started
char *server    = NULL;//Server name as text
int howmany     = 0;  //Number of messages at last connect time
int erased      = 0;  //1 = messages have been deleted this session
int sounds      = 0;  //1 = sounds on, 0 = sounds off. 
int attachments = 0;  //1 = Message Header has Content-Type: Multipart/Mixed;
int numofaddies = 0;  //Number of possible return addresses for reply.

char *sound1, *sound2, *sound3, *sound4, *sound5, *sound6;

char * buf      = NULL;
char * boundary = NULL;
int  size       = 0;

typedef struct {  //Structure of a Single List Line
  char deleted;
  int  mesnum;
  char from[30];
  char subj[41];
} mbuf;

typedef struct {  //Structure for a return address.
  char use;
  char *addy;
} addressST;

struct termios tio;
struct termios termsettings;

addressST * address;
addressST * tempreplylist = NULL;
mbuf * mbuffer = NULL;  //Buffer For Main List of parsed message headers

// *** FUNCTION DECLARATIONS ***

// LowLevel Functions
int  connect(int verbose);
int  reconnect();       
int  howmanymessages(); 
int  howmanynew();
int  decrementtempcount ();
int  maxnum();
int  setsounds();
int  fixreturn();
char * GetReturnAddy(int num);
char * extractfilename();
char * extractboundary(char * headbound);
char * getfilenames();


// Display Altering Functions
void drawongrid(int x, int y, char item);
char * gline();
int  refresh();         
int  playsound(int soundevent);

void memerror() {
  printf("Memory Allocation Error.\n");
  exit(1);
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

  printf("a small test\n");
  exit(1);
  //temp
  splashlogo = XMLgetNode(configxml, "xml/splashlogo");
  printf("%s", XMLgetAttr(splashlogo, "logo"));

//  splashlogo = XMLgetNode(configxml, "xml/splashlogo");

//  printf("%s", splashlogo->Node.Value);
}

void menuclr(){
  con_setbg(COL_LightBlue);
  con_setfg(COL_White);
  fflush(stdout);
}

void bodyclr(){
  con_setbg(COL_Blue);
  con_setfg(COL_White);
  con_update();
}

void headclr(){
  con_setbg(COL_Blue);
  con_setfg(COL_LightBlue);
  con_update();
}

int makeserverinbox(char * server) {
  char * path;
  char * tempstr = NULL;

  server[16] = 0;
 
  path = fpathname("data/servers/", getappdir(), 1);
  tempstr = (char *)malloc(strlen(path)+strlen(server)+2);
  if(tempstr == NULL)
    memerror();

  sprintf(tempstr, "%s%s", path, server);
  return(mkdir(tempstr, 0));
}

int addserver() {
  char * name,* address,* username,* password;
  char input = 'n';

  lineeditmode();

  while(!(input == 'y'||input == 'Y')) {
    printf("Display Name for the server: ");
    con_update();
    getline(&buf, &size, stdin);
    name = strdup(buf);
    name[strlen(name)-1] = 0;

    printf("Address of this Server: ");
    con_update();
    getline(&buf, &size, stdin);
    address = strdup(buf);
    address[strlen(address)-1] = 0;

    printf("Username for this Server: ");
    con_update();
    getline(&buf, &size, stdin);
    username = strdup(buf);
    username[strlen(username)-1] = 0;

    printf("Password for this Server: ");
    con_update();
    getline(&buf, &size, stdin);
    password = strdup(buf);
    password[strlen(password)-1] = 0;

    printf(" -- %s: Overview --\n", name);
    printf(" Address:  %s\n", address);
    printf(" Username: %s\n", username);
    printf(" Password: %s\n", password);
    putchar('\n');
    printf(" Correct?");
    con_update();
    onecharmode();
    input = getchar();
    lineeditmode();
    if(!(input == 'y'||input == 'Y')) {
      free(name);
      free(address);
      free(username);
      free(password);
    }
  }

  if(!makeserverinbox(address)) {
    printf("An error occurred trying to create the servers inbox\n");
    return(1);
  } else {
    
  }
  return(1);
}


// The Main Program Functions
int list();
int displaylist();
void drawlist(int startline, int endline);
int view(int messagenum, char * type);//type="view" or "download"
char * viewmodeheader();
int expunge();
int erase(int messagenum);
int deletefinal();
int reply(int messagei, char * type);//type="reply" or "forward"
int choosereturnaddy();
void drawreturnaddylist();
int compose();

// Download Related Functions
int dealwithmime(int messagei);
int dealwithmimetext();
int dealwithmimebin(char * filename);

void helptext() {
  printf("Mail\n");
  printf("    USAGE: mail [-s servernumber][-c][-w]\n");
  printf("           -c for configure, -w for mailwatch\n");
  exit(1);
}

// *** MAIN LOOP ***

void main(int argc, char *argv[]){
  int new;
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

  gettio(STDOUT_FILENO, &termsettings);  
  bodyclr();
  con_clrscr();
  con_update();

  path = fpathname("resources/mailconfig.xml", getappdir(), 1);
  configxml = XMLloadFile(path);

  drawlogo();

  printf("                           Initializing Settings...\n\n");

exit(1);
  setsounds();
  playsound(HELLO);

  gettio(STDOUT_FILENO, &tio);
  tio.MIN = 1;
  settio(STDOUT_FILENO, &tio);

  connect(1);
  printf("\n         Successfully Logged In. Gathering Mailbox Information...\n\n");

  howmanymessages();
  printf("         There are %d messages on this server.  ", howmany);
  new = howmanynew();
  if(new > 1) {
    playsound(NEWMAIL);
    printf("%d are new.\n", new); 
  } else if(new ==1) {
    playsound(NEWMAIL);
    printf("There is 1 NEW message.\n");
  } else {
    playsound(NONEWMAIL);
    printf("There are no NEW messages.\n");
  }

  list();        //builds the list for the first time. 
  displaylist(); //displays the list. Doesn't leave this function 
                 //until the user wants to quit.

  printf("Quitting and Logging out\n");
  deletefinal();

  playsound(GOODBYE);

  printf("\x1b[0m"); //reset the terminal.
  con_clrscr();
  con_update();

  fflush(fp);
  fprintf(fp, "QUIT\r\n");
  gline();
  fclose(fp);

  settio(STDOUT_FILENO, &termsettings);
}

void drawongrid(int x, int y, char item) {
  con_gotoxy(x, y);
  putchar(item);
  con_update();
}

char * gline(){
  fflush(fp);
  getline (&buf, &size, fp);
  return(strdup(buf));
}

int setsounds(){
  FILE * soundsrc;
  char * PATH = NULL;

  PATH     = fpathname("resources/sounds.rc", getappdir(), 1);
  soundsrc = fopen(PATH, "r");

  if(!soundsrc) {
    sounds = 0;
    return(0);
  } else {
    getline(&buf, &size, soundsrc);

    if(buf[0] == 'y')
      sounds = 1;
    else {
      sounds = 0;
      return(0);
    }

    getline(&buf, &size, soundsrc);
    sound1 = strdup(buf);
    sound1[strlen(sound1)-1] = 0;

    getline(&buf, &size, soundsrc);
    sound2 = strdup(buf);
    sound2[strlen(sound2)-1] = 0;

    getline(&buf, &size, soundsrc);
    sound3 = strdup(buf);
    sound3[strlen(sound3)-1] = 0;

    getline(&buf, &size, soundsrc);
    sound4 = strdup(buf);
    sound4[strlen(sound4)-1] = 0;

    getline(&buf, &size, soundsrc);
    sound5 = strdup(buf);
    sound5[strlen(sound5)-1] = 0;

    getline(&buf, &size, soundsrc);
    sound6 = strdup(buf);
    sound6[strlen(sound6)-1] = 0;

    fclose(soundsrc);
    return(1);
  }
}

int playsound(int soundevent) {
  char * tempstr = NULL;
  if (!sounds) 
    return(0);
  
  switch(soundevent) {
   case HELLO:
     tempstr = (char *)malloc(strlen("wavplay  2>/dev/null >/dev/null &  ")+strlen(sound1)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "wavplay %s 2>/dev/null >/dev/null &", sound1);
     system(tempstr);
   break;
   case NEWMAIL:
     tempstr = (char *)malloc(strlen("wavplay  2>/dev/null >/dev/null &  ")+strlen(sound2)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "wavplay %s 2>/dev/null >/dev/null &", sound2);
     system(tempstr);
   break;
   case NONEWMAIL:
     tempstr = (char *)malloc(strlen("wavplay  2>/dev/null >/dev/null &  ")+strlen(sound3)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "wavplay %s 2>/dev/null >/dev/null &", sound3);
     system(tempstr);
   break;
   case MAILSENT:
     tempstr = (char *)malloc(strlen("wavplay  2>/dev/null >/dev/null &  ")+strlen(sound4)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "wavplay %s 2>/dev/null >/dev/null &", sound4);
     system(tempstr);
   break;
   case REFRESH:
     tempstr = (char *)malloc(strlen("wavplay  2>/dev/null >/dev/null &  ")+strlen(sound5)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "wavplay %s 2>/dev/null >/dev/null &", sound5);
     system(tempstr);
   break;
   case GOODBYE:
     tempstr = (char *)malloc(strlen("wavplay  2>/dev/null >/dev/null &  ")+strlen(sound6)+1);
     if(!tempstr)memerror();
     sprintf(tempstr, "wavplay %s 2>/dev/null >/dev/null &", sound6);
     system(tempstr);
   break;
  }
  return(1);
}

int connect(int verbose){
  FILE *resource;
  char recon;
  char *user       = NULL;
  char *pass       = NULL;
  char *path       = NULL;
  char *tcpstr     = NULL;
  int  servchoicei = 0;
  int  servnumi    = 0;
  int  i           = 0; 

  path     = fpathname("resources/mail.rc", getappdir(), 1);
  resource = fopen(path, "r");

  if(!resource) {
    printf("Mail's Resource File wasn't found. We'll create one!\n");
    createmailrc();
    resource = fopen(path, "r");
  }

  getline(&buf, &size, resource);
  buf[strlen(buf)-1] = 0;
  servnumi = atoi(buf);

  if((servnumi > 1) && (currentserv == 0)){
   
    lineeditmode();

    printf("         Which Mailbox do you want to open? You have %d configured.\n", servnumi);

    getline(&buf, &size, stdin);
    buf[strlen(buf)-1] = 0;

    servchoicei = atoi(buf);
    currentserv = servchoicei;

  } else {
    if(currentserv != 0)
      servchoicei = currentserv;
    else {
      servchoicei = 1;
      currentserv = 1;   
    }
  }

  for(i = 1; i < currentserv; i++){
    getline(&buf, &size, resource);
    getline(&buf, &size, resource);
    getline(&buf, &size, resource);
  }

  getline(&buf, &size, resource);
  user = strdup(buf);
  user[strlen(user)-1] = 0;

  getline(&buf, &size, resource);
  pass = strdup(buf);
  pass[strlen(pass)-1] = 0;

  getline(&buf, &size, resource);
  server = strdup(buf);
  server[strlen(server)-1] = 0;

  fclose(resource);

  if(verbose)
    printf("                Connecting to server, %s...\n\n", server);

  tcpstr = (char *)malloc(strlen("/dev/tcp/:110"+strlen(server)+2));
  if(tcpstr == NULL)
    memerror();
  sprintf(tcpstr, "/dev/tcp/%s:110", server);
  fp = fopen(tcpstr, "r+");
  free(tcpstr);

  if(!fp){
    printf("%s", buf);
    printf("The server could not be connected to\n");
    exit(-1);
  }

  gline();

  fflush(fp);
  fprintf(fp, "USER %s\r\n", user);

  if(verbose)
    printf("Sent Username %s...\n", user);

  gline();
  fflush(fp);
  fprintf(fp, "PASS %s\r\n", pass);

  if(verbose){
    printf("Sent Password ;$%#&@*...\n");
    printf("Authenticating User...\n");
  }
  gline();

  if(buf[0] == '-'){
    printf("Error: Username and/or Password incorrect\n");
    printf("Make sure your UserName and Password are correct, and they are for the correct Server\n");
    printf("Do you want to Recreate the resource file?\n");

    onecharmode();

    if(getchar() == 'y')
      createmailrc();
    printf("You'll now need to restart Mail\n");
    exit(-1);
  }

  return(1);
}

int reconnect(){
  int howmanywere;

  howmanywere = howmany;
  howmanynew(); //if there are new messages we adjust the mailcount.dat

  fclose(fp);
  connect(0);
  fflush(fp);
  howmanymessages();

  if((howmanywere < howmany)||(erased != 0)){
    playsound(REFRESH);
    printf("\nMessage offset has changed.. Refreshing\n");
    list();     //rebuild the list from new connection.
    erased = 0; //reset erased to 0. 0 have been erased this connection.
    return(0);  //return 0 to whatever called reconnect.. to abort the action
  }

  // if no new messages, and non erased last time, message list doesn't
  // have to be rebuilt. We can send the history stored command to the 
  // function that called reconnect, and give it a one to keep going.
  
  fflush(fp);
  fprintf(fp, "%s", history);
  fflush(fp);
  return(1);
}

int refresh() {
  playsound(REFRESH);
  printf("Refreshing Mail List... Expunge Messages Flagged for Deletion?\n (if you Say 'n', Flagged Messages will be unflagged)\n");
  if(getchar() == 'y')
    expunge();
  else
    reconnect();
  return(1);
}

int howmanymessages(){
  fflush(fp);
  fprintf(fp, "LIST\r\n");
  fflush(fp);

  howmany = 0;

  getline(&buf, &size, fp);
  
  while(buf[0] != '.'){
    howmany++;
    getline(&buf, &size, fp);
    //printf("%s %d\n", buf, howmany);
  }

  howmany--;
  return(1);
}

int howmanynew(){
  FILE * tempcountfile;
  char * path    = NULL;  
  char * tempstr = NULL;
  int  newcount;

  tempstr = (char *)malloc(strlen("data/mailcount  .dat")+1);
  if(tempstr == NULL)
    memerror();
  sprintf(tempstr, "data/mailcount%d.dat", currentserv);
  path          = fpathname(tempstr, getappdir(), 1);
  tempcountfile = fopen(path, "r");
  free(tempstr);

  if(!tempcountfile)
    newcount = 0;
  else {
    getline(&buf, &size, tempcountfile);
    newcount = atoi(buf);
    fclose(tempcountfile);
  }

  tempcountfile = fopen(path, "w");

  if(!tempcountfile)
    printf("Error writing Temporary file containing Message Count\n");
  else { 
    fprintf(tempcountfile, "%d\n", howmany);
    fclose(tempcountfile);
  }
  return(howmany - newcount);
}

int maxnum(){
  FILE * resource;
  char * path  = NULL;
  int  serverI = 0;
  int  num     = 0;
  int  i       = 0;

  path     = fpathname("resources/mail.rc", getappdir(), 1);
  resource = fopen(path, "r");

  if(!resource){ 
    printf("You seem to no longer have a mail resource file. \n");
    createmailrc();
    resource = fopen(path, "r");
  }  

  //check how many servers are setup, then skip the server info. 

  getline(&buf, &size, resource);
  serverI = atoi(buf);
  for(i = 0; i < (serverI*3); i++)
    getline(&buf, &size, resource);

  getline(&buf, &size, resource);

  num = atoi(buf);
  max = num;
  return(0);    
}

int list() {
  char check;
  int  i           = 0;
  int  j           = 0;
  int  placeholder = 0;
  mbuf *mbufptr;

  mbuffer = NULL;
  maxnum();

  if(max > howmany)
    max = howmany;

  //printf("building list max = %d\n", max);

  mbuffer = (mbuf *)malloc( (sizeof(mbuf)) * (max));
  mbufptr = mbuffer;

  if(mbuffer == NULL)
    memerror();

  //printf("Getting Message Information. first number %d\n", howmany-max);
  //printf("Howmany equals, %d\n", howmany);

  for(i = 1; i<(max+1); i++){
    printf("-%d",i);
    fflush(stdout);
    mbufptr->mesnum = howmany-max+i;
    mbufptr->deleted = ' ';
    fflush(fp);
  
    //printf("TOP %d 0\n", howmany-max+i);

    fprintf(fp, "TOP %d 0\r\n", howmany-max+i);
    sprintf(history, "TOP %d 0\r\n", howmany-max+i);
    fflush(fp);
    if(-1 == getline(&buf, &size, fp)){
      reconnect();
      gline();
    }

    //printf("%s", buf);
    if(buf[0] == '-'){
      printf("Error Getting list\n");
      break;
    }
    do{
      getline(&buf, &size, fp);

      if(buf[0] == 'F'){
        placeholder = strlen(buf);
        buf[placeholder-2] = ' ';
        buf[placeholder-1] = ' ';
        for(j = placeholder; j < 35; j++) 
          buf[j] = ' ';
         
        buf[35] = 0;
        strcpy(mbufptr->from , (char *)buf+6);
      } else if(buf[0] == 'S' && buf[1] =='u'){

        placeholder = strlen(buf);
        buf[placeholder-2] = ' ';
        buf[placeholder-1] = ' ';
        for(j = placeholder; j < 49; j++) 
          buf[j] = ' ';
         
        buf[49] = 0;
        strcpy(mbufptr->subj , (char *)buf+9);
      }
    } while(buf[0] != '.');
    mbufptr++;
  } // ends the forloop that gets all message headers.

  return(1);
}

void drawlist(int startline, int endline) {
  int j;

    con_clrscr();
    bodyclr();

    con_gotoxy(1,0);
    printf("Current Server: %s",  server); //Printout what current server is

    con_gotoxy(49,0);
    printf("-Mail : DAC Productions 2002-\n");
    printf("------------- FROM: --------------------- SUBJECT: -----------------------------");
    con_update();

    con_setfg(COL_Yellow);
  
    //DRAW the LIST!!
   
    for(j = startline; j<endline; j++) 
      if (j < max) 
        printf("\n%c %4d %s %s", mbuffer[j].deleted, mbuffer[j].mesnum, mbuffer[j].from, mbuffer[j].subj);
        
    onecharmode();
    menuclr();

    con_gotoxy(0,23);
    printf(" +/-/Q (v)iew (f)orward (c)ompose (r)eply (d)ownload (e)rase (X)punge (R)efresh");
}

int displaylist() {
  int startline = 0;
  int endline   = 0;
  char option;
  int msgindex  = 0;
  int msgscrpos = 2;

  endline = startline + 21;

  drawlist(startline, endline);

  //place the  message arrow
  con_gotoxy(2,msgscrpos);
  putchar('>');
  con_gotoxy(0,24);
  con_update();

  do {

    attachments=0;
    fflush(stdin);
    fflush(stdout);
    option = getchar();

    switch(option) {

      case '+':
        if(msgscrpos > 2){
          con_gotoxy(2,msgscrpos);
          putchar(' ');
          msgscrpos--;
          con_gotoxy(2,msgscrpos);
          putchar('>');
          con_gotoxy(0,24);
          con_update();
          msgindex--;
        } else { 
          if(startline > 0){
            startline = startline - 21;
            endline   = startline + 21;
            drawlist(startline, endline);
            msgscrpos=22;
            con_gotoxy(2,msgscrpos);
            putchar('>');
            con_gotoxy(0,24);
            con_update();
            msgindex--;
          }
        }
      break;
      
      case '-':
        godown:
        if(msgscrpos < 22 && msgindex < max-1) {
          con_gotoxy(2,msgscrpos);
          putchar(' ');
          msgscrpos++;
          con_gotoxy(2,msgscrpos);
          putchar('>');
          con_gotoxy(0,24);
          con_update();
          msgindex++;
        } else {
          if (max > endline) {
            startline += 21;
            endline = startline + 21;
            drawlist(startline, endline);
            msgscrpos = 2;
            con_gotoxy(2,msgscrpos);
            putchar('>');
            con_gotoxy(0,24);
            con_update();
            msgindex++;
          }
        }
      break;

      case '\n':
        view(mbuffer[msgindex].mesnum, "view");
        drawlist(startline, endline);
        con_gotoxy(2,msgscrpos);
        putchar('>');
        con_gotoxy(0,24);
        con_update();
      break;
      
      case 'v':
        view(-1, "view");
        drawlist(startline, endline);
        con_gotoxy(2,msgscrpos);
        putchar('>');
        con_gotoxy(0,24);
        con_update();
      break;

      case 'd':
        view(mbuffer[msgindex].mesnum, "download");
        drawlist(startline, endline);
        con_gotoxy(2,msgscrpos);
        putchar('>');
        con_gotoxy(0,24);
        con_update();
      break;

      case 'e':
        if ( mbuffer[msgindex].deleted == 'E') {
          mbuffer[msgindex].deleted = ' ';
          con_gotoxy(1,msgscrpos);
          putchar(' ');
        } else {
          mbuffer[msgindex].deleted = 'E';
          con_gotoxy(1,msgscrpos);
          putchar('E');
        }
        con_gotoxy(0,24);
        con_update();
        goto godown;
      break;
   
      case 'r':
        reply(mbuffer[msgindex].mesnum, "reply");
        drawlist(startline, endline);
        con_gotoxy(2,msgscrpos);
        putchar('>');
        con_gotoxy(0,24);
        con_update();
      break;

      case 'f':
        reply(mbuffer[msgindex].mesnum, "forward");
        drawlist(startline, endline);
        con_gotoxy(2,msgscrpos);
        putchar('>');
        con_gotoxy(0,24);
        con_update();
      break;

      case 'c':
        compose();
        drawlist(startline, endline);
        con_gotoxy(2,msgscrpos);
        putchar('>');
        con_gotoxy(0,24);
        con_update();
      break;

      case 'X':
        expunge();
        drawlist(startline, endline);
        con_gotoxy(2,msgscrpos);
        putchar('>');
        con_gotoxy(0,24);
        con_update();
      break;

      case 'R':
        reconnect();
        drawlist(startline, endline);
        con_gotoxy(2,msgscrpos);
        putchar('>');
        con_gotoxy(0,24);
        con_update();
      break;
    }
  } while(option != 'Q');

  return(0);
}

int view(int messagenum, char * type){
  char option; 
  int  messagei = 0;
  int  skipped  = 0;
  int  lines    = 19;
  int  index    = 0;
  int  eom      = 0;
  FILE * download;
  char * filename = NULL;
  char * header;

  fflush(stdin);
  lineeditmode();

  if(messagenum == -1) {
    printf("\nWhich message # do you want to access? ");
    fflush(stdout);

    getline (&buf, &size, stdin); 
    buf[strlen(buf)-1] = 0;
    messagei = atoi(buf);
  } else {
    messagei = messagenum;
  }

  if(type == "download") {
    printf("What filename do you want to save this message as?\n");
    getline(&buf, &size, stdin); 
    filename = strdup(buf);
    filename[strlen(filename)-1] = 0;
  }

  con_clrscr();
  
  while(option != 's'){
    fflush(stdin);
    fflush(fp);
    eom = 0;

    if(type == "download") {
      download = fopen(filename, "w");
      if(download == NULL) {
        printf("Could not create file %s\n", filename);
        onecharmode();
        getchar();
        return(0);        
      }
      fprintf(fp, "RETR %d\r\n", messagei);
      sprintf(history, "RETR %d\r\n", messagei);    
    } else {
      fprintf(fp, "TOP %d %d\r\n", messagei, lines);
      sprintf(history, "TOP %d %d\r\n", messagei, lines);    
    }

    if((header = viewmodeheader()) == NULL)
      return(0);
    else {
      if(type == "download")
        fprintf(download, "%s", header);
      else
       printf("%s", header);
    }
  
    buf[0]  = 0 ;
    skipped = lines-19;
    index   = 0;

    bodyclr();       

    if(attachments) { //smart viewing

      while(!(buf[0] == '.' && buf[1] == '\r' && buf[2] == '\n')) {
        index++;
        getline (&buf, &size, fp);

        if(strstr(buf, boundary)) {
          getline(&buf, &size, fp);
          if(strstr(buf, "ontent"))
            if(! (strstr(buf, "text") || strstr(buf, "Text") || strstr(buf, "TEXT"))) {
              eom = 1;
            } else
             getline(&buf, &size, fp);
        }
 
        if(!eom) {
          if(type == "download") {
            fprintf(download, "%s", buf);
          } else {
            if(index>skipped)
              printf("%s", buf);
          }
        } else 
          index--;
      } 

    } else { //stupid viewing

      while(!(buf[0] == '.' && buf[1] == '\r' && buf[2] == '\n')) {
        index++;
        getline (&buf, &size, fp);
 
        if(type == "download")
          fprintf(download, "%s", buf);
        else {    
          if(index>skipped)
            printf("%s", buf);
        }
      } 

    }

    if(type == "download") {
      fclose(download);
      fflush(fp);
      return(0);
    }

    fflush(fp);
    onecharmode();

    menuclr();

    if(attachments) {
      if(eom)
        printf("(+), (s)top, (f)/(r)eply (d)ownload attached?  Line #%d Message #%d of %d\n", lines, messagei, howmany);
      else
        printf("(+/-), (s)top, (f)/(r)eply (d)ownload attached?  Line #%d Message #%d of %d\n", lines, messagei, howmany);
    } else {
      printf("(+/-), (s)top, (f)/(r)eply (n)ext (p)rev ? Line #%d Message #%d of %d\n", lines, messagei, howmany);
    }

    option = getchar();

    if(option == '+'){
      if(lines > 19) {
        lines = lines - 19;
        eom = 0;
      }
    } else if(option == '-'){
      if(!eom)
        lines = lines + 19;
    } else if(option == 'r'){
      reply(messagei, "reply");
    } else if(option == 'f'){
      reply(messagei, "forward");
    } else if((option =='d') && (attachments)) {
      dealwithmime(messagei);
    } else if(option == 'n') {
      view(messagei+1, "view");
      return(0);
    } else if(option == 'p') {
      view(messagei-1, "view");
      return(0);
    } else if(option == 'e') {
      erase(messagei);
    }
  }

  fflush(fp);
  return(0);
}

char * viewmodeheader(){
  char * header;
  char * tempptr;
  attachments = 0;

  fflush(fp);
  if(-1 == getline(&buf, &size, fp)){
    if(!(reconnect()))
      return(NULL);
    gline();
  }
  if(buf[0] == '-'){
    printf("Error Message doesn't exist\n");
    return(NULL);
  }
  headclr();

  header = strdup("");

  do{
    getline(&buf, &size, fp);

    if(!(strncmp(buf, "To:",  3))) {
      tempptr = header;
      header = (char *)malloc(strlen(header)+1+strlen(buf)+1);
      sprintf(header, "%s%s", tempptr, buf);
      free(tempptr);
    } else if(!(strncmp(buf, "From", 4))) {
      tempptr = header;
      header = (char *)malloc(strlen(header)+1+strlen(buf)+1);
      sprintf(header, "%s%s", tempptr, buf);
      free(tempptr);
    } else if(!(strncmp(buf, "Subj", 4))) {
      tempptr = header;
      header = (char *)malloc(strlen(header)+1+strlen(buf)+1);
      sprintf(header, "%s%s", tempptr, buf);
      free(tempptr);
    } else if(!(strncmp(buf, "Content-Type: multipart/mixed;", 30))) {
      attachments = 1;
    } else if(!(strncmp(buf, "Content-Type: MULTIPART/MIXED;", 30))) {
      attachments = 1;
    } 

    if(strstr(buf, "boundary")) {
      boundary = extractboundary(buf);
    }
  } while((strcmp(buf, "\r\n")));

  return(header);
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
  sprintf(history, "RETR %d\r\n", messagei);
  fflush(fp);

  if(-1 == getline(&buf, &size, fp)){
    if(!(reconnect()))
      return(0);
    gline();
  }
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

char * extractboundary(char * headbound){
  char * boundary = NULL;
  int  i          = 0;
  int  j          = 0;
  int  start      = 0;

  boundary = (char *)malloc(strlen(headbound)-strlen("boundary"));
  if(boundary == NULL)
    memerror();

  //extract a max of 20 chars for boundary from between the quotes        

  for(i = 0; j<20; i++) {
    if(start) {
      if(headbound[i] == '"'){
        boundary[j] = 0;
        j = 20;
      } else {
        boundary[j] = headbound[i];
        j++;
        boundary[j] = 0;
      }
    } else if(headbound[i] == '"') {
      start=1;
    }
  }      
  return(boundary);
}

int erase(int messagenum){

  if((howmany-messagenum) <= max)
    mbuffer[(max-(howmany-messagenum))-1].deleted = 'E';

  return(1);
}

int deletefinal() {
  int i = 0;

  fflush(fp);

  for(i = 0; i < max; i++){

    if(mbuffer[i].deleted == 'E'){
      fflush(fp);  
      //printf("Erasing message #%d From the Server\n", mbuffer[i].mesnum);
      fprintf(fp, "DELE %d\r\n", mbuffer[i].mesnum);
      //printf("i is %d, messagenum is %d\n", i, mbuffer[i].mesnum);

      erased++;

      sprintf(history, "DELE %d\r\n", mbuffer[i].mesnum);
      fflush(fp);

      if(-1 == getline(&buf, &size, fp)){
        if(!(reconnect()))
          printf("Messages were not deleted!\n");
        getline(&buf, &size, fp);
      }

//      printf("%s", buf);
      if(buf[0] == '-')
        printf("This message could not be deleted, Continuing to erase the others\n");
      else {
        //printf("Message Erase Successful!\n");
        decrementtempcount();
      }
    }
  }
  //printf("About to return\n\n");
  //printf("Messages Flagged For Deletion, Have been Expunged\n");
  return(0);
}

int expunge() {

  //printf("Expunging messages Flagged for Deletion\n");
  deletefinal();
  printf("\nRefreshing List\n");

  fflush(stdout);
  fflush(fp);
  fprintf(fp, "QUIT\r\n");
  gline();

  erased=1;
  reconnect();
  return(1);
}

int decrementtempcount (){
  FILE * counter;
  char * buf     = NULL;
  char * path    = NULL;
  char * tempstr = NULL;
  int  size      = 0;
  int  count     = 0;

  tempstr = (char *)malloc(strlen("data/mailcount.dat")+5);
  if(tempstr == NULL)
    memerror();
  sprintf(tempstr, "data/mailcount%d.dat", currentserv);
  path    = fpathname(tempstr, getappdir(), 1);
  counter = fopen(path, "r");
  free(tempstr);

  if(counter){
    getline(&buf, &size, counter);
    count = atoi(buf);
    count--;
    fclose(counter);
  } else {
    printf("Could not open mailcount%d.dat\n", currentserv);
    getchar();
    fflush(stdin);
  }

  counter = fopen(path, "w");
  if(!counter) {
    printf("Couldn't create mailcount%d.dat\n", currentserv);
    getchar();
    fflush(stdin);
    return(0);
  } else {
    fprintf(counter, "%d\n", count);
    fclose(counter);
  }
  return(0);
}
  
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
  sprintf(history, "TOP %d 0\r\n", messagei);
  fflush(fp);

  //Get the first line... this has either + or - in the first element
  if(getline(&buf, &size, fp) == EOF){
    if(!reconnect()) 
      return(0);
    getline(&buf, &size, fp);
  }

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

  bodyclr();
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

void drawreturnaddylist() {
  int i = 0;
  bodyclr();
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

        case '\n':
          for(i = 0; i<numofaddies; i++) {
            if(address[i].use == 'R') 
              gotoeditor = 1;
          }
        break;
/*
        case 's':
          return(0);
        break;
*/
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

  bodyclr();
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

int createmailrc() {
  FILE * fp;
  char * path = NULL;  
  int integer = 0;
  int i       = 0;

  path = fpathname("resources/mail.rc", getappdir(), 1);
  fp   = fopen(path, "w");

  if(!fp){
    printf("There was an error creating your mail resource file.\n");
    exit(-1);
  }

  printf("We'll create a resource file that stores information about your mail\n configuration. \n Just answer the following Questions... \n");
  printf("\n Howmany servers do you want to set up for? ");
  fflush(stdout);

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;

  integer = atoi(buf);

  fprintf(fp, "%d\n", integer);

  for(i = 0; i < integer; i++) {
    printf("What is the username for server #%d?\n", i+1);
    getline(&buf, &size, stdin);
    fprintf(fp, "%s", buf);

    printf("What is the password for server #%d?\n", i+1);
    getline(&buf, &size, stdin);
    fprintf(fp, "%s", buf);

    printf("And, what is the incoming mail address of server #%d?\n", i+1);
    getline(&buf, &size, stdin);
    fprintf(fp, "%s", buf);
   
    printf("\n");
  }

  printf("What is the maximum number of messages you want to see in the \n List?  (You can still access every message, but this saves time; it takes\n ~1 second per message to be added to the list. )\n ");

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;
  integer = atoi(buf);

  fprintf(fp, "%d\n", integer);
  fclose(fp);

  printf("OK, your mail resource file has been created.\n You can edit it manually, or run mail setup.\n The file is mail.app/resources/mail.rc \n");

  return(0);
}

