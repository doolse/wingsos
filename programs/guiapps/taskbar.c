#include <dirent.h>
#include <fcntl.h>
#include <winlib.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <taskbar.h>
#include <winman.h>

#define LAUNCHIT 200

int appchannel,position;
item * itemhead = NULL;
void * taskbar;
int numofitems = 0;

void getrunningapps();

unsigned char ICO_APP_DFL[] = {
0,1,3,3,7,6,14,12,
0,128,192,192,224,96,112,48,
28,31,63,56,120,120,252,0,
56,248,252,28,30,30,63,0,  
0x06,0x06,0x06,0x06
};

unsigned char cbuticon[] = {
0,7,31,63,63,124,120,120,
0,128,128,128,254,124,120,0,
120,120,124,63,63,31,7,0,
0,120,124,254,128,128,128,0,
0x16,0x16,0xf6,0xf6
};

void resizebar(int direction) {
  int size,pos;

  numofitems += direction;
  size = 16 * numofitems;

  if(position == TSKM_ONLEFT) {
    pos = 100 - ((numofitems * 16)/2);
    JWSetBounds(taskbar,0,pos,16,size);
  } else if(position == TSKM_ONRIGHT) {
    pos = 100 - ((numofitems * 16)/2);
    JWSetBounds(taskbar,304,pos,16,size);
  } else if(position == TSKM_ONTOP) {
    pos = 160 - ((numofitems * 16)/2);
    JWSetBounds(taskbar,pos,0,size,16);
  } else if(position == TSKM_ONBOTTOM) {
    pos = 160 - ((numofitems * 16)/2);
    JWSetBounds(taskbar,pos,184,size,16);
  }

  JWndSetProp(taskbar);
}

MenuData * getmenudir(char * path) {
  DIR * dir;
  struct dirent * entry;
  char * nextdir, * launchpath;
  MenuData * menudata, *mdptr, *submenu;
  int i = 0;

  dir = opendir(path);
  while(readdir(dir))
    i++;
  closedir(dir);
  i++;  //for the null terminating MenuData element

  dir = opendir(path);
  mdptr = menudata = malloc(sizeof(MenuData)*i);
  launchpath = malloc(strlen(path) + (16 * 2) +2);
  i = 25;
  while(entry = readdir(dir)) {
    i--;
    if(entry->d_type == 6 && strcmp(".app",entry->d_name + strlen(entry->d_name)-4)) {
      nextdir = malloc(strlen(path) + strlen(entry->d_name)+2);
      sprintf(nextdir,"%s%s/",path,entry->d_name);
      submenu = getmenudir(nextdir);
    } else
      submenu = NULL;

    mdptr->name     = strdup(entry->d_name);

    if(!strcmp(".app",entry->d_name + strlen(entry->d_name)-4)) {
      sprintf(launchpath,"%s%s/start",path,entry->d_name);
      mdptr->name[strlen(mdptr->name)-4] = 0;
    } else {
      sprintf(launchpath,"%s%s",path,entry->d_name);
    }

    mdptr->shortcut = 0;
    mdptr->icon     = NULL;
    mdptr->flags    = 0;
    mdptr->command  = LAUNCHIT;
    mdptr->data     = strdup(launchpath);
    mdptr->submenu  = submenu;
    mdptr++;
    if(!i)
      break;
  }
  closedir(dir);
  free(launchpath);

  mdptr->name     = NULL;
  mdptr->shortcut = 0;
  mdptr->icon     = NULL;
  mdptr->flags    = 0;
  mdptr->command  = 0;
  mdptr->data     = NULL;
  mdptr->submenu  = NULL;

  return(menudata);
}

void cmenulaunch(void * self,MenuData *item) {

  switch(item->command) {
    case LAUNCHIT:
      spawnlp(0,item->data,NULL);
    break;
  }
}

void opencmenu(void * cbut, int Type, int X, int Y, int XAbs, int YAbs) {
  int xy[2];
  void * dynmenu, * menudata = JWGetData(cbut);

  JWAbs(cbut,xy);
  if(position == TSKM_ONLEFT || position == TSKM_ONRIGHT)
    dynmenu = JMnuInit(NULL,0,menudata,xy[0]+16,xy[1],cmenulaunch);
  else if(position == TSKM_ONTOP || position == TSKM_ONBOTTOM)
    dynmenu = JMnuInit(NULL,0,menudata,xy[0],xy[1]+16,cmenulaunch);

  JWinShow(dynmenu);
}

void makecbutton() {
  void * clickbmpclass, * cbut;
  MenuData * mainmenu;

  clickbmpclass = JSubclass(&JBmpClass,-1,
                              METHOD(MJW, Button), opencmenu,
                              -1);
  cbut = JNew(clickbmpclass);
  JBmpInit(cbut,16,16,cbuticon);
  ((JW*)cbut)->Sense |= WEV_Button;

  mainmenu = getmenudir("/wings/");
  JWSetData(cbut,mainmenu);

  resizebar(1);
  JCntAdd(taskbar,cbut);
  JWinShow(cbut);
  sendChan(appchannel,REFRESH);

  newThread(getrunningapps,STACK_DFL,NULL);
}

void changefocus(void * icon,int subtype) {
  int fd;
  item * thisitem = JWGetData(icon);

  if(subtype == EVS_But1Up) {
    if((fd = open("/sys/winman",O_PROC)) != -1) {
      sendCon(fd,TSKM_INFOCUS,0,thisitem->mainwin);
      close(fd);
    }
  }
}

void updateitem(msgpass * msg) {
  int code = msg->code;
  int getnum = msg->getnum;
  void * mainwin = msg->mainwin;
  item * curitem = itemhead;
  void * clickbmpclass;
  int region;
  RegInfo props;
  int size,position;

  switch(code) {
    case TSKM_KILLED:
      if(!curitem)
        break;
      while(curitem->mainwin != mainwin) {
        curitem = curitem->next;
        //printf("looking for item to kill\n");
      }
      JCntRemove(taskbar,curitem->icon);
      JWKill(curitem->icon);
      itemhead = remQueue(itemhead,curitem);
      sendChan(appchannel,REFRESH);
      resizebar(-1);
    break;    

    case TSKM_NEWITEM:
      curitem = malloc(sizeof(item));

      region = ((JMan *)mainwin)->Region;
      JRegInfo(region,&props);

      if(props.metadata) {
        curitem->title     = props.metadata->title;
        curitem->launchstr = props.metadata->launchpath;
        if(props.metadata->icon) {
          curitem->iconbmp = props.metadata->icon;
        } else {
          curitem->iconbmp = malloc(36);
          memcpy(curitem->iconbmp,ICO_APP_DFL,36);
        }
      } else {
        curitem->title     = strdup("Application");
        curitem->launchstr = strdup(""); //something that can be free()'d
        curitem->iconbmp = malloc(36);
        memcpy(curitem->iconbmp,ICO_APP_DFL,36);
      }

      clickbmpclass = JSubclass(&JBmpClass,-1,
                                  METHOD(MJW, Button), changefocus,
                                  -1);
      curitem->icon = JNew(clickbmpclass);
      JBmpInit(curitem->icon,16,16,curitem->iconbmp);
      ((JW*)curitem->icon)->Sense |= WEV_Button;
      JWSetData(curitem->icon,curitem);

      curitem->status  = getnum;
      curitem->mainwin = mainwin;
      
      itemhead = addQueueB(itemhead,itemhead,curitem);

      resizebar(1);
      JCntAdd(taskbar,curitem->icon);
      JWinShow(curitem->icon);
      sendChan(appchannel,REFRESH);

      //printf("taskbar: new item, %d\n",getnum);

      if(getnum == TSKM_INFOCUS)
        sendChan(appchannel,SETCOL,curitem->icon,COL_FOC);
      else if(getnum == TSKM_INBLUR)
        sendChan(appchannel,SETCOL,curitem->icon,COL_NFOC);
      else
        sendChan(appchannel,SETCOL,curitem->icon,COL_MIN);
    break;

    case TSKM_INFOCUS:
      if(!curitem)
        break;
      while(curitem->mainwin != mainwin) {
        curitem = curitem->next;
        //printf("finding first item (infocus)\n");
      }
      while(curitem->next->mainwin != mainwin) {
        //printf("I think this is the bad loop.\n");
        curitem = curitem->next;
        if(curitem->status == TSKM_INFOCUS) {
          sendChan(appchannel,SETCOL,curitem->icon,COL_NFOC);
          curitem->status = TSKM_INBLUR;
        }
      }
      curitem = curitem->next;
      sendChan(appchannel,SETCOL,curitem->icon,COL_FOC);
      curitem->status = TSKM_INFOCUS;
    break;

    case TSKM_MINIMIZED:
      if(!curitem)
        break;
      while(curitem->mainwin != mainwin) {
        curitem = curitem->next;
        //printf("finding first item (minimized)\n");
      }
      sendChan(appchannel,SETCOL,curitem->icon,COL_MIN);
      curitem->status = TSKM_MINIMIZED;
    break;
  }
}

void listener() {
  int channel,rcvid,returncode;
  msgpass * msg = malloc(sizeof(msgpass));

  channel = makeChanP("/sys/taskbar");
  
  while(1) {
    rcvid = recvMsg(channel,(void *)&msg);

    switch(msg->code) {
      case TSKM_KILLED:
      case TSKM_NEWITEM:
      case TSKM_MINIMIZED:
      case TSKM_INFOCUS:
      case TSKM_INBLUR:
        newThread(updateitem,STACK_DFL,msg);
        returncode = 0;
      break;
      case IO_OPEN:
        if(*(int *) (((char *)msg)+6) & (O_PROC|O_STAT))
          returncode = makeCon(rcvid,1);
        else
          returncode = -1;
      break;
    }
    replyMsg(rcvid,returncode);
  }
}

void getrunningapps() {
  int num,i,fd;

  if((fd = open("/sys/winman",O_PROC)) != -1) {
    sendCon(fd,position);

    num = sendCon(fd,TSKM_GETALL);
    for(i=0;i<num;i++)
      sendCon(fd,TSKM_GETNUM,i);
    close(fd);
  }
  //printf("Finished Getting all currently running apps\n");
}

void main(int argc, char * argv[]) {
  void *appl;
  int type,rcvid,icocol;
  char * msg;
  void * but;

  rcvid = open("/sys/taskbar",O_PROC);
  if(rcvid != -1) {
    printf("Taskbar is already running!\n");
    exit(1);
  }

  appchannel = makeChan();
  appl = JAppInit(NULL,appchannel);

  //default position is at top of screen
  position = TSKM_ONTOP;
  if(argc > 1) {
    if(!strcmp(argv[1],"left"))
      position = TSKM_ONLEFT;
    else if(!strcmp(argv[1],"right"))
      position = TSKM_ONRIGHT;
    else if(!strcmp(argv[1],"top"))
      position = TSKM_ONTOP;
    else if(!strcmp(argv[1],"bottom"))
      position = TSKM_ONBOTTOM;
  }

  taskbar = JCntInit(NULL);
  if(position == TSKM_ONTOP || position == TSKM_ONBOTTOM) {
    ((JCnt*)taskbar)->Orient = JCntF_LeftRight;
    JWSetBounds(taskbar,0,0,0,16);
  } else {
    ((JCnt*)taskbar)->Orient = JCntF_TopBottom;
    JWSetBounds(taskbar,0,0,16,0);
  }

  JWndSetProp(taskbar);
  JAppSetMain(appl,taskbar);

  JWinShow(taskbar);
  newThread(makecbutton,STACK_DFL,NULL);
  retexit(0);

  newThread(listener,STACK_DFL,NULL);

  while(1) {
    rcvid = recvMsg(appchannel,(void *)&msg);
    type = *(int *)msg;

    //printf("got appchannel msg: #%d\n",type);

    switch(type) {
      case WIN_EventRecv:
        JAppDrain(appl);
      break;
      case REFRESH:
        JWinLayout(taskbar);
      break;
      case SETCOL:
        but    = *(void **)(msg+2);
        icocol = *(int *)(msg+6);

        memset(((JBmp*)but)->Cols,icocol,4);
        /*
        ((JBmp*)but)->Cols[0] = icocol;
        ((JBmp*)but)->Cols[1] = icocol;
        ((JBmp*)but)->Cols[2] = icocol;
        ((JBmp*)but)->Cols[3] = icocol;
        */
        JWReDraw(but);
      break;
    }
    replyMsg(rcvid,0);
  }
}
