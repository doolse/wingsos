#include <winlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <wgs/obj.h>
#include <wgs/util.h>

#define backsize 320*25+1000

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
0x7f, 0xc0, 0x83, 0x83, 0x80, 0x80, 0x80, 0x80,
0xfe, 0x03, 0xfd, 0xfd, 0x71, 0x71, 0x71, 0x71,
0x80, 0xb8, 0xbe, 0x9f, 0x8f, 0x83, 0xc0, 0x7f,
0x71, 0x71, 0x71, 0xf1, 0xe1, 0xc1, 0x03, 0xfe,
0xbc, 0xbc, 0xbc, 0xbc
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
	    JWSetMin(Self, props.MinX+8, props.MinY+8);
	    JWSetMax(Self, props.MaxX+8, props.MaxY+8);
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
	    JWKill(Self);
	    break;
	case EVS_Hidden:
	    JWHide(Self);
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

int main() {

    void *window;
    void *App;
    JWin *WinMan,*butcon,*temp;
    FILE *back;
    void *backbmp;

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
    JWSetBounds(WinMan, 0,0, 320,200);
    JCntAdd(WinMan, JBmpInit(NULL, 320,200, backbmp));

/*	butcon = JCntInit(NULL);
    temp = JIcoInit(NULL, butcon, 0, 16, 16, icon);
    temp = JButInit(NULL, butcon, 0, "Ajirc V1.0.."); */
    JWinShow(WinMan); 
    // Request to be the window manager!
    JWReq(WinMan);

    retexit(0);
    JAppLoop(App);
    return 0;
}
