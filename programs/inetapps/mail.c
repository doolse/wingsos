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
  char addy[80];
} addressST;

struct termios tio;
struct termios termsettings;

addressST address[10];
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
char * extractboundary();

// Display Altering Functions
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
int view(int messagenum);
int viewmodeheader();
int expunge();
int erase(int messagenum);
int deletefinal();
int reply(int messagei);
int choosereturnaddy();
int compose();

// Download Related Functions
int download();
int downloadheader(FILE *lcemail);

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
  printf("about to try connecting\n");

  setsounds();
  playsound(HELLO);

  gettio(STDOUT_FILENO, &tio);
  tio.MIN = 1;
  settio(STDOUT_FILENO, &tio);

  connect(1);
  printf("\n Logged In. Have Fun! \n\n");

  howmanymessages();
  printf("There are %d messages on this server.  ", howmany);
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

  printf("\x1b[0m\x1b[H\x1b[2J");

  fflush(stdout);
  fflush(fp);
  fprintf(fp, "QUIT\r\n");
  gline();
  fclose(fp);

  settio(STDOUT_FILENO, &termsettings);
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
     sprintf(buf, "wavplay %s >/dev/null &", sound1);
     system(buf);
     break;
   case NEWMAIL:
     sprintf(buf, "wavplay %s >/dev/null &", sound2);
     system(buf);
     break;
   case NONEWMAIL:
     sprintf(buf, "wavplay %s >/dev/null &", sound3);
     system(buf);
     break;
   case MAILSENT:
     sprintf(buf, "wavplay %s >/dev/null &", sound4);
     system(buf);
     break;
   case REFRESH:
     sprintf(buf, "wavplay %s >/dev/null &", sound5);
     system(buf);
     break;
   case GOODBYE:
     sprintf(buf, "wavplay %s >/dev/null &", sound6);
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
  char *server     = NULL;
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

    printf("Which server do you want to use? You have %d configured.\n", servnumi);

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

  //printf("server choice, %d", currentserv);

  for(i = 1; i < currentserv; i++){
    getline(&buf, &size, resource);
    getline(&buf, &size, resource);
    getline(&buf, &size, resource);
  }

  getline(&buf, &size, resource);
  user = (char *)malloc(strlen(buf));
  if(user == NULL)
    memerror();
  user = strdup(buf);
  user[strlen(user)-1] = 0;

  getline(&buf, &size, resource);
  pass = (char *)malloc(strlen(buf));
  if(pass == NULL)
    memerror();
  pass = strdup(buf);
  pass[strlen(pass)-1] = 0;

  getline(&buf, &size, resource);
  server = (char *)malloc(strlen(buf));
  if(server == NULL)
    memerror();
  server = strdup(buf);
  server[strlen(server)-1] = 0;

  fclose(resource);

  if(verbose)
    printf("Connecting to server, %s\n", server);

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
    printf("Sent Username %s\n", user);

  gline();
  fflush(fp);
  fprintf(fp, "PASS %s\r\n", pass);

  if(verbose){
    printf("Sent Password ****\n");
    printf("Authenticating User\n");
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
    if(howmanywere < howmany)
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
        for(j = placeholder; j < 29; j++) 
          buf[j] = ' ';
         
        buf[29] = 0;
        strcpy(mbufptr->from , buf);
      } else if(buf[0] == 'S' && buf[1] =='u'){

        placeholder = strlen(buf);
        buf[placeholder-2] = ' ';
        buf[placeholder-1] = ' ';
        for(j = placeholder; j < 40; j++) 
          buf[j] = ' ';
         
        buf[40] = 0;
        strcpy(mbufptr->subj , buf);
      }
    } while(buf[0] != '.');
    mbufptr++;
  } // ends the forloop that gets all message headers.

  return(1);
}

int displaylist() {
  int startline = 0;
  int endline   = 22;
  int j         = 0;
  char option;

  do {
    refreshscreen();
    if(endline > max)
      endline = max;
    if(endline < max) {
      if(max < 22)
        endline = max;
      else
        endline = startline + 22;
    }
     
    bodyclr();

    //printf("Displaying list Max = %d\n", max);
    //printf("startline = %d\n", startline);
    //printf("endline = %d\n", endline);

    //DRAW the LIST!!
   
    for(j = startline; j<endline; j++)
      printf("\n\x1b[33m%c %d %s %s", mbuffer[j].deleted, mbuffer[j].mesnum, mbuffer[j].from, mbuffer[j].subj);

    tio.flags &= ~TF_ICANON;
    settio(STDOUT_FILENO, &tio);
    menuclr();

    printf("\n+/-/Q  (v)iew (c)ompose (r)eply (d)ownload (e)rase (X)punge re(f)resh");

    attachments=0;
    fflush(stdin);
    fflush(stdout);
    option = getchar();

    switch(option) {

      case '+':
        //page up a max of 22 lines...
 
        if(max <= 22){
          startline = 0;
          endline = max;
        } else { 
          if(startline > 0){
            startline = startline - 22;
            if(startline < 0)
              startline = 0;
            endline = startline + 22;
          }
        }
      break;
      
      case '-':
        //page down a max of 22 lines...  

        if(max <= 22){
          startline = 0;
          endline = max;
        } else { 
          if(endline <= max){
            startline = startline + 22;
            endline = startline +22;
            if(endline > max){
              endline = max; 
              startline = endline -22;
            }
          }
        }
      break;

      case 'v':
        view(-1);
      break;
      
      case 'd':
        download();
      break;

      case 'e':
        erase(-1);
      break;
   
      case 'r':
        reply(-1);
      break;

      case 'c':
        compose();
      break;

      case 'X':
        expunge();
      break;

      case 'f':
        reconnect();
      break;
    }
  } while(option != 'Q');

  return(0);
}

int view(int messagenum){
  char option; 
  int  messagei = 0;
  int  skipped  = 0;
  int  lines    = 19;
  int  index    = 0;

  fflush(stdin);
  tio.flags |= TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  if(messagenum == -1) {
    printf("\nWhich Message # do you Want to view? ");
    fflush(stdout);

    getline (&buf, &size, stdin); 
    buf[strlen(buf)-1] = 0;
    messagei = atoi(buf);
  } else {
    messagei = messagenum;
  }

  refreshscreen();
  
  while(option != 's'){
    fflush(stdin);
    fflush(fp);
    fprintf(fp, "TOP %d %d\r\n", messagei, lines);
    sprintf(history, "TOP %d %d\r\n", messagei, lines);    
    if(!(viewmodeheader()))
      return(0);
  
    buf[0]  = 0 ;
    skipped = lines-19;
    index   = 0;

    bodyclr();       

    while(!(buf[0] == '.' && buf[1] == '\r' && buf[2] == '\n')) {
      index++;
      getline (&buf, &size, fp);
 
      if(index>skipped)
        printf("%s", buf);
    } 

    fflush(fp);
    tio.flags &= ~TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    menuclr();

    if(attachments) {
      printf("(+/-), (s)top, (r)eply (d)ownload attached?  Line #%d Message #%d of %d\n", lines, messagei, howmany);
    } else {
      printf("(+/-), (s)top, (r)eply (n)ext (p)rev ? Line #%d Message #%d of %d\n", lines, messagei, howmany);
    }

    option = getchar();

    if(option == '+'){
      if(lines!=19)
        lines = lines - 19;
    } else if(option == '-'){
      lines = lines + 19;
    } else if(option == 'r'){
      reply(messagei);
    } else if((option =='d') && (attachments)) {
      dealwithmime(messagei);
    } else if(option == 'n') {
      view(messagei+1);
      return(0);
    } else if(option == 'p') {
      view(messagei-1);
      return(0);
    } else if(option == 'e') {
      erase(messagei);
    }
  }

  fflush(fp);
  return(0);
}

int viewmodeheader(){

  attachments = 0;
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
  headclr();
  do{
    getline(&buf, &size, fp);

    if(!     (strncmp(buf, "To:",  3)))
      printf("%s", buf);

    else if(!(strncmp(buf, "From", 4)))
      printf("%s", buf);

    else if(!(strncmp(buf, "Subj", 4)))
      printf("%s", buf);

    else if(!(strncmp(buf, "Content-Type: multipart/mixed;", 30))) {
      attachments = 1;
      boundary = extractboundary();
    } else if(!(strncmp(buf, "Content-Type: MULTIPART/MIXED;", 30))) {
      attachments = 1;
      boundary = extractboundary();
    }
  } while((strcmp(buf, "\r\n")));
  return(1);
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
      newstr = newstr+6;
    else if(newstr = strstr(buf, "name= "))
      newstr = newstr+7;
    else
      newstr = newstr+6;

  } else if(newstr = strstr(buf, "name =")) {

    if(newstr = strstr(buf, "name = \""))
      newstr = newstr+8;
    else if(newstr = strstr(buf, "name =\""))
      newstr = newstr+7;
    else 
      newstr = newstr+6;
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

char * extractboundary(){
  char * boundary = NULL;
  int  i          = 0;
  int  j          = 0;
  int  start      = 0;

  boundary = (char *)malloc(21);
  if(boundary == NULL)
    memerror();

  if(!strstr(buf, "boundary")) 
    getline(&buf, &size, fp);

  //extract a max of 20 chars for boundary from between the quotes        

  for(i = 0; j<20; i++) {
    if(start) {
      if(buf[i] == '"'){
        boundary[j] = 0;
        j = 16;
      } else {
        boundary[j] = buf[i];
        boundary[j+1] = 0;
        j++;
      }
    } else if(buf[i] == '"')
      start=1;
    }      
  return(boundary);
}

int download() {
  FILE *lcemail;
  char *messagec = NULL;
  int  messagei  = 0;

  fflush(stdin);
  printf("\nWhich Message # Do you want to Download? ");
  fflush(stdout);

  tio.flags |= TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  getline(&buf, &size, stdin);

  messagec = (char *)malloc(strlen(buf));
  if(messagec == NULL)
    memerror();

  messagec = strdup(buf);
  messagei = atoi(messagec);

  printf("\nEnter a filename You want for this download:\n");

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1]=0;

  lcemail = fopen(buf, "w+");
  if(!lcemail){
    printf("Error: Local email file could not be created. Aborting Download.\n");
    return(0);
  }
  fflush(fp);
  fprintf(fp, "RETR %d\r\n", messagei);
  sprintf(history, "RETR %d\r\n", messagei);
  fflush(fp);

    if(!(downloadheader(lcemail)))
      return(0);

    while(!(buf[0] == '.' && buf[1] == '\r' && buf[2] == '\n')) {
      getline (&buf, &size, fp);
      if(buf[0] == '.' && buf[1] == '\r' && buf[2] == '\n')
        break; 
      if(buf[strlen(buf)-2] == '\r'){
        buf[strlen(buf)-2] = '\n';
        buf[strlen(buf)-1] = 0;
      }
      fprintf(lcemail, "%s", buf);
    } 

    fclose(lcemail);

    printf("Download Successful.  Press Any Key.\n");
    tio.flags &= ~TF_ICANON; 
    settio(STDOUT_FILENO, &tio);
    getchar();

    return(0);
}

int downloadheader(FILE *lcemail){

  if(-1 == getline(&buf, &size, fp)){
    if(!(reconnect()))
      return(0);
    gline();
  }

  if(buf[0] == '-'){
    printf("Error Message doesn't exist\n");
    fclose(lcemail);
    return(0);
  }
  do{
    getline(&buf, &size, fp);
    if(buf[strlen(buf)-2] == '\r'){
      buf[strlen(buf)-2] = '\n';
      buf[strlen(buf)-1] = 0;
    }

    if(!(strncmp(buf, "To: ", 4))){
      fprintf(lcemail, "%s", buf);
    }
    else if(!(strncmp(buf, "From", 4))){
      fprintf(lcemail, "%s", buf);
    }
    else if(!(strncmp(buf, "Subj", 4))){
      fprintf(lcemail, "%s", buf);
    }
  } while((strcmp(buf, "\n")));
  fprintf(lcemail, "--\n");
  return(1);
}  

int erase(int messagenum){
  int messagei = 0;
  int first    = 0;
  int last     = 0;
  int i        = 0;

  if(messagenum == -1) {
    printf("\nToggle Erase Flag for a (s)ingle message or a (r)ange? ");
    fflush(stdout);

    if(getchar() == 's') {

      printf("\nWhich Message # to Toggle Erase Flag? ");
      fflush(stdout);

      fflush(stdin);
      tio.flags |= TF_ICANON;
      settio(STDOUT_FILENO, &tio);

      getline (&buf, &size, stdin); 
      buf[strlen(buf)-1] = 0;
      messagei = atoi(buf);
 
      if((howmany-messagei) <= max){
        if(mbuffer[(max-(howmany-messagei))-1].deleted == 'E')
          mbuffer[(max-(howmany-messagei))-1].deleted = ' ';
        else
          mbuffer[(max-(howmany-messagei))-1].deleted = 'E';
      }
    } else {
    
      fflush(stdin);
      tio.flags |= TF_ICANON;
      settio(STDOUT_FILENO, &tio);

      printf("\nFirst message # in Range? ");
      fflush(stdout);

      getline(&buf, &size, stdin);
      buf[strlen(buf)-1] = 0;

      first = atoi(buf);

      printf("Last message # in Range? ");
      fflush(stdout);

      getline(&buf, &size, stdin);
      buf[strlen(buf)-1] = 0;

      last = atoi(buf);

      for(i = 0; i < max; i++) {
        if(((mbuffer[i].mesnum == first) || (mbuffer[i].mesnum > first)) && ((mbuffer[i].mesnum == last) || (mbuffer[i].mesnum < last))) {
          if(mbuffer[i].deleted == 'E')
            mbuffer[i].deleted = ' ';
          else
            mbuffer[i].deleted = 'E';
        }
      }
    } 
  } else {

    messagei = messagenum;

    if((howmany-messagei) <= max)
      mbuffer[(max-(howmany-messagei))-1].deleted = 'E';
  }
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

    var1 = (char *)malloc(strlen(buf));
    if(var1 == NULL)
      memerror();

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
  
int reply(int messagei){
  FILE * tempfile;
  char * subject       = NULL;
  char * path          = NULL;
  int  i               = 0;
  int  j               = 0;

  numofaddies = 0;

  for(i = 0; i < 10; i++) {
    address[i].use = '-';
    address[i].addy[0] = 0;
  }

  if(messagei == -1){

    fflush(stdin);
    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    printf("\nWhich message # Do you want to reply to? ");
    fflush(stdout);

    getline(&buf, &size, stdin);
    buf[strlen(buf)-1] = 0;

    fflush(fp);

    fprintf(fp,      "RETR %d\r\n", atoi(buf));
    sprintf(history, "RETR %d\r\n", atoi(buf));
    fflush(fp);

  } else {

    fprintf(fp,      "RETR %d\r\n", messagei);
    sprintf(history, "RETR %d\r\n", messagei);
    fflush(fp);

  }

  if(getline(&buf, &size, fp)==-1){
    if(!(reconnect()))
      return(0);
    gline();
  }
  if(buf[0] == '-')
   return(0);

  getline(&buf, &size, fp);
  
  while(strlen(buf) > 2) {
    if(strlen(buf) > 1) {
      if(buf[strlen(buf)-2] == '\r') {
        buf[strlen(buf)-2] = 0;
      }
      if(buf[strlen(buf)-1] == '\n') {
        buf[strlen(buf)-1] = 0;
      }
    }

    if(strstr(buf, "@")) {         
      if(numofaddies < 8) {
        strcpy(address[numofaddies].addy,buf);
        numofaddies++;
      }
    } else if(strstr(buf, "Subject")) {

      subject = (char *)malloc(strlen(buf));
      if(subject == NULL) 
        memerror();

      subject = strdup(buf);
      if(subject[strlen(subject)] == '\n')
        subject[strlen(subject)-1] = 0;
    }
    getline(&buf, &size, fp);
  } 


  if(!numofaddies) {

    tio.flags &= ~TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    printf("There were no addresses available to return this message to.\n");
    printf("Press any key.\n");
    getchar();    
    return(0);
  } else {
    fixreturn();
  }

  choosereturnaddy();

  // Fix the Subject line...
  for(i = 9; i<strlen(subject); i++){
    buffer[j] = subject[i];
    j++;
  }  
  buffer[j] = 0;
  strcpy(subject, buffer);

  // Create Quoted Body Text...

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

    fprintf(tempfile, ">%s", buf);
    getline(&buf, &size, fp);
  }
  fclose(tempfile);

  tio.flags &= ~TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  gettio(STDOUT_FILENO, &tio);  
  spawnlp(S_WAIT, "ned", path, NULL);
  settio(STDOUT_FILENO, &tio);

  bodyclr();
  refreshscreen();

  printf("Would you like to send this message? ");
  fflush(stdout);  

  if(getchar() == 'y'){

    j = 0;
    for(i = 0; i<numofaddies; i++){
      if(address[i].use == 'R')
        j++;
    }

    printf("Sending with QuickSend.\n");
    if(j < 2)
      sprintf(buffer, "qsend -s \"%s\" -t %s -m %s", subject, GetReturnAddy(0), path);
    else
      sprintf(buffer, "qsend -s \"%s\" -t %s -C %s -m %s", subject, GetReturnAddy(0), GetReturnAddy(j-1), path);
    system(buffer);
  }

  playsound(MAILSENT);

  printf("\nWould you like to save this message? ");
  fflush(stdout);

  if(getchar()== 'y'){

    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    printf("\nWhat filename do you want for the saved message?\n");

    getline(&buf, &size, stdin);
    buf[strlen(buf)-1] = 0;
    buf[16] = 0;

    spawnlp(S_WAIT, "mv" , path, buf, NULL);

  } else {
    spawnlp(S_WAIT, "rm", path, NULL);
  }
  return(1);
}

char * GetReturnAddy(int num) {
  int  i = 0;
  char *ccstring = NULL;
  int  first = 1;

  ccstring = (char *)malloc(800);

  //if num is 0, it returns the first Address with an R flag set.

  //if num is 1-9 it returns between 1 and 9 addresses whose R flag is
  //set, skipping the first one found, and seperating the returned addies
  //with commas.

  if(num == 0) {
    for(i = 0; i<numofaddies; i++)
      if(address[i].use == 'R')
        return(address[i].addy);
  } else if(num > 0) {
    for(i=0; i<numofaddies; i++) {
      if(address[i].use == 'R') {
        if(!first) {
          strcat(ccstring, address[i].addy);
          strcat(ccstring, ",");          
        } else {
          first = 0;
        }
      }
    }
    ccstring[strlen(ccstring)-1] = 0;
    return(ccstring);
  }
  return("billnacu@king.igs.net");
}

int fixreturn() {
  char * newstring = NULL;
  int i            = 0;
  int j            = 0;
  int k            = 0;

  for(i = 0;i<numofaddies;i++) {

    // -------- Remove Spaces ------------ 
      newstring = (char *)malloc(strlen(address[i].addy));
      if(newstring == NULL) 
        memerror();

      for(k = 0; k <strlen(address[i].addy); k++) {
        if(address[i].addy[k] != ' ') {
          newstring[j] = address[i].addy[k];
          j++;
        }
      }
      newstring[j] = 0;

      strcpy(address[i].addy, newstring);    
    // -----------------------------------

    if(strstr(address[i].addy, "<") && strstr(address[i].addy, ">")) {
      //Extract the address from between the < and >

      for(k = 0; k < strlen(address[i].addy); k++) {
        if(address[i].addy[k] == '<') {
          j = k+1;
          while((address[i].addy[j] != '>')&&(address[i].addy[j] != 0)){
            newstring[j-k-1] = address[i].addy[j];
            j++;
          }
          newstring[j-k-1] = 0;
        }
      }

      strcpy(address[i].addy, newstring);
      j = 0;

    } else if(strstr(address[i].addy, "(") && strstr(address[i].addy, ")")) {
      //Extract the address from between the ( and )

      for(k = 0; k < strlen(address[i].addy); k++) {
        if(address[i].addy[k] == '(') {
          j = k+1;
          while((address[i].addy[j] != ')')&&(address[i].addy[j] != 0)){
            newstring[j-k-1] = address[i].addy[j];
            j++;
          }
          newstring[j-k-1] = 0;
        }
      }

      strcpy(address[i].addy, newstring);
      j = 0;

    } else if(strstr(address[i].addy, ":") && strstr(address[i].addy, ";")) {
      //Extract the address from between the : and ;

      for(k = 0; k < strlen(address[i].addy); k++) {
        if(address[i].addy[k] == ':') {
          j = k+1;
          while((address[i].addy[j] != ';')&&(address[i].addy[j] != 0)){
            newstring[j-k-1] = address[i].addy[j];
            j++;
          }
          newstring[j-k-1] = 0;
        }
      }

      strcpy(address[i].addy, newstring);
      j = 0;

    } else if(strstr(address[i].addy, ":")) {
      //Extract the address from after the : to the end

      for(k = 0; k < strlen(address[i].addy); k++) {
        if(address[i].addy[k] == ':') {
          j = k+1;
          while(address[i].addy[j] != 0){
            newstring[j-k-1] = address[i].addy[j];
            j++;
          }
          newstring[j-k-1] = 0;
        }
      }

      strcpy(address[i].addy, newstring);
      j = 0;

    }
    //Otherwise... the address slips through unchanged.
  
    free(newstring);
    j = 0;
    
  }

  return(1);
}

int choosereturnaddy() {
  int  currentpos = 0;
  int  i          = 0;  
  char choice     = '-';

  tio.flags &= ~TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  while(choice != '\n') {
    bodyclr();
    refreshscreen();
    printf("\n  Use +/- to position cursor beside address\n");
    printf("  Press r beside address to toggle reply flag\n");
    printf("  Press return when finished\n");

    for(i = 0; i < numofaddies; i++) {
      if(i == currentpos) 
        printf(" > ");
      else
        printf("   ");
      printf("%c %s\n", address[i].use, address[i].addy);
    }  
 
    choice = getchar();
    switch(choice) {
      case '+':
        if(currentpos == 0)
          break;
        currentpos = currentpos-1;
      break;
      case '-':
        if((currentpos == 9) || (currentpos == numofaddies-1))
          break;
        currentpos = currentpos+1;
      break;
      case 'r':
        if(address[currentpos].use == 'R')
          address[currentpos].use = '-';
        else 
          address[currentpos].use = 'R';
      break;
    }
  }

  return(1);
}

int compose(){
  FILE * tempfile;
  static char sendaddress[100];
  char * subject = NULL;
  char * attach  = NULL;
  char * path    = NULL;

  subject = (char *)malloc(100);
  if(subject == NULL) {
    printf("Memory allocation error. Press a key to return to list.\n");
    getchar();
    return(0);
  }

  fflush(stdin);
  tio.flags |= TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  printf("\nTo (address/nick): ");
  fflush(stdout);

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;
  strcpy(sendaddress, buf);

  printf("Subject: ");
  fflush(stdout);

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;
  subject = strdup(buf);  

  fflush(stdin);
  tio.flags &= ~TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  printf("Do you want to attach a file or files? (y/n) ");
  fflush(stdout);

  if(getchar() == 'y') {
    attach = (char *)malloc(255);
    if(attach == NULL) {
      printf("Memory allocation error. Press a key to return to list.\n");
      getchar();
      return(0);
    }

    printf("\nAttachments: (ie /path/path/filename.jpg,/path/filename.wav,...)\n");

    fflush(stdin);
    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    getline(&buf, &size, stdin);  
    buf[strlen(buf)-1] = 0;
    attach = strdup(buf);  

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
      sprintf(buffer, "qsend -s \"%s\" -t %s -a %s -m %s", subject, sendaddress, attach, path);	
      system(buffer);
    }
  } else {
    if(getchar()=='y'){
      printf("working\n");
      sprintf(buffer, "qsend -s \"%s\" -t %s -m %s", subject, sendaddress, path);
      system(buffer);
    }
  }
  
  playsound(MAILSENT);
  
  printf("would you like to save this message? ");
  fflush(stdout);

  if(getchar()=='y'){

    fflush(stdin);
    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    printf("What filename do you want for the saved message?\n");

    getline(&buf, &size, stdin);
    buf[strlen(buf)-1] = 0;
    buf[16] = 0;

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

