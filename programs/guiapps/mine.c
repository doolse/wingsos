#include <winlib.h>
#include <wgslib.h>
#include <stdlib.h>
#include <string.h>
#include <wgs/obj.h>

char *bmem;
JWin *Board;
int boardx=16;
int boardy=16;
int mines=40;
int died=0;

unsigned char app_icon[] = {
0,1,17,57,31,12,25,123,
0,128,136,220,248,48,152,222,
123,25,12,31,57,17,1,0,
222,152,48,248,220,136,128,0,
0x01,0x01,0x01,0x01
};

extern void DrawBoard();
extern void DoButton();
extern void DoRButton();

void Replay() {
	died=0;
	PrepBoard();
	JWReDraw(Board);
}

void main(int argc, char * argv[]) {
	void *Appl,*Window,*But;
	void *newclass;
	JCnt *butcnt;
	SizeHints sizes;
	JMeta * metadata = malloc(sizeof(JMeta));
	
	bmem = malloc(65536);
	Appl = JAppInit(NULL,0);

	metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
	metadata->title = "MineSweeper";
	metadata->icon = app_icon;
	metadata->showicon = 1;
	metadata->parentreg = -1;

	Window = JWndInit(NULL, "Minesweeper", JWndF_Resizable,metadata);
	((JCnt *)Window)->Orient = JCntF_TopBottom;
	JAppSetMain(Appl, Window);
	
	newclass = JSubclass(&JWClass, -1, 
			METHOD(MJW, Draw), DrawBoard, 
			METHOD(MJW, Button), DoButton,
			METHOD(MJW, RButton), DoRButton, -1);
	Board = JNew(newclass);
	JWInit(Board, boardx*8,boardy*8, WEV_MotionBut|WEV_Button, 0);
	butcnt = JCntInit(NULL);
	But = JButInit(NULL, "Replay");
	JCntAdd(butcnt, But);
	
	JCntAdd(Window, butcnt);
	JCntAdd(Window, Board);
	
	JWinCallback(But, JBut, Clicked, Replay);
	JCntGetHints(Window, &sizes);
	JWSetMax(Window,sizes.PrefX,sizes.PrefY);
        JWSetMin(Window,64,48);
	JWSetBounds(Window, 32,32, sizes.PrefX, sizes.PrefY);
        JWndSetProp(Window);
	PrepBoard();
	JWinShow(Window);

        retexit(1);
	JAppLoop(Appl);
}

