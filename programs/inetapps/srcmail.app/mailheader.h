#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <fcntl.h>
#include <console.h>
#include <termio.h>
#include <xmldom.h>
#include "../srcqsend.app/qsend.h"

// Sound Event Defines
#define HELLO        1
#define NEWMAIL      2
#define NONEWMAIL    3
#define DOWNLOADDONE 4
#define MAILSENT     5
#define GOODBYE      6

// Address Book Defines
#define GET_ATTRIB   225
#define GET_ALL_LIST 226
#define MAKE_ENTRY   227
#define PUT_ATTRIB   228

#define NOENTRY      -2
#define ERROR        -1
#define SUCCESS      0

// Compose Message section Defines
#define TO      0
#define SUBJECT 1
#define CC      2
#define BCC     3
#define ATTACH  4
#define BODY    5

// TYPE of Message Compose Defines
#define COMPOSENEW       0
#define REPLY            1
#define REPLYCONTINUED   2
#define COMPOSECONTINUED 3
#define RESEND           4

// Box Type Defines   
#define INBOX     1   
#define DRAFTSBOX 2   
#define SENTBOX   3

typedef struct dataset_s {
  ulong number;
  char * string;
} dataset;

typedef struct msgpass_s {
  int code;
} msgpass;

typedef struct msgline_s {
  struct msgline_s * prevline;
  struct msgline_s * nextline;
  char * line;   
} msgline;

typedef struct soundsprofile_s {
  int position;

  char *hello;
  char *newmail;
  char *nonewmail;
  char *downloaddone;
  char *mailsent;
  char *goodbye;   
  int active;
} soundsprofile;   

typedef struct accountprofile_s {
  char * display;
  char * username;
  char * password;
  char * address;
  char * fromname;
  char * returnaddress;   
    int lastmsg;
  ulong mailcheck;
} accountprofile;

//addressbook data structure  
  
typedef struct namelist_s {
  int use;
  char * firstname;
  char * lastname;
} namelist;  
  
//message box object
  
typedef struct msgboxobj_s {
  int numoflines; 
  int showprogress;  
  
  char * msgline[3];  
  
  int progresswidth;  
  ulong numofitems;
  ulong progressposition;
  int linelength;
  
  int top;
  int bottom;
  int left;
  int right;
    
} msgboxobj;




// FUNCTION Declarations 

soundsprofile * initsoundsettings();
soundsprofile * setupsounds(soundsprofile * soundtemp);
  
int playsound(int soundevent); //returns int, so you can do an early return(1);
      
int setupcolors();

void mailwatch();  

int establishconnection(accountprofile * aprofile);
void terminateconnection();

int getnewmail(accountprofile *aprofile, DOMElement * messages, char * serverpath, msgboxobj * mb, ulong skipsize, int deletefromserver);
int countservermessages(accountprofile *aprofile, int connect);
dataset * getnewmsgsinfo(accountprofile *aprofile, DOMElement * messages, DOMElement * server);

int view(DOMElement * server,int fileref, char * serverpath, char * subpath);
msgline * parsehtmlcontent(msgline * prevline);
void givesomedatatoweb(); //fprintf data to a pipe until you hit a boundary
void givealldatatoweb();  //printf all data (regardless of mime) to a pipe

void viewattachedlist(char * serverpath, DOMElement * message);

msgboxobj * initmsgboxobj(char * msgline1, char * msgline2, char * msgline3, int showprogress, ulong numofitems);
void drawmsgboxobj(msgboxobj * mb);
     
void drawmessagebox(char * string1, char * string2, int wait);

char * getmyline(int size, int x, int y, int password);
char * getmylinen(int size, int x, int y);
