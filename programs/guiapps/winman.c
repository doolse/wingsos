#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <taskbar.h>
#include <unistd.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <winlib.h>
#include <winman.h>
#include <xmldom.h>

int appchannel,taskbarprocid;
void *backimgwidget,*backbmp;
char *taskbarpos,*backimgname;

extern JMan *JManInit(JWin *self, char *title, int wndflags, int region, int parentreg,int showicon);
extern void JManDraw(JWin *Self);
extern void JManNotify(JWin *Self, int Type);
extern void JManButton(JWin *Self, int SubType, int X, int Y, int XAbs, int YAbs);
extern void JManMotion(JWin *Self, int SubType, int X, int Y, int XAbs, int YAbs);

JMan *LastFoc = NULL;
void *JManClass;
void *WinManClass;
Vec *winList;

int mwidth=320,mheight=200,mleft=0,mtop=0;

JMan * getJManForRegion(int region) {
  RegInfo props;    
  JMan *man;
  uint i;
  Vec *list = winList;

  //default JMan Attributes
  char *name    = "Application";
  int showicon  =  1;
  int parentreg = -1;
  int flags     =  0;
    
  JRegInfo(region, &props);
  for(i=0; i<VecSize(list); i++) {
    man = VecGet(list, i);
    if(man->Region == region)
      return(man);
  }
    
  if(props.Hasprop) {
    name  = props.Name;
    flags = props.Flags;

    if(props.metadata) {
      parentreg = props.metadata->parentreg;
      showicon  = props.metadata->showicon;
    } else {
      parentreg = -1;
      showicon = 1;
    }
  }

  man = JManInit(NULL, name, flags, region, parentreg, showicon);
  JWRePare(man, region);
  VecAdd(list, man);
  return(man);
}

void JManDoFocus(JMan *Self) {
  int fd,i;
  JMan * temp;

  //Already in focus...
  if(Self == LastFoc)
    return;

  if(Self) {

    //Update the taskbar icon colour
    if(Self->parentreg == -1) {
      if((fd = open("/sys/taskbar",O_PROC)) != -1) {
        sendCon(fd,TSKM_INFOCUS,0,Self);
        close(fd);
      }
    }

    //Bring it into focus, "Unminimize" if necessary
    ((JW *)Self)->Flags |= JF_Focused;
    JReqFocus(Self->Region);
    if(Self->Flags2 & JManF_Minimized) {
      Self->Flags2 &= ~JManF_Minimized;
      JWinShow(Self);
    }

    //If the last window was a different app, 
    //Find hidden children of this app, reveal them and bring them to front.
    if(!LastFoc || LastFoc->parentreg == -1) {
      for(i=0;i<VecSize(winList);i++) {
        temp = VecGet(winList,(uint)i);
        if(temp != Self && temp->parentreg == Self->Region) {
          JWinShow(temp);
          JWToFront(temp);
        }
      }
    }

    //Finally bring this window to front
    JWToFront(Self);
    JWReDraw(Self);
  }

  if(LastFoc) {
    ((JW *)LastFoc)->Flags &= ~JF_Focused;
    JWReDraw(LastFoc);

    //if the new window selected is not a child...
    if(Self->parentreg == -1 && LastFoc->parentreg != Self->Region) {
      if(LastFoc->parentreg == -1) {
        //printf("selected a different parent window. last was a parent\n");

        for(i=0;i<VecSize(winList);i++) {
          temp = VecGet(winList,(uint)i);
          if(temp->parentreg == LastFoc->Region) {
            JWinHide(temp);
          }
        }
      } else {
        //printf("selected a different parent window. last was a child\n");

        for(i=0;i<VecSize(winList);i++) {
          temp = VecGet(winList,(uint)i);
          if(temp->parentreg == LastFoc->parentreg) {
            JWinHide(temp);
          }
        }
      }
    } //else 
      //printf("selected a child, or the parent of the last child.\n");
  }

  LastFoc = Self;
}

void revealMan(JMan *Self) {
  JShow(Self->Region);
  JWinShow(Self);
  JManDoFocus(Self);
}

void showMan(JMan *Self) {
  RegInfo props;
  int From = Self->Region;

  JRegInfo(From, &props);

  if(props.Reg.X < 8)
    props.Reg.X = 8;

  if(props.Reg.Y < 8)
    props.Reg.Y = 8;

  if(props.Hasprop) {
    JWSetMin(Self, props.MinX+16, props.MinY+16);
    JWSetMax(Self, props.MaxX+16, props.MaxY+16);
  }

  JWSetBounds(Self, props.Reg.X-8, props.Reg.Y-8, props.Reg.XSize+16, props.Reg.YSize+16);
  JEGeom(From, 8, 8, props.Reg.XSize, props.Reg.YSize);
  JShow(From);
  JWinShow(Self);
  JManDoFocus(Self);
}

void JManNotice(JWin *Self, int SubType, int From, void *data)  {
  int fd,i;
  JMan * temp;

  //printf("JMan Notice: %d\n", SubType);

  switch (SubType){
    case EVS_Deleted:  /* CMD_CLOSE */
      if(((JMan*)Self)->parentreg == -1) {
        if((fd = open("/sys/taskbar",O_PROC)) != -1) {
          sendCon(fd,TSKM_KILLED,0,Self);
          close(fd);
        }
      }
      if (Self == LastFoc)
        LastFoc = NULL;
      VecRemove(winList, Self);
      JCntKill(Self);
    break;

    case EVS_Hidden: /* CMD_MINI */
      if(((JMan*)Self)->parentreg == -1) {
        if((fd = open("/sys/taskbar",O_PROC)) != -1) {
          sendCon(fd,TSKM_MINIMIZED,0,Self);
          close(fd);
        }
        if(Self == LastFoc)
          LastFoc = NULL;
        JWinHide(Self);

        printf("Clicked minimize on %d\n",((JMan*)Self)->Region);

        for(i=0;i<VecSize(winList);i++) {
          temp = VecGet(winList,(uint)i);
          if(temp != Self && temp->parentreg == ((JMan*)Self)->Region) {
          //printf("found a window, parentreg == %d\n",temp->parentreg);
            JWinHide(temp);
          } 
        }
      } else {
        //conceal the child window
        if(Self == LastFoc)
          LastFoc = NULL;
        JWinHide(Self);
      }
    break;

    case EVS_ReqShow: 
      revealMan(Self);
    break;
  }
}

void WinNotice(JWin *Self, int type, int region, char *data) {
  int fd;
  JMan *man;
    
  switch (type) {
    //A Window is created for the first time. 
    case EVS_ReqShow:
      man = getJManForRegion(region);
      if(man->parentreg == -1) {
        if((fd = open("/sys/taskbar",O_PROC)) != -1) {
          sendCon(fd,TSKM_NEWITEM,(int)TSKM_INFOCUS,man);
          close(fd);
        } 
      }
      showMan(man);
    break;
  }
}

void nextappwinfocus() {
  RegInfo props;
  char * pathstr;
  uint i,lastfocindex;
  JMan * temp;

  if(VecSize(winList) > 1 && LastFoc) {
    JRegInfo(LastFoc->Region, &props);
    pathstr = props.metadata->launchpath;
    lastfocindex = i = VecIndexOf(winList,LastFoc);
    while(1) {
      i++;
      if(i == lastfocindex)
	break;
      if(i >= VecSize(winList))
        i = 0;

      temp = VecGet(winList,i);
      if(((JW*)temp)->HideCnt == 1)
        continue;
      JRegInfo(temp->Region,&props);
      if(!strcmp(pathstr,props.metadata->launchpath))
        break;
    }

    if(i != lastfocindex)
      JManDoFocus(temp);
  }
}

void listener() {
  int channel,rcvid,returncode,fd,j;
  uint i,lastfocindex;
  JMan * temp;
  FILE *back;
  msgpass * msg = malloc(sizeof(msgpass));

  channel = makeChanP("/sys/winman");

  while(1) {
    rcvid = recvMsg(channel,(void *)&msg);

    switch(msg->code) {
      case TSKM_NEXTWIN:
        if(VecSize(winList) > 1 && LastFoc) {
          lastfocindex = i = VecIndexOf(winList,LastFoc);
          do {
            i++;
            if(i == VecSize(winList))
              i = 0;
            temp = VecGet(winList,i);
            j = ((JW*)temp)->HideCnt;
          } while(j != 0 && i != lastfocindex);
          if(i != lastfocindex)
            newThread(JManDoFocus,STACK_DFL,temp);
        }
        returncode = 0;
      break;

      case TSKM_NEXTAPPWIN:
	newThread(nextappwinfocus,STACK_DFL,NULL);
        returncode = 0;
      break;

      case TSKM_INFOCUS:
        //printf("winman: received infocus msg\n");
        JManDoFocus(msg->mainwin);
        sendChan(appchannel,TSKM_INFOCUS,msg->mainwin);
        returncode = 0;
      break; 

      case TSKM_ONLEFT:
        mwidth = 304;
        mheight = 200;
        mleft = 16;
        mtop = 0;
      break;

      case TSKM_ONRIGHT:
        mwidth = 304;
        mheight = 200;
        mleft = 0;
        mtop = 0;
      break;

      case TSKM_ONTOP:
        mwidth = 320;
        mheight = 184;
        mleft = 0;
        mtop = 16;
      break;

      case TSKM_ONBOTTOM:
        mwidth = 320;
        mheight = 184;
        mleft = 0;
        mtop = 0;
      break;

      case TSKM_GETALL:
        returncode = VecSize(winList);
      break;

      case TSKM_GETNUM:
        temp = VecGet(winList,((uint)(msg->getnum)));
        if(temp->parentreg == -1) {
          if((fd = open("/sys/taskbar",O_PROC)) != -1) {
            if(temp == LastFoc) {
              //printf("winman: this icon in focus\n");
              sendCon(fd,TSKM_NEWITEM,(int)TSKM_INFOCUS,temp);
            } else {
              if(((JW*)temp)->HideCnt) {
                //printf("winman: this icon is minimized\n");
                revealMan(temp);
                sendCon(fd,TSKM_NEWITEM,(int)TSKM_MINIMIZED,temp);
              } else {
                //printf("winman: this icon in blur\n");
                sendCon(fd,TSKM_NEWITEM,(int)TSKM_INBLUR,temp);
              }
            }
            close(fd);
          } 
        }
        returncode = 0;
      break;

      case TSKM_UPDATEBG:
        back = fopen(msg->str,"rb");
        if(back) {
          fseek(back, 2, SEEK_CUR);
          fread(backbmp, 1, backsize, back);
          fclose(back);
          JWReDraw(backimgwidget);
        }
      break;

      case TSKM_RELAUNCHTASKBAR:
        kill(taskbarprocid,1);
        taskbarprocid = spawnlp(0,"taskbar",msg->str,NULL);
      break;

      case IO_OPEN:
        if(*(int *)  (((char *)msg)+6) & (O_PROC|O_STAT))
          returncode = makeCon(rcvid,1);
        else
          returncode = -1;
      break;
    }

    replyMsg(rcvid,returncode);
  }
}

void main() {
    void *window;
    void *App;
    JWin *WinMan;
    FILE *back;
    DOMElement * configxml, * confignode;
    char * configpath;

    int type,rcvid;
    char * msg;
    JMan * man;

    //Do window manager configuration
    chdir(getappdir());
    configpath = fpathname("winmanconfig.xml",getappdir(),1);
    configxml  = XMLloadFile(configpath);

    confignode = XMLgetNode(configxml,"xml/taskbar");
    taskbarpos = XMLgetAttr(confignode,"position");

    confignode  = XMLgetNode(configxml,"xml/backimg");
    backimgname = XMLgetAttr(confignode,"filename");

    appchannel = makeChan();
    App = JAppInit(NULL,appchannel);

    backbmp = malloc(backsize);
    back = fopen(backimgname,"rb");
    if(back) {
      fseek(back, 2, SEEK_CUR);
      fread(backbmp, 1, backsize, back);
      fclose(back);
    } else {
      //to be expanded to allow a selection of patterns for backgrounds
      memset(backbmp,0xff,backsize);
    }

    //winList maintains a list of Managed Windows (JMan pointers)
    winList = VecInit(NULL);

    //this defines the class for individual Managed Windows
    JManClass =  JSubclass(&JCntClass, sizeof(JMan), 
		    METHOD(MJW, Draw),   JManDraw, 
		    METHOD(MJW, Notify), JManNotify, 
		    METHOD(MJW, Button), JManButton, 
		    METHOD(MJW, Motion), JManMotion, 
		    METHOD(MJW, Notice), JManNotice, 
		    -1);

    //this is the class for the whole window manager
    WinManClass = JSubclass(&JCntClass, -1, 
                    METHOD(MJW, Notice), WinNotice, 
                    -1);

    WinMan = JNew(WinManClass);
    JCntInit(WinMan);

    ((JCnt *)WinMan)->Orient = JCntF_TopBottom;
    JWSetBounds(WinMan, 0,0, SCRWIDTH,SCRHEIGHT);

    backimgwidget = JBmpInit(NULL,SCRWIDTH,SCRHEIGHT,backbmp);
    JCntAdd(WinMan, backimgwidget);

    JWinShow(WinMan); 
    // Request to be the window manager!
    JWReq(WinMan);

    retexit(0);

    newThread(listener,STACK_DFL,NULL);

    taskbarprocid = spawnlp(0,"taskbar",taskbarpos,NULL);

    while(1) {
      rcvid = recvMsg(appchannel,(void *)&msg);
      type = * (int *)msg;

      switch(type) {
        case WIN_EventRecv:
          JAppDrain(App);
        break;
        case TSKM_INFOCUS:
          man = *(JMan**)(msg+2);
          JManDoFocus(man);          
        break;
      }
      replyMsg(rcvid,0);
    }
}
