//Mail v1.5 for WiNGs

// A Simple Mail Client. 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>
#include <console.h>
#include <termio.h>
extern char *getappdir();

// Sound Event Defines
#define HELLO     1
#define NEWMAIL   2
#define NONEWMAIL 3
#define MAILSENT  4
#define REFRESH   5
#define GOODBYE   6

// The all important Globals. 

FILE *fp;             //THE Main Server connection.
char history[80];     //keeps track of most recent resquest to server
int max;              //maximum number of message headers to get for  the list
int currentserv = 0;  //Server selected when program started
char *server    = NULL;//Server name as text
int howmany     = 0;  //Number of messages at last connect time
int erased      = 0;  //1 = messages have been deleted this session
int sounds      = 0;  //1 = sounds on, 0 = sounds off. 
int attachments = 0;  //1 = Message Header has Content-Type: Multipart/Mixed;
int numofaddies = 0;  //Number of possible return addresses for reply.

char sound1[255];
char sound2[255];
char sound3[255];
char sound4[255];
char sound5[255];
char sound6[255];

char buffer[100];
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

// Display Altering Functions
void drawongrid(int x, int y, char item);
void DrawLogo();
char * gline();
int  refresh();         
void refreshscreen();
void menuclr();
void headclr();
void bodyclr();
void memerror();
int  playsound(int soundevent);

// Setup Function
int  createmailrc();

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

// *** MAIN LOOP ***

void main(int argc, char *argv[]){
  int new;
  char * path = NULL;

  if (argc > 1){
   if (argv[1][0] == 'w') {
      if(argc == 4){
        path = fpathname("mailwatch", getappdir(), 1);
        sprintf(buffer, "%s %s %s &", path, argv[2], argv[3]);
      } else if(argc == 3){
        path = fpathname("mailwatch", getappdir(), 1);
        sprintf(buffer, "%s %s &", path, argv[2]);
      } else {
        path = fpathname("mailwatch", getappdir(), 1);
        sprintf(buffer, "%s 1 &", path);
      }
      system(buffer);
      exit(1);
    } else if (argv[1][0] == 's') {
      createmailrc();
    } else
      currentserv = atoi(argv[1]);
  }

  gettio(STDOUT_FILENO, &termsettings);  
  bodyclr();
  refreshscreen();

  DrawLogo();

  printf("                           Initializing Settings...\n\n");

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

  list();  //builds the list for the first time. 
  displaylist(); //displays the list. Doesn't leave this function 
                 // until the user wants to quit.

  printf("Quitting and Logging out\n");
  deletefinal();

  playsound(GOODBYE);

  printf("\x1b[0m"); //Change the background color to black? not sure.
  printf("\x1b[H");  //Move cursor to home position
  printf("\x1b[2J"); //Clear the screen/reset the console

  fflush(stdout); //flush the control sequences to the console

  fflush(fp);
  fprintf(fp, "QUIT\r\n");
  gline();
  fclose(fp);

  settio(STDOUT_FILENO, &termsettings);
}

void drawongrid(int x, int y, char item) {
  printf("\x1b[%d;%dH%c", y, x, item);
}

void DrawLogo() {
  char * path = NULL;
  FILE *resource;

  path     = fpathname("resources/splash.logo", getappdir(), 1);
  resource = fopen(path, "r");

  if(resource) {
    while(getline(&buf, &size, resource) != EOF) {
      printf("%s", buf); 
    }
    fclose(resource);
  }
}

char * gline(){
  fflush(fp);
  getline (&buf, &size, fp);
  return(strdup(buf));
}

void memerror() {
  printf("Memory Allocation Error.\n");
  exit(1);
}

void refreshscreen(){
  printf("\x1b[H");
  printf("\x1b[2J");
  fflush(stdout);
}

void menuclr(){
  printf("\x1b[1;44m"); //lt blue background
  printf("\x1b[1;37m"); //white forground
  fflush(stdout);
}

void bodyclr(){
  printf("\x1b[1;44m");
  printf("\x1b[37m");
  fflush(stdout);
}

void headclr(){
  printf("\x1b[1;44m");
  printf("\x1b[34m");
  fflush(stdout);
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
    buf[strlen(buf)-1] = 0;
    strcpy(sound1, buf);

    getline(&buf, &size, soundsrc);
    buf[strlen(buf)-1] = 0;
    strcpy(sound2, buf);

    getline(&buf, &size, soundsrc);
    buf[strlen(buf)-1] = 0;
    strcpy(sound3, buf);

    getline(&buf, &size, soundsrc);
    buf[strlen(buf)-1] = 0;
    strcpy(sound4, buf);

    getline(&buf, &size, soundsrc);
    buf[strlen(buf)-1] = 0;
    strcpy(sound5, buf);

    getline(&buf, &size, soundsrc);
    buf[strlen(buf)-1] = 0;
    strcpy(sound6, buf);

    fclose(soundsrc);
    return(1);
  }
}

int playsound(int soundevent) {
  if (!sounds) 
    return(0);
  
  switch(soundevent) {
   case HELLO:
     sprintf(buf, "wavplay %s 2>/dev/null >/dev/null &", sound1);
     system(buf);
     break;
   case NEWMAIL:
     sprintf(buf, "wavplay %s 2>/dev/null >/dev/null &", sound2);
     system(buf);
     break;
   case NONEWMAIL:
     sprintf(buf, "wavplay %s 2>/dev/null >/dev/null &", sound3);
     system(buf);
     break;
   case MAILSENT:
     sprintf(buf, "wavplay %s 2>/dev/null >/dev/null &", sound4);
     system(buf);
     break;
   case REFRESH:
     sprintf(buf, "wavplay %s 2>/dev/null >/dev/null &", sound5);
     system(buf);
     break;
   case GOODBYE:
     sprintf(buf, "wavplay %s 2>/dev/null >/dev/null &", sound6);
     system(buf);
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

    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

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

  sprintf(buffer, "/dev/tcp/%s:110", server);
  fp = fopen(buffer, "r+");

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

    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

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
  char * path = NULL;  
  int  newcount;

  sprintf(buffer, "data/mailcount%d.dat", currentserv);
  path          = fpathname(buffer, getappdir(), 1);
  tempcountfile = fopen(path, "r");

  if(!tempcountfile)
    newcount = 0;
  else {
    getline(&buf, &size, tempcountfile);
    newcount = atoi(strdup(buf));
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

    refreshscreen();
    bodyclr();

    //printf("Displaying list Max = %d\n", max);
    //printf("startline = %d\n", startline);
    //printf("endline = %d\n", endline);

    printf("Current Server: %s",  server); //Printout what current server is
    printf("\x1b[1;50H");
    fflush(stdout);
    printf("-Mail : DAC Productions 2002-\n");
    printf("------------- FROM: --------------------- SUBJECT: -----------------------------");

    printf("\x1b[33m");
  
    //DRAW the LIST!!
   
    for(j = startline; j<endline; j++) 
      if (j < max) 
        printf("\n%c %4d %s %s", mbuffer[j].deleted, mbuffer[j].mesnum, mbuffer[j].from, mbuffer[j].subj);
        

    tio.flags &= ~TF_ICANON;
    settio(STDOUT_FILENO, &tio);
    menuclr();

    printf("\x1b[24;0H +/-/Q (v)iew (f)orward (c)ompose (r)eply (d)ownload (e)rase (X)punge (R)efresh");
}

int displaylist() {
  int startline = 0;
  int endline   = 0;
  char option;
  int msgindex  = 0;
  int msgscrpos = 3;

  endline = startline + 21;

  drawlist(startline, endline);

  //place the  message arrow
  printf("\x1b[%d;2H>", msgscrpos);
  printf("\x1b[25;1H");
  fflush(stdout);

  do {

    attachments=0;
    fflush(stdin);
    fflush(stdout);
    option = getchar();

    switch(option) {

      case '+':
        if(msgscrpos > 3){
          printf("\x1b[%d;2H ", msgscrpos);
          msgscrpos--;
          printf("\x1b[%d;2H>", msgscrpos);
          printf("\x1b[25;1H");
          fflush(stdout);
          msgindex--;
        } else { 
          if(startline > 0){
            startline = startline - 21;
            endline   = startline + 21;
            drawlist(startline, endline);
            msgscrpos=23;
            printf("\x1b[%d;2H>", msgscrpos);
            printf("\x1b[25;1H");
            fflush(stdout);
            msgindex--;
          }
        }
      break;
      
      case '-':
        godown:
        if(msgscrpos < 23 && msgindex < max-1) {
          printf("\x1b[%d;2H ", msgscrpos);        
          msgscrpos++;
          printf("\x1b[%d;2H>", msgscrpos);        
          printf("\x1b[25;1H");
          fflush(stdout);
          msgindex++;
        } else {
          if (max > endline) {
            startline += 21;
            endline = startline + 21;
            drawlist(startline, endline);
            msgscrpos = 3;
            printf("\x1b[%d;2H>", msgscrpos);        
            printf("\x1b[25;1H");
            fflush(stdout);
            msgindex++;
          }
        }
      break;

      case '\n':
        view(mbuffer[msgindex].mesnum, "view");
        drawlist(startline, endline);
        printf("\x1b[%d;2H>", msgscrpos);        
        printf("\x1b[25;1H");
        fflush(stdout);
      break;
      
      case 'v':
        view(-1, "view");
        drawlist(startline, endline);
        printf("\x1b[%d;2H>", msgscrpos);        
        printf("\x1b[25;1H");
        fflush(stdout);
      break;

      case 'd':
        view(mbuffer[msgindex].mesnum, "download");
        drawlist(startline, endline);
        printf("\x1b[%d;2H>", msgscrpos);        
        printf("\x1b[25;1H");
        fflush(stdout);
      break;

      case 'e':
        if ( mbuffer[msgindex].deleted == 'E') {
          mbuffer[msgindex].deleted = ' ';
          printf("\x1b[%d;1H ", msgscrpos);
        } else {
          mbuffer[msgindex].deleted = 'E';
          printf("\x1b[%d;1HE", msgscrpos);
        }
        printf("\x1b[25;1H");
        fflush(stdout);
        goto godown;
      break;
   
      case 'r':
        reply(mbuffer[msgindex].mesnum, "reply");
        drawlist(startline, endline);
        printf("\x1b[%d;2H>", msgscrpos);        
        printf("\x1b[25;1H");
        fflush(stdout);
      break;

      case 'f':
        reply(mbuffer[msgindex].mesnum, "forward");
        drawlist(startline, endline);
        printf("\x1b[%d;2H>", msgscrpos);        
        printf("\x1b[25;1H");
        fflush(stdout);
      break;

      case 'c':
        compose();
        drawlist(startline, endline);
        printf("\x1b[%d;2H>", msgscrpos);        
        printf("\x1b[25;1H");
        fflush(stdout);
      break;

      case 'X':
        expunge();
        drawlist(startline, endline);
        printf("\x1b[%d;2H>", msgscrpos);        
        printf("\x1b[25;1H");
        fflush(stdout);
      break;

      case 'R':
        reconnect();
        drawlist(startline, endline);
        printf("\x1b[%d;2H>", msgscrpos);        
        printf("\x1b[25;1H");
        fflush(stdout);
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
  tio.flags |= TF_ICANON;
  settio(STDOUT_FILENO, &tio);

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

  refreshscreen();
  
  while(option != 's'){
    fflush(stdin);
    fflush(fp);
    eom = 0;

    if(type == "download") {
      download = fopen(filename, "w");
      if(download == NULL) {
        printf("Could not create file %s\n", filename);
        tio.flags &= ~TF_ICANON;
        settio(STDOUT_FILENO, &tio);
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
    tio.flags &= ~TF_ICANON;
    settio(STDOUT_FILENO, &tio);

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
    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    getline(&buf, &size, stdin);
    buf[strlen(buf)-1]=0;

    fflush(stdin);
    tio.flags &= ~TF_ICANON;
    settio(STDOUT_FILENO, &tio);

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

    sprintf(buffer, "cat %s |base64 d >%s", path, filename);
    system(buffer);

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
  char * var1 = NULL;
  char * buf  = NULL;
  char * path = NULL;
  int  size   = 0;
  int  var2;

  sprintf(buffer, "data/mailcount%d.dat", currentserv);
  path    = fpathname(buffer, getappdir(), 1);
  counter = fopen(path, "r");
  if(counter){
    getline(&buf, &size, counter);
    var1 = strdup(buf);
    var2 = atoi(var1);
    var2--;
    fclose(counter);
  } else {
    printf("Could not open mailcount%d.dat\n", currentserv);
    getchar();
    fflush(stdin);
  }

  sprintf(buffer, "data/mailcount%d.dat", currentserv);
  path    = fpathname(buffer,getappdir(), 1);
  counter = fopen(path, "w");
  if(!counter) {
    printf("Couldn't create mailcount%d.dat\n", currentserv);
    getchar();
    fflush(stdin);
    return(0);
  } else {
    fprintf(counter, "%d\n", var2);
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

      address[i].addy = buf;
      buf = NULL;
      size = 0;
      i++;

    } else if(strstr(buf, "ubject")) {

      subject = buf;
      buf = NULL;
      size = 0;

    }
    getline(&buf, &size, fp);
  } 

  if(numofaddies) 
    fixreturn();

  if(!choosereturnaddy()) 
    //return(0);

  refreshscreen();

  // Fix the Subject line...
  newsubject = subject;
  newsubject += 9;

  printf("Subject: %s\n\n", newsubject);
  printf("Modify the subject line? ");
  fflush(stdout);

  tio.flags &= ~TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  if(getchar() == 'y') {
    
    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    printf("\n");
    printf("Enter New Subject: ");
    fflush(stdout);
    getline(&buf, &size, stdin);
    free(subject);
    subject = buf;
    newsubject = subject;
    buf = NULL;
    size = 0;
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

  tio.flags &= ~TF_ICANON;
  settio(STDOUT_FILENO, &tio);

//  if(type == "reply") {
    gettio(STDOUT_FILENO, &tio);  
    spawnlp(S_WAIT, "ned", path, NULL);
    settio(STDOUT_FILENO, &tio);
//  }

  bodyclr();
  refreshscreen();

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
    if(j < 2)
      sprintf(buffer, "qsend -v -s \"%s\" -t %s -m %s", newsubject, GetReturnAddy(0), path);
    else
      sprintf(buffer, "qsend -v -s \"%s\" -t %s -C %s -m %s", newsubject, GetReturnAddy(0), GetReturnAddy(j-1), path);

    system(buffer);

    playsound(MAILSENT);
  }

  printf("\nWould you like to save this message? ");
  fflush(stdout);

  if(getchar()== 'y'){

    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

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
  refreshscreen();

  tio.flags &= ~TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  printf("\n");

  for(i = 0; i < numofaddies; i++) {
    address[i].use = '-';
    printf("   - %s\n", address[i].addy);
  }

  printf("\x1b[23;1H");
//  printf("  +/-/[return]  (s)top (t)ake address (r)eply flag toggle (a)dd address\n");
  printf("  +/-/[return]  (t)ake address (r)eply flag toggle (a)dd address\n");

  //first draw the arrow beside the first list item.
  //Then move the cursor to home. we know where it was with arrowpos

  printf("\x1b[%d;2H>", 2); 
  printf("\x1b[24;1H");
  fflush(stdout);
}

int choosereturnaddy() {
  int  currentpos = 0;
  int  arrowpos   = 2;
  int  gotoeditor = 0;
  int  i          = 0;
  char choice     = '-';

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
          printf("\x1b[%d;2H ", arrowpos);
          arrowpos--;
          printf("\x1b[%d;2H>", arrowpos);
          printf("\x1b[24;1H");
          fflush(stdout);
        break;

        case '-':
          if(currentpos == numofaddies-1)
            break;
          currentpos++;
          //erase arrow
          printf("\x1b[%d;2H ", arrowpos);
          arrowpos++;
          printf("\x1b[%d;2H>", arrowpos);
          printf("\x1b[24;1H");
          fflush(stdout);
        break;

        case 'r':
          if(address[currentpos].use == 'R') {
            address[currentpos].use = '-';
            printf("\x1b[%d;4H-", arrowpos);
            printf("\x1b[24;1H");
            fflush(stdout);
          } else {
            address[currentpos].use = 'R';
            printf("\x1b[%d;4H*", arrowpos);
            printf("\x1b[24;1H");
            fflush(stdout);
          }

          if(currentpos == numofaddies-1)
            break;
          currentpos++;
          //erase arrow
          printf("\x1b[%d;2H ", arrowpos);
          arrowpos++;
          printf("\x1b[%d;2H>", arrowpos);
          printf("\x1b[24;1H");
          fflush(stdout);

        break;

        case 'a':
          tio.flags |= TF_ICANON;
          settio(STDOUT_FILENO, &tio);
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
          tio.flags |= TF_ICANON;
          settio(STDOUT_FILENO, &tio);
          printf("What nick do you want to make this address?\n");
          getline(&buf, &size, stdin);
          if(buf[0] != '\n') {
            if(buf[strlen(buf)-1] == '\n')
              buf[strlen(buf)-1] = 0;
            sprintf(buffer, "echo %s %s >> /wings/programs/net/qsend.app/resources/nicks.rc", buf, address[currentpos].addy); 
            system(buffer);
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

  fflush(stdin);
  tio.flags |= TF_ICANON;
  settio(STDOUT_FILENO, &tio);

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
  tio.flags &= ~TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  printf("Do you want to attach a file or files? (y/n) ");
  fflush(stdout);

  if(getchar() == 'y') {

    printf("\nAttachments: (ie /path/path/filename.jpg,/path/filename.wav,...)\n");

    fflush(stdin);
    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    getline(&buf, &size, stdin);  
    attach = strdup(buf);  
    attach[strlen(attach)-1] = 0;

    fflush(stdin);
    tio.flags &= ~TF_ICANON;
    settio(STDOUT_FILENO, &tio);
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
  refreshscreen();
  printf("Would you like to send this message? ");
  fflush(stdout);

  if(attach) {
    if(getchar()=='y'){
      printf("working\n");
      sprintf(buffer, "qsend -v -s \"%s\" -t %s -a %s -m %s", subject, sendaddress, attach, path);	
      system(buffer);
    }
  } else {
    if(getchar()=='y'){
      printf("Working...\n");
      sprintf(buffer, "qsend -v -s \"%s\" -t %s -m %s", subject, sendaddress, path);
      system(buffer);

      playsound(MAILSENT);
    }
  }
  
  printf("would you like to save this message? ");
  fflush(stdout);

  if(getchar()=='y'){

    fflush(stdin);
    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    printf("What filename do you want for the saved message?\n");

    getline(&buf, &size, stdin);
    buf[strlen(buf)-1] = 0;

    spawnlp(S_WAIT, "mv" , path, buf, NULL);
  } else {
    spawnlp(S_WAIT, "rm", path, NULL);
  }

  fflush(stdin);
  tio.flags &= ~TF_ICANON;
  settio(STDOUT_FILENO, &tio);

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

