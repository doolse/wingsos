#include <winlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <wgs/obj.h>
#include <wgs/util.h>

#include <string.h>

#define CMD_STARTPROG      0x1000
#define CMD_CHANGEBACKBMP  0x1001
#define CMD_PS             0x1002
#define CMD_MEM            0x1003

#define backsize 320*25+1000

void *backimgwidget,*backbmp;

MenuData progmenu[]={
  {"ajirc",    0, NULL, 0, CMD_STARTPROG, NULL, NULL},
  {"mine",     0, NULL, 0, CMD_STARTPROG, NULL, NULL}, 
  {"search",   0, NULL, 0, CMD_STARTPROG, NULL, NULL},
  {"wordserve",0, NULL, 0, CMD_STARTPROG, NULL, NULL},
  {"launch",   0, NULL, 0, CMD_STARTPROG, NULL, NULL},
  {NULL,       0, NULL, 0, 0,             NULL, NULL}
};

MenuData toolsmenu[]={
  {"Process List", 0, NULL, 0, CMD_PS,  NULL, NULL},
  {"Memory Info",  0, NULL, 0, CMD_MEM, NULL, NULL},
  {NULL,           0, NULL, 0, 0,       NULL, NULL}
};

MenuData backpics[]={
  {"backimg.hbm",0,NULL,0,CMD_CHANGEBACKBMP,NULL,NULL},
  {"ferrari.hbm",0,NULL,0,CMD_CHANGEBACKBMP,NULL,NULL},
  {"penguin.hbm",0,NULL,0,CMD_CHANGEBACKBMP,NULL,NULL},
  {"jos.hbm",    0,NULL,0,CMD_CHANGEBACKBMP,NULL,NULL},
  {NULL,0,NULL,0,0,NULL,NULL}
};

MenuData rootmenu[]={
  {"Programs",         0, NULL, 0, 0,             NULL, progmenu},
  {"Tools",            0, NULL, 0, 0,             NULL, toolsmenu},
  {"Change Background",0, NULL, 0, 0,             NULL, backpics},
  {"credits",          0, NULL, 0, CMD_STARTPROG, NULL, NULL}, 
  {NULL,               0, NULL, 0, 0,             NULL, NULL}
};

// TODO
// Remove JMan from the vector, when theyre killed

typedef struct JMan {
	JCnt JCntParent;
	char *Label;
	int Flags;
	int ButAbsX;
	int ButAbsY;
	int DragX;
	int DragY;
	int Flags2;
	int RestX;
	int RestY;
	unsigned int RestXS;
	unsigned int RestYS;
	int DragType;
	int Region;
} JMan;

extern JMan *JManInit(JWin *self, char *title, int wndflags, int region);
extern void JManDraw(JWin *Self);
extern void JManNotify(JWin *Self, int Type);
extern void JManButton(JWin *Self, int SubType, int X, int Y, int XAbs, int YAbs);
extern void JManMotion(JWin *Self, int SubType, int X, int Y, int XAbs, int YAbs);

unsigned char icon[] = {
0xff, 0x80, 0x83, 0x83, 0x80, 0x80, 0x80, 0x80,
0xff, 0x01, 0xfd, 0xfd, 0x71, 0x71, 0x71, 0x71,
0x80, 0xb8, 0xbe, 0x9f, 0x8f, 0x83, 0x80, 0xff,
0x71, 0x71, 0x71, 0xf1, 0xe1, 0xc1, 0x01, 0xff,
0x50, 0x50, 0x50, 0x50
};

unsigned char icon_on[] = {
0x7f, 0xc0, 0x83, 0x83, 0x80, 0x80, 0x80, 0x80,
0xfe, 0x03, 0xfd, 0xfd, 0x71, 0x71, 0x71, 0x71,
0x80, 0xb8, 0xbe, 0x9f, 0x8f, 0x83, 0xc0, 0x7f,
0x71, 0x71, 0x71, 0xf1, 0xe1, 0xc1, 0x03, 0xfe,
0xcb, 0xcb, 0xcb, 0xcb
};

JMan *LastFoc = NULL;
void *JManClass;
Vec *winList;

JMan *findTopLevel(int region)
{
    RegInfo props;    
    Vec *list = winList;
    uint i;
    JMan *man;
    char *name = "JOS";
    int flags = 0;
    uint sz = VecSize(list);
    
    JRegInfo(region, &props);
    for (i=0; i<sz; i++)
    {
	man = VecGet(list, i);
	if (man->Region == region)
	    return man;
    }
    
    if (props.Hasprop) {
	name = props.Name;
	flags = props.Flags;
    }
//    printf("Creating a new JMan for %d with flags %d\n",region,flags);
    man = JManInit(NULL, name, flags, region);
    JWRePare(man, region);
    VecAdd(list, man);
    return man;
}

void JManDoFocus(JMan *Self)
{
    if (Self == LastFoc)
	return;
    if (Self)
    {
	((JW *)Self)->Flags |= JF_Focused;
	JReqFocus(Self->Region);
	JWToFront(Self);
	JWReDraw(Self);
    }
    if (LastFoc)
    {
	((JW *)LastFoc)->Flags &= ~JF_Focused;
	JWReDraw(LastFoc);
    }
    LastFoc = Self;
}

void showMan(JMan *Self)
{
    int From = Self->Region;
    RegInfo props;
//    printf("Getting info for %d\n", From);
    JRegInfo(From, &props);
    if (props.Reg.X < 8)
	    props.Reg.X = 8;
    if (props.Reg.Y < 8)
	    props.Reg.Y = 8;
    if (props.Hasprop) {
	    JWSetMin(Self, props.MinX+16, props.MinY+16);
	    JWSetMax(Self, props.MaxX+16, props.MaxY+16);
    }
//    printf("Geom is %d,%d,%d,%d\n", props.Reg.X, props.Reg.Y, props.Reg.XSize, props.Reg.YSize);
    JWSetBounds(Self, props.Reg.X-8, props.Reg.Y-8, props.Reg.XSize+16, props.Reg.YSize+16);
    JEGeom(From, 8, 8, props.Reg.XSize, props.Reg.YSize);
    JShow(From);
    JWinShow(Self);
    JManDoFocus(Self);
}

void JManNotice(JWin *Self, int SubType, int From, void *data) 
{
//    printf("JMan Notice: %d\n", SubType);
    switch (SubType)
    {
	case EVS_Deleted:
	    if (Self == LastFoc)
		LastFoc = NULL;
	    VecRemove(winList, Self);
	    JCntKill(Self);
	    break;
	case EVS_Hidden:
	    JWinHide(Self);
	    break;
	case EVS_ReqShow:
	    showMan(Self);
	    break;
    }
}

void WinNotice(JWin *Self, int type, int region, char *data) {
    JMan *man;
    
//    printf("WinMan Notice: Region %d, %d\n",region,type);
    switch (type)
    {
	case EVS_PropChange:
//	    printf("Called %s\n", props.Name);
	    break;
	case EVS_Added:	    
	    break;
	case EVS_ReqShow:
	    man = findTopLevel(region);
	    showMan(man);
	    break;
    }
}


void menuhandler(void *self, MenuData * item) {
  FILE * fp;

  switch(item->command) {
    case CMD_STARTPROG:
      spawnlp(0,item->name,NULL);
    break;
    case CMD_PS:
      system("ps |guitext -w 208 -h 80");
    break;
    case CMD_MEM:
      system("mem |guitext -h 56 -w 156");
    break;

    case CMD_CHANGEBACKBMP:
      chdir(getappdir());
      fp = fopen(item->name,"rb");
      fseek(fp, 2, SEEK_CUR);
      fread(backbmp, 1, backsize, fp);
      fclose(fp);
      JWReDraw(backimgwidget);
    break;
  }

}

void rightclick(void *self, int Type,int X, int Y, int XAbs, int YAbs) {
  int xy[2];

  JWAbs(self,xy);

  JWinShow(JMnuInit(NULL,rootmenu,xy[0],xy[1]+16,menuhandler));
}

int main() {

    void *window;
    void *App;
    JWin *WinMan,*butcon,*temp;
    FILE *back;

    chdir(getappdir());
    App = JAppInit(NULL,0);
    back = fopen("backimg.hbm","rb");
    if (!back) {
	    perror("winman");
	    exit(1);
    }
    backbmp = malloc(backsize);
    fseek(back, 2, SEEK_CUR);
    fread(backbmp, 1, backsize, back);
    fclose(back);

    winList = VecInit(NULL);
    JManClass =  JSubclass(&JCntClass, sizeof(JMan), 
		    METHOD(MJW, Draw), JManDraw, 
		    METHOD(MJW, Notify), JManNotify, 
		    METHOD(MJW, Button), JManButton, 
		    METHOD(MJW, Motion), JManMotion, 
		    METHOD(MJW, Notice), JManNotice, 
		    -1);
    WinMan = JNew(JSubclass(&JCntClass, -1, METHOD(MJW, Notice), WinNotice, -1));
    JCntInit(WinMan);
    ((JCnt *)WinMan)->Orient = JCntF_TopBottom;
    JWSetBounds(WinMan, 0,0, 320,200);

    butcon = JCntInit(NULL);
    temp = JIbtInit(NULL,16,16,icon,icon_on);
    JWinCallback(temp,JBut,Clicked,rightclick);
    JCntAdd(butcon,temp);
    JCntAdd(WinMan,butcon);
    //temp = JButInit(NULL, butcon, 0, "Ajirc V1.0..");

    backimgwidget = JBmpInit(NULL, 320,200, backbmp);
    JCntAdd(WinMan, backimgwidget);

    JWinShow(WinMan); 
    // Request to be the window manager!
    JWReq(WinMan);

    retexit(0);
    JAppLoop(App);
    return 0;
}
