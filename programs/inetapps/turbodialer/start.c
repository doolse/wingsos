//A console application for managing internet dialup accounts, by Greg Nacu

//spawns "ppp" written by Jolz Maginnis
//spawns pppterm which is a modification of "term" written by Jolz Maginnis
//uses embedded code ripped from netstat.c and poff.c written by Jolz Maginnis

#include <stdio.h>
#include <console.h>
#include <exception.h>
#include <fcntl.h>
#include <net.h>
//#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <xmldom.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../mail/messagebox.h"
#include "../mail/getlines.h"


struct wmutex pppmutex = {-1,-1};
int stoppppcheck=0,pppcheckactive=0,sleepchan, pppstatus = 0;
IntStat ibufs[128];
long bauds[12] = {0, 110, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400};
//struct PSInfo APS;

void dialmodem(char * number, char * baudstr) {
  int ch,firstchar = 0,charcount = 0;
  FILE *stream;
  struct termios T1;
  unsigned int baud=0;
  long brate,min=1000000,dif;

  brate  = strtol(baudstr, NULL, 0);

  ch = 0;
  while(ch<11) {
    dif = bauds[ch] - brate;
    if(dif<0)
      dif = -dif;
    if(dif<min) {
      baud = ch;
      min = dif;
    }
    ch++;
  }  

  stream = fopen("/dev/ser0","w+");
  gettio(fileno(stream),&T1);
  T1.flags = TF_OPOST|TF_IGNCR;
  T1.Speed = baud;
  settio(fileno(stream),&T1);
  gettio(STDIN_FILENO,&T1);
  T1.flags &= ~(TF_ICANON|TF_ECHO|TF_ICRLF);
  settio(STDIN_FILENO,&T1);

  fprintf(stream, "atdt%s\n", number);

  charcount = strlen("atdt\n") + strlen(number) + 5;

  fclose(stream);
  stream = fopen("/dev/ser0","rb");
  gettio(fileno(stream),&T1);
  T1.flags = TF_OPOST|TF_IGNCR;
  T1.Speed = baud;
  settio(fileno(stream),&T1);
  gettio(STDIN_FILENO,&T1);
  T1.flags &= ~(TF_ICANON|TF_ECHO|TF_ICRLF);
  settio(STDIN_FILENO,&T1);

  stream->_flags |= _IOBUFEMP;

  while(!feof(stream)) {
    while ((ch = fgetc(stream)) != EOF) {
      if(charcount > 0) {
        if(!firstchar && ch == 'a')
          firstchar = 1;
        if(firstchar)
          charcount--;
      } else
        goto enddialup;
    }
    fflush(stdout);
  }
  enddialup:
  fclose(stream);
}

void mysleep(int seconds) {
  void * Msg;

  setTimer(-1,seconds*1000, 0, sleepchan, PMSG_Alarm);
  recvMsg(sleepchan,(void *)&Msg);
}

void stopppp() {	
  int fd;

  fd = open("/sys/ppp0",O_READ|O_WRITE|O_PROC);
  if (fd != -1) {
    sendCon(fd, IO_CONTROL);
    close(fd);
    mysleep(2);
  } else 
    drawmessagebox("Error: ppp not up.","Press a key.",1);
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

void checkpppstatus() {
  IntStat *icur = ibufs;
  int channel,i,tcpFD; 
  uchar * msg;

  pppcheckactive = 1;

  channel = makeChan();
  tcpFD   = open("/sys/tcpip",O_PROC);

  setTimer(-1, 2000, 0, channel, PMSG_Alarm);

  while(1 /*&& !stoppppcheck*/) {
    recvMsg(channel,(void *)&msg);
//    if(stoppppcheck)
//      break;
    getMutex(&pppmutex);

      //Try to get IP address from tcpip thingy
      i = statint(tcpFD,ibufs,128*sizeof(IntStat));

      if(i>0) {
        con_gotoxy(14,3);
        pppstatus = 1;
        printf("Connected");
        con_gotoxy(14,4);
        printf("%s",inet_ntoa(icur->IP));
      } else {
        con_gotoxy(14,3);
        pppstatus = 0;
        printf("Not Connected");
        con_gotoxy(14,4);
        printf("               ");
      }

      con_update();
    relMutex(&pppmutex);

    //loop every 2 seconds.
    setTimer(-1, 2000, 0, channel, PMSG_Alarm);
  }
  stoppppcheck = 0;
  pppcheckactive = 0;
}

void loadtcpip(int silent) {
  int tcpFD;

  tcpFD = open("/sys/tcpip",O_PROC);

  if(tcpFD == -1) {
    if(!silent)
      drawmessagebox("Loading TCP/IP Driver...","",0);
    system("tcpip.drv >/dev/null 2>/dev/null");
  } 

  tcpFD = open("/sys/tcpip",O_PROC);

  if(tcpFD == -1) {
    drawmessagebox("TCP/IP Driver could not be loaded.","Press any key to quit.",1);
    con_end();
    con_clrscr();
    con_update();
    exit(1);
  } else 
    close(tcpFD);
}

void loadserialdev() {
  FILE * stream;
  
  stream = fopen("/dev/ser0", "rb");

  if(!stream) {
    drawmessagebox("Loading Serial Device Driver...","",0);
    system("uart.drv >/dev/null 2>/dev/null");
  }

  stream = fopen("/dev/ser0", "rb");
  if(!stream) {
    drawmessagebox("Serial Device Driver could not be loaded.","Press any key to quit.",1);
    con_end();
    con_clrscr();
    con_update();
    exit(1);
  }
  fclose(stream);
}

void drawmainmenu() {
  con_gotoxy(0,con_ysize-1);
  con_clrline(LC_End);
  if(con_xsize > 40) {
    printf(" (Q)uit, (a)dd new account, (e)dit account");
    if(pppstatus)
      printf(", (D)isconnect");
  } else {
    printf("(Q)uit, (a)dd, (e)dit");
    if(pppstatus)
      printf(", (D)isconnect");
  }
}

void drawmaininterface(DOMElement * firstaccount, int pos) {
  int i = pos+1;
  DOMElement * tempnode = firstaccount;

  con_clrscr();
  con_gotoxy(0,1);
  printf(" - Turbo Dialer (beta v0.5) -");
  
  con_gotoxy(0,3);
  printf("      Status:");  

  con_gotoxy(0,4);
  printf("  Current IP:");

  if(tempnode) {
    do {
      con_gotoxy(3,i++);
      printf("%s",XMLgetAttr(tempnode,"accountname"));
      tempnode = tempnode->NextElem;
    } while(tempnode != firstaccount);
  }

  drawmainmenu();
}

DOMElement * addaccount() {
  DOMElement * thisacct;
  char * tempstr;
  int i;

  thisacct = XMLnewNode(NodeType_Element,"account","");

  drawmessagebox("Account Name:","                    ",0);
  tempstr = getmylinerestrict(NULL,20,20,30,13,"",0);
  XMLsetAttr(thisacct,"accountname",tempstr);
  
  drawmessagebox("User Name:","                    ",0);
  tempstr = getmylinerestrict(NULL,20,20,30,13,"",0);
  XMLsetAttr(thisacct,"username",tempstr);

  drawmessagebox("Password:","                    ",0);
  tempstr = getmylinerestrict(NULL,20,20,30,13,"",1);
  XMLsetAttr(thisacct,"password",tempstr);

  drawmessagebox("Dial Up Number:","                    ",0);
  tempstr = getmylinerestrict(NULL,20,20,30,13,"",0);
  XMLsetAttr(thisacct,"number",tempstr);

  drawmessagebox("Authentication Type:"," (p)ap  or  (s)hell  ",0);
  i = 0;
  while(i != 'p' && i != 's')
    i = con_getkey();
  if(i == 'p')
    XMLsetAttr(thisacct,"auth","pap");
  else
    XMLsetAttr(thisacct,"auth","shell");  

  return(thisacct);
}

void editaccount(DOMElement * thisacct) {
  char * tempstr;
  int i;

  tempstr = strdup(XMLgetAttr(thisacct,"accountname"));
  drawmessagebox("Account Name:","                    ",0);
  tempstr = getmylinerestrict(tempstr,20,20,30,13,"",0);
  XMLsetAttr(thisacct,"accountname",tempstr);

  tempstr = strdup(XMLgetAttr(thisacct,"username"));
  drawmessagebox("User Name:","                    ",0);
  tempstr = getmylinerestrict(tempstr,20,20,30,13,"",0);
  XMLsetAttr(thisacct,"username",tempstr);

  tempstr = strdup(XMLgetAttr(thisacct,"password"));
  drawmessagebox("Password:","                    ",0);
  tempstr = getmylinerestrict(tempstr,20,20,30,13,"",1);
  XMLsetAttr(thisacct,"password",tempstr);

  tempstr = strdup(XMLgetAttr(thisacct,"number"));
  drawmessagebox("Dial Up Number:","                    ",0);
  tempstr = getmylinerestrict(tempstr,20,20,30,13,"",0);
  XMLsetAttr(thisacct,"number",tempstr);

  drawmessagebox("Authentication Type:"," (p)ap  or  (s)hell  ",0);
  i = 0;
  while(i != 'p' && i != 's')
    i = con_getkey();
  if(i == 'p')
    XMLsetAttr(thisacct,"auth","pap");
  else
    XMLsetAttr(thisacct,"auth","shell");  
}

void main(int argc, char *argv[]) {
  int prevpid,ex,numofaccounts,liststartpos,pos,input,refresh;
  void * exp;
  FILE *accountfp;
  DOMElement *xmlroot,*xmltag,*firstaccount, *curaccount, *tempelem;
  char *tempstr;

  prepconsole();

  sleepchan = makeChan();

  loadserialdev();
  loadtcpip(0);

  newThread(checkpppstatus,STACK_DFL,NULL);

  Try {
    xmlroot = XMLloadFile(fpathname("accounts.xml",getappdir(),1));
  }
  Catch2(ex,exp) {
    accountfp = fopen(fpathname("accounts.xml",getappdir(),1),"w");
    fprintf(accountfp,"<xml/>");
    fclose(accountfp);    
    xmlroot = XMLloadFile(fpathname("accounts.xml",getappdir(),1));
  }
  
  xmltag = XMLgetNode(xmlroot,"/xml");

  firstaccount = XMLgetNode(xmltag,"account");
  if(!firstaccount) 
    numofaccounts = 0;
  else 
    numofaccounts = xmltag->NumElements;

  liststartpos = 8;
  drawmaininterface(firstaccount, liststartpos);

  pos = 1;
  curaccount = firstaccount;

  if(numofaccounts) {
    con_gotoxy(1,liststartpos+pos);  
    putchar('>');
  }
  con_update();

  input = 0;
  while(input != 'Q') {
    input   = con_getkey();
    refresh = 1;        

    switch(input) {
      case CURD:
        if(pos < numofaccounts) {
          con_gotoxy(1,liststartpos+pos);
          putchar(' ');
          pos++;
          con_gotoxy(1,liststartpos+pos);
          putchar('>');
          curaccount = curaccount->NextElem;
          con_update();
        } 
        refresh = 0;
      break;

      case CURU:
        if(pos > 1) {
          con_gotoxy(1,liststartpos+pos);
          putchar(' ');
          pos--;
          con_gotoxy(1,liststartpos+pos);
          putchar('>');
          curaccount = curaccount->PrevElem;
          con_update();
        } 
        refresh = 0;
      break;

      case 'a':
        if(numofaccounts < 5) {
          tempelem = addaccount();
          XMLinsert(xmltag,NULL,tempelem);
          if(!numofaccounts)
            firstaccount = curaccount = XMLgetNode(xmltag,"account");
          numofaccounts++;
        } else {
          drawmessagebox("Only 5 accounts may be configured.","Press a key.",1);
        }
      break;

      case 'e':
        if(curaccount) {
          editaccount(curaccount);
        } else 
          refresh = 0;
      break;

      case DEL:
        if(curaccount) {
          drawmessagebox("Delete this dialup account? (y/n)","",0);
          while(input != 'y' && input != 'n')
            input = con_getkey();
          if(input == 'y') {
            numofaccounts--;
            tempelem = curaccount;
            if(!numofaccounts) 
              firstaccount = curaccount = NULL;
            else {
              if(curaccount == firstaccount)
                firstaccount = curaccount = curaccount->NextElem;
              else
                curaccount = curaccount->NextElem;
            }
            XMLremNode(tempelem);
          } 
        } else 
          refresh = 0;
      break;

      case 'D':
        if(pppstatus || 1) {
          getMutex(&pppmutex);
/*
          stoppppcheck = 1;
          while(pppcheckactive)
            mysleep(1);
*/
          stopppp();
          //system("poff >/dev/null 2>/dev/null");
          relMutex(&pppmutex);
/*
          mysleep(1);
          prevpid = 1;
          while(prevpid) {
            prevpid = getPSInfo(prevpid,&APS);
            if(!strncmp(APS.Name,"tcpip.drv",9)) {
              kill(APS.PID,1);
              break;
            }
          }          
          loadtcpip(1);
          newThread(checkpppstatus,STACK_DFL,NULL);
*/
        } else
          refresh = 0;
      break;

      case '\n':
        if(!pppstatus) {
          getMutex(&pppmutex);
          tempstr = malloc(strlen("ppp /dev/ser0   >/dev/null 2>/dev/null &") + strlen(XMLgetAttr(curaccount,"username")) + strlen(XMLgetAttr(curaccount,"password")) +1);

          if(!strcmp(XMLgetAttr(curaccount,"auth"),"pap")) {
            sprintf(tempstr,"ppp /dev/ser0 %s %s", XMLgetAttr(curaccount,"username"), XMLgetAttr(curaccount,"password"));
            dialmodem(XMLgetAttr(curaccount,"number"),"57600");
            mysleep(1);
          } else {
            sprintf(tempstr,"ppp /dev/ser0");
            spawnlp(S_WAIT,fpathname("pppterm",getappdir(),1),XMLgetAttr(curaccount,"number"),"57600",NULL);
          }
          system(tempstr);
          free(tempstr);
          relMutex(&pppmutex);
        }
      break;

      default:
        refresh = 0;
    }
    if(refresh) {
      drawmaininterface(firstaccount, liststartpos);

      pos = 1;
      curaccount = firstaccount;

      if(numofaccounts) {
        con_gotoxy(1,liststartpos+pos);  
        putchar('>');
      }
      con_update();
    }
  }

  XMLsaveFile(xmlroot,fpathname("accounts.xml",getappdir(),1));

  con_end();
  con_clrscr();
  con_update();

  //ensure you don't exit with the /sys/tcpip descriptor open.
  getMutex(&pppmutex);
}
