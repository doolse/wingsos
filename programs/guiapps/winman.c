#include <winlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <wgs/obj.h>

#define backsize 320*25+1000

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

extern JWin *JManInit(JWin *self, char *title, int wndflags, int region);
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


void JManNotice(JWin *Self, int SubType, int From, void *data) {
	printf("Notice %d\n", SubType);
	if (SubType == EVS_Deleted) {
		if (Self == LastFoc)
			LastFoc = NULL;
		JWKill(Self);
	}
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

void WinNotify(JWin *Self, int type, int region, char *data) {
	RegInfo props;
	JWin *temp;
	char *name = "JOS";
	int flags = 0;
	
	JRegInfo(region, &props);
	printf("Region %d, %d\n",region,type);
	if (type == 3) {
		printf("Called %s\n", props.Name);
	} else
	if (type == 7) {
		if (props.Hasprop) {
			name = props.Name;
			flags = props.Flags;
		}
		printf("The flags are %d\n",flags);
		temp = JManInit(NULL, name, flags, region);
		if (props.Reg.X < 8)
			props.Reg.X = 8;
		if (props.Reg.Y < 8)
			props.Reg.Y = 8;
		if (props.Hasprop) {
			JWSetMin(temp, props.MinX+8, props.MinY+8);
			JWSetMax(temp, props.MaxX+8, props.MaxY+8);
		}
		JWSetBounds(temp, props.Reg.X-8, props.Reg.Y-8, props.Reg.XSize+16, props.Reg.YSize+16);
		JWRePare(temp, region);
		JEGeom(region, 8, 8, props.Reg.XSize, props.Reg.YSize);
		JShow(region);
		JWinShow(temp);
		JManDoFocus(temp);
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
	
	JManClass =  JSubclass(&JCntClass, sizeof(JMan), 
			METHOD(MJW, Draw), JManDraw, 
			METHOD(MJW, Notify), JManNotify, 
			METHOD(MJW, Button), JManButton, 
			METHOD(MJW, Motion), JManMotion, 
			METHOD(MJW, Notice), JManNotice, 
			-1);
	WinMan = JNew(JSubclass(&JCntClass, -1, METHOD(MJW, Notice), WinNotify, -1));
	JCntInit(WinMan);
	JWSetBounds(WinMan, 0,0, 320,200);
	JCntAdd(WinMan, JBmpInit(NULL, 320,200, backbmp));
	
/*	butcon = JCntInit(NULL);
	temp = JIcoInit(NULL, butcon, 0, 16, 16, icon);
	temp = JButInit(NULL, butcon, 0, "Ajirc V1.0.."); */
	JWinShow(WinMan); 
	// Request to be the window manager!
	JWReq(WinMan);
	
	retexit(1);
	JAppLoop(App);
}
