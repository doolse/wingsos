//Mail v2.x settings.c; Edit Account Settings code module.
#include "mailheader.h"

// *** Global Variables ***   

DOMElement * configxml; //the root element of the config xml element.
soundsprofile * soundsettings;

int logofg_col,     logobg_col,     serverselectfg_col, serverselectbg_col;
int listfg_col,     listbg_col,     listheadfg_col,     listheadbg_col;
int listmenufg_col, listmenubg_col, messagefg_col,      messagebg_col;

// *** Function Declarations ***

void displaymsgcount(accountprofile * aprofile);

char * getrequeststring(char * requeststr);
int lastmsgrequest();
ulong skipsizerequest();

int checkforchanges(DOMElement *server, soundsprofile *soundfiles, DOMElement * messages, accountprofile *aprofile, int deletemsgs, ulong skipsize);
int saveserverchanges(accountprofile * aprofile,soundsprofile * soundfiles,int deletemsgs, ulong skipsize, DOMElement *inboxindex, DOMElement *server, DOMElement *messages);

accountprofile * makeprofile(DOMElement * server);
soundsprofile  * soundselect(soundsprofile *soundfiles);
soundsprofile  * clearsoundfile(soundsprofile *soundfiles);

void editserverdisplay(accountprofile * aprofile,int deletemsgs, ulong skipsize, int tabfocus, soundsprofile * soundfiles, int baseposition);

// *** Body of Module's Code ***

int editserver(DOMElement * server, soundsprofile * soundfiles) {
  DIR * dir;
  accountprofile * aprofile;
  DOMElement * inboxindex, * messages;
  char * path, * tempstr;
  int update, baseposition, input, deletemsgs, tabfocus;
  unsigned long skipsize;
      
  deletemsgs = atoi(XMLgetAttr(server, "deletemsgs"));
  skipsize   = strtoul(XMLgetAttr(server, "skipsize"), NULL, 10);
 
  aprofile = makeprofile(server);
        
  path    = fpathname("data/servers/", getappdir(), 1);
  tempstr = (char *)malloc(strlen(path)+strlen(aprofile->datadir)+strlen("/index.xml")+1);
      
  sprintf(tempstr, "%s%s/index.xml", path, aprofile->datadir);
          
  inboxindex = XMLloadFile(tempstr);
  free(path);
  free(tempstr);
      
  messages           = XMLgetNode(inboxindex, "/xml/messages");
  aprofile->lastmsg  = atoi(XMLgetAttr(messages, "firstnum"));
        
  baseposition = 17;
  update       = 1;
  input        = 0;  
  tabfocus     = 1;
      
  while(input != 'Q') {
  
    if(update) {
      editserverdisplay(aprofile,deletemsgs,skipsize,tabfocus,soundfiles,baseposition);
      if(tabfocus == 2) {
        con_gotoxy(2,baseposition+soundfiles->position);
        putchar('>');
        con_update();
      }
    } else
      update = 1;
  
    input = con_getkey();
    switch(input) {
        
      //Standard Options:
  
      case 'd':
        aprofile->display = getrequeststring("Enter new display name:");
      break;
      case 'i':
        aprofile->address = getrequeststring("Enter new address:");
      break; 
      case 'u': 
        aprofile->username = getrequeststring("Enter new username:");
      break;
      case 'p':
        //Special case here, handled inside getrequeststring.
        aprofile->password = getrequeststring("password");
      break;
      case 'f':
        aprofile->fromname = getrequeststring("Enter your name:");
      break;
      case 'r':
        aprofile->returnaddress = getrequeststring("Enter return email address:");
      break;
      
      //Tab Switching:
      
      case 'a':
        if(tabfocus == 2)
          tabfocus = 1;
        else
          update = 0;
      break;
      case 'e':
        if(tabfocus == 1)
          tabfocus = 2;
        else
          update = 0;
      break;   
        
      //Advanced Options (Tab):
      
      case 'h':
        if(tabfocus == 1)
          displaymsgcount(aprofile);
        else
          update = 0;
      break;
      case 'n':
        if(tabfocus == 1)
          aprofile->lastmsg = lastmsgrequest();
        else   
          update = 0;
      break;
      case 's':
        if(tabfocus == 1)
          skipsize = skipsizerequest();
        else { 
          if(soundfiles->active == 1)
            soundfiles->active = 0;
          else
            soundfiles->active = 1;
        }   
      break;   
      case 'D':
        if(tabfocus == 1) {
          if(deletemsgs)
            deletemsgs = 0;
          else 
            deletemsgs = 1;
        } else
          update = 0;
      break;   
        
//      case 'i':
//        if(tabfocus == 1)
//          getdirectfromserver(username, password, address);
//        else
//          update = 0;
//      break;
          
      //Sound Event Settings (Tab):
      
      case CURU:
        if(tabfocus == 2) {
          if(soundfiles->position > 0) {
            con_gotoxy(2,baseposition+soundfiles->position--);
            putchar(' ');
            con_gotoxy(2,baseposition+soundfiles->position);
            putchar('>');
            con_update();
          }
        }
        update = 0;
      break;
      
      case CURD:
        if(tabfocus == 2) {
          if(soundfiles->position < 5) {
            con_gotoxy(2,baseposition+soundfiles->position++);
            putchar(' ');
            con_gotoxy(2,baseposition+soundfiles->position);
            putchar('>');
            con_update();
          } 
        }
        update = 0;
      break;
     
      case '\n':
      case '\r':
        if(tabfocus == 2)
          soundfiles = soundselect(soundfiles);
      break;
      case 'c': 
        if(tabfocus == 2)  
          soundfiles = clearsoundfile(soundfiles);
      break;
            
      case 'Q':
        if(checkforchanges(server, soundfiles, messages, aprofile, deletemsgs, skipsize)) {
          drawmessagebox("Do you want to save the changes? (y/n)","",0);
         
          input = 0;
          while(input != 'y' && input != 'n')
            input = con_getkey();
      
          if(input == 'y')
            return(saveserverchanges(aprofile, soundfiles,deletemsgs,skipsize, inboxindex, server, messages));
          else
            return(0);   
        }   
      break;
      default:
        update = 0;
      break;
    }
  }
  return(0);
}

void editserverdisplay(accountprofile * aprofile,int deletemsgs, ulong skipsize, int tabfocus, soundsprofile * soundfiles, int baseposition) {
  con_clrscr();
  con_update();
    
  printf("        Edit email account settings for '%s':\n\n", aprofile->display);
  
  printf("  Standard Options:\n\n");
    
  printf("             (d)isplay name: %s\n", aprofile->display);
  printf("                             (For reference only)\n");
  printf("         (i)ncoming address: %s\n", aprofile->address);
  printf("                             (POP3 incoming mail server)\n");
  printf("                 (u)sername: %s\n", aprofile->username);
  printf("                 (p)assword: ********\n");
  printf("                (f)rom name: %s\n", aprofile->fromname);
  printf("     (r)eturn email address: %s\n", aprofile->returnaddress);
      
  if(tabfocus == 1) {
    printf("     ____________________     ________________________\n");
    printf("____/   Advanced Options %c___/ Sound (e)vent Settings %c_________________________\n",'\\','\\');
    printf("                          %c_____________________________________________________\n", '\\');
    putchar('\n');
    printf("     (h)ow many messages are on the server?\n");
    printf("     Start at message (n)umber: %d\n", aprofile->lastmsg);
    printf("     (D)elete messages from server after downloading them? : ");
    deletemsgs?printf("Yes\n"):printf("No\n");

    putchar('\n');
    if(skipsize)
      printf("     (s)kip messages more than %ld bytes long.\n", skipsize);
    else
      printf("     No messages will be (s)kipped. All messages will be downloaded.\n");
      
    con_gotoxy(1,24);
    printf("(Q)uit to inbox selector; Standard: (d,i,u,p,f,r) Tabs: (e) Advanced: (h,n,D,s)");
    
  } else {
    printf("     ____________________     ________________________\n");
    printf("____/ (a)dvanced Options %c___/   Sound Event Settings %c_________________________\n", '\\', '\\');
    printf("____________________________/\n");
  
    printf("   Make mail (s)ounds active? :");
    soundfiles->active ? printf("Yes"):printf("No");
  
    con_gotoxy(1,baseposition++);
    printf("   Program starting       :");
    if(!strcmp(soundfiles->hello,"/dev/null"))
      printf("No sound");
    else
      printf("%50s", soundfiles->hello);
    
    con_gotoxy(1,baseposition++);
    printf("   New mail               :");
    if(!strcmp(soundfiles->newmail,"/dev/null"))
      printf("No Sound");
    else
      printf("%50s", soundfiles->newmail);

    con_gotoxy(1,baseposition++);
    printf("   No new mail            :");
    if(!strcmp(soundfiles->nonewmail,"/dev/null"))
      printf("No sound");
    else
      printf("%50s", soundfiles->nonewmail);
    
    con_gotoxy(1,baseposition++);
    printf("   Mail download complete :");
    if(!strcmp(soundfiles->downloaddone,"/dev/null"))
      printf("No sound");
    else
      printf("%50s", soundfiles->downloaddone);
  
    con_gotoxy(1,baseposition++);
    printf("   Message sent           :");
    if(!strcmp(soundfiles->mailsent,"/dev/null"))
      printf("No sound");
    else
      printf("%50s", soundfiles->mailsent);   
      
    con_gotoxy(1,baseposition++);
    printf("   Program ending         :");
    if(!strcmp(soundfiles->goodbye,"/dev/null"))
      printf("No sound");
    else
      printf("%50s", soundfiles->goodbye);
      
    con_gotoxy(1,24);
    printf("(Q)uit to inbox selector; Standard: (d,i,u,p,f,r) Tabs: (a) Sounds: (s,c)");
  }
  con_update();
}


void displaymsgcount(accountprofile * aprofile) {
  char * tempstr, *tempstr2;
  int msgcount, newmsgcount;
      
  msgcount = countservermessages(aprofile, 1);
  newmsgcount = msgcount-(aprofile->lastmsg-1);
  
  if(newmsgcount < 0)
    newmsgcount = 0;
    
  tempstr = (char *)malloc(strlen("There are 1234567890 messages on the server .")+strlen(aprofile->address)+1);
  if(!msgcount)
    sprintf(tempstr, "There are no messages on the server %s.", aprofile->address);
  else if(msgcount == 1)
    sprintf(tempstr, "There is 1 message on the server %s.", aprofile->address);
  else
    sprintf(tempstr, "There are %d messages on the server %s.", msgcount, aprofile->address);
      
  if(newmsgcount)
    tempstr2 = (char *)malloc(80);
  else
    tempstr2 = strdup("None of the messages are new.");
    
  if(newmsgcount == 1)
    sprintf(tempstr2, "1 message is new.");
  if(newmsgcount > 1)
    sprintf(tempstr2, "%d of the messages are new.", newmsgcount);
  
  drawmessagebox(tempstr,tempstr2, 1);
  free(tempstr);
  free(tempstr2);
}


char * getrequeststring(char * requeststr) {
  char * returnstr;
  
  if(!strcmp(requeststr, "password")) {
    drawmessagebox("Enter new password","                              ",0);
    return(getmyline(30,25,13,1));
  } else {
    drawmessagebox(requeststr,"                              ",0);
    return(getmyline(30,25,13,0));
  }
}



int lastmsgrequest() {
  int num;
  char * linebuf;
    
  drawmessagebox("Specify message number the download will start from:", " ", 0);
  linebuf = getmylinen(10,14,13);
  num = atoi(linebuf);
  free(linebuf);
  if(num < 1)   
    num = 1;
 
  return(num);
} 

ulong skipsizerequest() {
  char * linebuf;
  ulong returnval; 
  
  drawmessagebox("Specify in bytes the message size, greater than which will be skipped:"," ", 0);
  linebuf = getmylinen(9,5,13);
  returnval = strtoul(linebuf, NULL, 10);
  free(linebuf);
  return(returnval);
}
   
int saveserverchanges(accountprofile * aprofile,soundsprofile * soundfiles,int deletemsgs, ulong skipsize, DOMElement *inboxindex, DOMElement *server, DOMElement *messages) {
  DIR *dir;
  char * path, * tempstr;
  DOMElement *soundselem;

  XMLsetAttr(server, "address",       aprofile->address);
  XMLsetAttr(server, "password",      aprofile->password);
  XMLsetAttr(server, "username",      aprofile->username);
  XMLsetAttr(server, "name",          aprofile->display);
  XMLsetAttr(server, "fromname",      aprofile->fromname);
  XMLsetAttr(server, "returnaddress", aprofile->returnaddress);
  
  if(deletemsgs)
    XMLsetAttr(server, "deletemsgs", "1");
  else
    XMLsetAttr(server, "deletemsgs", "0");
  
  tempstr = (char *)malloc(20);
  sprintf(tempstr, "%ld", skipsize);
  XMLsetAttr(server, "skipsize", tempstr);
    
  sprintf(tempstr, "%d", aprofile->lastmsg);
  XMLsetAttr(messages, "firstnum", tempstr);
  free(tempstr);

  path    = fpathname("data/servers/", getappdir(), 1);
  tempstr = (char *)malloc(strlen(path)+strlen(aprofile->datadir)+strlen("/index.xml")+1);
  sprintf(tempstr, "%s%s/index.xml", path, aprofile->datadir);
      
  XMLsaveFile(inboxindex, tempstr);
  free(tempstr);
      
  soundselem = XMLgetNode(configxml, "xml/sounds");

  if(soundfiles->active)
    XMLsetAttr(soundselem, "active", "yes");
  else
    XMLsetAttr(soundselem, "active", "no");

  XMLsetAttr(XMLgetNode(soundselem, "hello"),        "file", soundfiles->hello);
  XMLsetAttr(XMLgetNode(soundselem, "newmail"),      "file", soundfiles->newmail);
  XMLsetAttr(XMLgetNode(soundselem, "nonewmail"),    "file", soundfiles->nonewmail);
  XMLsetAttr(XMLgetNode(soundselem, "downloaddone"), "file", soundfiles->downloaddone);
  XMLsetAttr(XMLgetNode(soundselem, "mailsent"),     "file", soundfiles->mailsent);
  XMLsetAttr(XMLgetNode(soundselem, "goodbye"),      "file", soundfiles->goodbye);
  
  return(1);
}   


int checkforchanges(DOMElement *server, soundsprofile *soundfiles, DOMElement * messages, accountprofile *aprofile, int deletemsgs, ulong skipsize) {
  
  if(strcmp(XMLgetAttr(server,"name"),          aprofile->display)) {
    //drawmessagebox("display","",1);
    return(1);
  }
  if(strcmp(XMLgetAttr(server,"address"),       aprofile->address)) {
    //drawmessagebox("address","",1);
    return(1);
  }
  if(strcmp(XMLgetAttr(server,"username"),      aprofile->username)) {
    //drawmessagebox("username","",1);
    return(1);
  }
  if(strcmp(XMLgetAttr(server,"password"),      aprofile->password)) {
    //drawmessagebox("password","",1);
    return(1);
  }
  if(strcmp(XMLgetAttr(server,"fromname"),      aprofile->fromname)) {
    //drawmessagebox("fromname","",1);
    return(1);
  }
  if(strcmp(XMLgetAttr(server,"returnaddress"), aprofile->returnaddress)) {
    //drawmessagebox("returnaddy","",1);
    return(1);
  }
  if(strcmp(soundsettings->hello,        soundfiles->hello)) {
    //drawmessagebox("hello","",1);
    return(1);
  }
  if(strcmp(soundsettings->newmail,      soundfiles->newmail)) {
    //drawmessagebox("newmail","",1);
    return(1);
  }
  if(strcmp(soundsettings->nonewmail,    soundfiles->nonewmail)) {
    //drawmessagebox("nonewmail","",1);
    return(1);
  }
  if(strcmp(soundsettings->downloaddone, soundfiles->downloaddone)) {
    //drawmessagebox("downloaddone","",1);
    return(1);
  }
  if(strcmp(soundsettings->mailsent,     soundfiles->mailsent)) {
    //drawmessagebox("mailsent","",1);
    return(1);
  }
  if(strcmp(soundsettings->goodbye,      soundfiles->goodbye)) {
    //drawmessagebox("goodbye","",1);
    return(1);
  }
    
  if(soundfiles->active) {
    if(strcmp(XMLgetAttr(XMLgetNode(configxml, "xml/sounds"), "active"), "yes"))
      return(1);
  } else {
    if(strcmp(XMLgetAttr(XMLgetNode(configxml, "xml/sounds"), "active"), "no"))
      return(1);
  }
  
  if(atoi(XMLgetAttr(server,"deletemsgs")) != deletemsgs) {
    //drawmessagebox("deletefromserver","",1);
    return(1);
  }
  if(strtoul(XMLgetAttr(server, "skipsize"), NULL, 10) != skipsize) {
    //drawmessagebox("skipsize","",1);
    return(1);
  }
  if(atoi(XMLgetAttr(messages,"firstnum")) != aprofile->lastmsg) {
    //drawmessagebox("firstnum","",1);
    return(1);
  }

  return(0);
} 

accountprofile * makeprofile(DOMElement * server) {
  accountprofile * aprofile;
  
  aprofile = (accountprofile *)malloc(sizeof(accountprofile));
  
  aprofile->datadir       = strdup(XMLgetAttr(server, "datadir"));
  aprofile->display       = strdup(XMLgetAttr(server, "name"));   
  aprofile->address       = strdup(XMLgetAttr(server, "address")); 
  aprofile->username      = strdup(XMLgetAttr(server, "username"));
  aprofile->password      = strdup(XMLgetAttr(server, "password"));
  aprofile->fromname      = strdup(XMLgetAttr(server, "fromname"));
  aprofile->returnaddress = strdup(XMLgetAttr(server, "returnaddress"));
  
  return(aprofile);
} 


soundsprofile * soundselect(soundsprofile *soundfiles) {
  FILE * fmptr;
  char * buf = NULL;
  int size = 0;     

  spawnlp(S_WAIT, "fileman", NULL);
     
  //the temp file is heinous and bad. but until I figure out
  //how to do it with pipes, a temp file it shall remain.
  
  fmptr = fopen("/wings/.fm.filepath.tmp", "r");
  getline(&buf, &size, fmptr);
  fclose(fmptr);

  switch(soundfiles->position) {
    case 0:
      free(soundfiles->hello);
      soundfiles->hello = strdup(buf);
    break;
    case 1:
      free(soundfiles->newmail);
      soundfiles->newmail = strdup(buf);
    break;
    case 2:
      free(soundfiles->nonewmail);
      soundfiles->nonewmail = strdup(buf);
    break;
    case 3:
      free(soundfiles->downloaddone);
      soundfiles->downloaddone = strdup(buf);
    break;
    case 4:
      free(soundfiles->mailsent);
      soundfiles->mailsent = strdup(buf);
    break;
    case 5:
      free(soundfiles->goodbye);
      soundfiles->goodbye = strdup(buf);
    break;
  }
  
  return(soundfiles);
}
  

soundsprofile * clearsoundfile(soundsprofile *soundfiles) {
    
  switch(soundfiles->position) {
    case 0:
      free(soundfiles->hello);
      soundfiles->hello = strdup("/dev/null");
    break; 
    case 1:
      free(soundfiles->newmail);
      soundfiles->newmail = strdup("/dev/null");
    break; 
    case 2:
      free(soundfiles->nonewmail);
      soundfiles->nonewmail = strdup("/dev/null");
    break; 
    case 3:
      free(soundfiles->downloaddone);
      soundfiles->downloaddone = strdup("/dev/null");
    break; 
    case 4:
      free(soundfiles->mailsent);
      soundfiles->mailsent = strdup("/dev/null");
    break;
    case 5:
      free(soundfiles->goodbye);
      soundfiles->goodbye = strdup("/dev/null");
    break;
  }

  return(soundfiles);
}


int makeserverinbox(DOMElement *servers) {
  char * path,* tempstr;
  int datadir; 

  path    = fpathname("data/servers/", getappdir(), 1);
  tempstr = (char *)malloc(strlen(path)+16+strlen("/drafts")+2);

  //get current datadir, increment, write it back

  datadir = atoi(XMLgetAttr(servers, "datadir")) + 1;
  sprintf(tempstr, "%d", datadir);
  XMLsetAttr(servers, "datadir", tempstr);

  sprintf(tempstr, "%s%d", path, datadir);
  mkdir(tempstr, 0);
  sprintf(tempstr, "%s%d/drafts", path, datadir);
  mkdir(tempstr, 0);
  sprintf(tempstr, "%s%d/sent", path, datadir);
  mkdir(tempstr, 0);

  free(tempstr);

  return(datadir);
}

void makenewmessageindex(int datadir) {
  FILE * messageindex;
  char * tempstr;

  tempstr = (char *)malloc(strlen("data/servers//drafts/index.xml")+16+2);

  //create inbox xml index file
  sprintf(tempstr, "data/servers/%d/index.xml", datadir);
  messageindex = fopen(fpathname(tempstr, getappdir(), 1), "w");
  fprintf(messageindex, "<xml><messages firstnum=\"1\" refnum=\"0\"></messages></xml>");
  fclose(messageindex);

  //create drafts xml index file
  sprintf(tempstr, "data/servers/%d/drafts/index.xml", datadir);
  messageindex = fopen(fpathname(tempstr, getappdir(), 1), "w");
  fprintf(messageindex, "<xml><messages refnum=\"0\"></messages></xml>");
  fclose(messageindex);

  //create sent mail xml index file
  sprintf(tempstr, "data/servers/%d/sent/index.xml", datadir);
  messageindex = fopen(fpathname(tempstr, getappdir(), 1), "w");
  fprintf(messageindex, "<xml><messages refnum=\"0\"></messages></xml>");
  fclose(messageindex);

  free(tempstr);
}

int addserver(DOMElement * servers) {
  DOMElement * newserver;
  char * name,* address,* username,* password;
  char * fromname,* returnaddress,* tempstr;
  int datadir,i,input = 'n';

  name     = address       = username = password = NULL;
  fromname = returnaddress = tempstr  = NULL;

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

  drawmessagebox("1) Setting up new account...","2) Creating new empty boxes...",0);

  datadir = makeserverinbox(servers);
  makenewmessageindex(datadir);

  newserver = XMLnewNode(NodeType_Element, "server", "");

  tempstr = (char *)malloc(16);
  sprintf(tempstr, "%d", datadir);

  XMLsetAttr(newserver, "datadir", tempstr);
  free(tempstr);

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

  XMLsetAttr(newserver, "mailwatch", "   ");

  //insert the new element as a child of "servers"
  XMLinsert(servers, NULL, newserver);
  
  return(1);
}

int deleteserver(DOMElement *server) {
  int input;
  char * serverpath, * datadir, * subpath;  

  drawmessagebox("Are you SURE you want to delete this account?","            (Y)es   or   (n)o                ",0);
  input = 'a';
  while(input != 'Y' && input != 'n')
    input = con_getkey();

  if(input == 'n')
    return(0);

  datadir = XMLgetAttr(server, "datadir");
  subpath = (char *)malloc(strlen(datadir) + strlen("data/servers/") + 1);
  sprintf(subpath, "data/servers/%s", datadir);
  serverpath = fpathname(subpath,getappdir(),1);

  spawnlp(0,"rm", "-r", "-f", serverpath, NULL);

  return(1);
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
