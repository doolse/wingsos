#include <winlib.h>
#include <stdlib.h>

char *bmem;
JWin *Board;
int boardx=16;
int boardy=16;
int mines=40;
int died=0;

extern void DrawBoard();
extern void DoButton();
extern void DoRButton();

void Replay() {
	died=0;
	PrepBoard();
	JWReDraw(Board);
}

void ExitMine() {
  exit(1);
}

int main() {
	void *Appl,*Window,*But, *ExitBut;
	void *newclass;
	JCnt *butcnt;
	SizeHints sizes;
	
	bmem = malloc(65536);
	Appl = JAppInit(NULL,0);
	Window = JWndInit(NULL, "Minesweeper", JWndF_Resizable);
	((JCnt *)Window)->Orient = JCntF_Vert;
	JAppSetMain(Appl, Window);
	
	newclass = JSubclass(&JWClass, -1, 
			METHOD(MJW, Draw), DrawBoard, 
			METHOD(MJW, Button), DoButton,
			METHOD(MJW, RButton), DoRButton, -1);
	Board = JNew(newclass);
	JWInit(Board, boardx*8,boardy*8, WEV_MotionBut|WEV_Button, 0);
	butcnt = JCntInit(NULL);
	But = JButInit(NULL, "Replay");
        ExitBut = JButInit(NULL, "Exit");
	JCntAdd(butcnt, But);
	JCntAdd(butcnt, ExitBut);
	
	JCntAdd(Window, butcnt);
	JCntAdd(Window, Board);
	
	JWinCallback(But, JBut, Clicked, Replay);
        JWinCallback(ExitBut, JBut, Clicked, ExitMine);
	JCntGetHints(Window, &sizes);
	JWSetBounds(Window, 0,0, sizes.PrefX, sizes.PrefY);
	PrepBoard();
	JWinShow(Window);

        retexit(1);
	JAppLoop(Appl);
}

