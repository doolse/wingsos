#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <winlib.h>
#include <winman.h>
#include <xmldom.h>

int channel;
DOMElement * configxml,* confignode;
struct PSInfo APS;
int httpdid,telnetdid;
FILE * passwdfp;
char * buf = NULL;
int size = 0;

void httpd(void * button) {
  void * webrootw = JWGetData(button);
  void * logfilew = JWGetData(webrootw);
  char * webroot = JTxfGetText(webrootw);
  char * logfile = JTxfGetText(logfilew);
  char * str;
  int prevpid = 1;

  if(httpdid) {
    kill(httpdid,1);
    httpdid = 0;
    ((JBut*)button)->Label[2] = 'a'; //change button to say Start
    ((JBut*)button)->Label[3] = 'r';
    ((JBut*)button)->Label[4] = 't';
    JWReDraw(button);
  } else {
    str = malloc(strlen("httpd")+strlen(webroot)+strlen(logfile)+5);
    sprintf(str,"httpd %s >>%s",webroot,logfile);
    system(str);
    free(str);
    ((JBut*)button)->Label[2] = 'o'; //change button to say Stop
    ((JBut*)button)->Label[3] = 'p';
    ((JBut*)button)->Label[4] = ' ';
    JWReDraw(button);
    confignode = XMLgetNode(configxml,"/xml/webroot");
    XMLsetAttr(confignode,"path",webroot);
    confignode = XMLgetNode(configxml,"/xml/logfile");
    XMLsetAttr(confignode,"path",logfile);
    XMLsaveFile(configxml,fpathname("config.xml",getappdir(),1));
    syncfs("/");

    do {
      prevpid = getPSInfo(prevpid, &APS);
      if(!strncmp(APS.Name,"httpd",strlen("httpd"))) {
        httpdid = APS.PID;
        break;
      }
    } while(prevpid);  
    printf("httpd started, PID == %d\n",httpdid);
  }
}

void telnetd(void * button) {
  void * passwdw = JWGetData(button);
  char * passwd = JTxfGetText(passwdw);  
  FILE * passwdfp;  
  int prevpid = 1;

  if(telnetdid) {
    kill(telnetdid,1);
    telnetdid = 0;
    ((JBut*)button)->Label[2] = 'a'; //change button to say Start
    ((JBut*)button)->Label[3] = 'r';
    ((JBut*)button)->Label[4] = 't';
    JWReDraw(button);
  } else {
    if(strlen(passwd) > 0) {
      confignode = XMLgetNode(configxml,"/xml/passwd");
      passwdfp = fopen(XMLgetAttr(confignode,"path"),"w");
      if(passwdfp) {
        fprintf(passwdfp,"%s\n",passwd);
        JTxfSetText(passwdw,"");
        fclose(passwdfp);    
        syncfs("/");
      }
    }
    system("telnetd");

    ((JBut*)button)->Label[2] = 'o'; //change button to say Stop
    ((JBut*)button)->Label[3] = 'p';
    ((JBut*)button)->Label[4] = ' ';
    JWReDraw(button);

    do {
      prevpid = getPSInfo(prevpid, &APS);
      if(!strncmp(APS.Name,"telnetd",strlen("telnetd"))) {
        telnetdid = APS.PID;
        break;
      }
    } while(prevpid);  
    printf("telnetd started, PID == %d\n",telnetdid);
  }
}

void main(int argc,char * argv[]) {
  JMeta * metadata;
  SizeHints sizes;
  void *app,*window,*textbar,*label,*button,*cnt;
  int prevpid = 1;

  app = JAppInit(NULL,0);
  
  metadata = malloc(sizeof(JMeta));
  metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
  metadata->title = "Sharing";
  metadata->icon = controlpanel_icon;
  metadata->showicon = 1;
  metadata->parentreg = -1;

  window = JWndInit(NULL,metadata->title,0,metadata);
  JAppSetMain(app,window);

  ((JCnt *)window)->Orient = JCntF_TopBottom;

  httpdid = telnetdid = 0;

  do {
    prevpid = getPSInfo(prevpid, &APS);
    if(!httpdid) {
      if(!strncmp(APS.Name,"httpd",strlen("httpd")))
        httpdid = APS.PID;
    }
    if(!telnetdid) {
      if(!strncmp(APS.Name,"telnetd",strlen("telnetd")))
        telnetdid = APS.PID;
    }
    if(httpdid && telnetdid)
      break;
  } while(prevpid);  


  configxml = XMLloadFile(fpathname("config.xml",getappdir(),1));

  cnt = JCntInit(NULL);
  ((JCnt*)cnt)->Orient = JCntF_LeftRight;
  JCntAdd(window,cnt);
  label = JStxInit(NULL,"Web Server:  ");
  JCntAdd(cnt,label);
  if(httpdid)
    button = JButInit(NULL,"Stop ");
  else
    button = JButInit(NULL,"Start");
  JCntAdd(cnt,button);
  JWinCallback(button,JBut,Clicked,httpd);

  cnt = JCntInit(NULL);
  ((JCnt*)cnt)->Orient = JCntF_LeftRight;
  JCntAdd(window,cnt);
  label = JStxInit(NULL,"Web Root:");
  JCntAdd(cnt,label);
  textbar = JTxfInit(NULL);
  JWSetPen(textbar,COL_White);
  JWSetBack(textbar,COL_Blue);
  JCntAdd(cnt,textbar);
  JWSetData(button,textbar);

  confignode = XMLgetNode(configxml,"/xml/webroot");
  JTxfSetText(textbar,XMLgetAttr(confignode,"path"));

  cnt = JCntInit(NULL);
  ((JCnt*)cnt)->Orient = JCntF_LeftRight;
  JCntAdd(window,cnt);
  label = JStxInit(NULL,"Log File: ");
  JCntAdd(cnt,label);
  button = JTxfInit(NULL); //we need to keep the old textbar ptr
  JWSetPen(button,COL_White);
  JWSetBack(button,COL_Blue);
  JCntAdd(cnt,button);
  JWSetData(textbar,button);

  confignode = XMLgetNode(configxml,"/xml/logfile");
  JTxfSetText(button,XMLgetAttr(confignode,"path"));

  cnt = JCntInit(NULL);
  ((JCnt*)cnt)->Orient = JCntF_LeftRight;
  JCntAdd(window,cnt);
  label = JStxInit(NULL,"Remote Login:");
  JCntAdd(cnt,label);
  if(telnetdid)
    button = JButInit(NULL,"Stop ");
  else
    button = JButInit(NULL,"Start");
  JCntAdd(cnt,button);
  JWinCallback(button,JBut,Clicked,telnetd);

  cnt = JCntInit(NULL);
  ((JCnt*)cnt)->Orient = JCntF_LeftRight;
  JCntAdd(window,cnt);
  label = JStxInit(NULL,"Password:");
  JCntAdd(cnt,label);
  textbar = JTxfInit(NULL);
  JCntAdd(cnt,textbar);
  JWSetPen(textbar,COL_White);
  JWSetBack(textbar,COL_Blue);
  JWSetData(button,textbar);

  JCntGetHints(window,&sizes);
  JWSetBounds(window,24,32,sizes.PrefX,sizes.PrefY);
  JWndSetProp(window);

  JWinShow(window);
  retexit(0);
  JAppLoop(app);
}
