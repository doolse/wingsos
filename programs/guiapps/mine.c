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
	JWinReDraw(Board);
}

int main() {
	void *Appl,*Window,*But;
	
	bmem = xmalloc(65536);
	Appl = JAppInit(NULL,0);
	Window = JWndInit(NULL, NULL, 0, "Minesweeper", JWndF_Resizable);
	JAppSetMain(Appl, Window);
	Board = JWinInit(NULL, 0, 24, 8,8, Window, WEV_MotionBut|WEV_Button, 0);
	But = JButInit(NULL, Window, 0, "Replay");
	JWinMove(But, 0,0, 0);
	JWinOveride(But, MJBut_Clicked, Replay);
	JWinOveride(Board, MJW_Draw, DrawBoard);
	JWinOveride(Board, MJW_Button, DoButton);
	JWinOveride(Board, MJW_RButton, DoRButton);
	JWinSize(Board, boardx*8+8, boardy*8+8);
	PrepBoard();
	JWinShow(Window);
	JAppLoop(Appl);
}

