#include <winlib.h>

void *TxtArea;	// needs to be global now

void Clicked(void *widget, int type);	// forward declaration

int main() {
	void *Appl,*Window,*Button;
	
	Appl = JAppInit(NULL,0);
	Window = JWndInit(NULL, NULL, 0, "This is the tutorial", JWndF_Resizable);
	JAppSetMain(Appl, Window);
	
	TxtArea = JTxtInit(NULL, Window, 0, "");
	JWinGeom(TxtArea, 0, 8, 0, 16, GEOM_TopLeft | GEOM_BotRight2);
	JWinSetBack(TxtArea, COL_MedGrey);
	JTxtAppend(TxtArea, "This is the scrollable textfield\n");

	Button = JButInit(NULL, Window, 0, "Press me");
	JWinMove(Button, 0, 16, GEOM_BotLeft);
	JWinAttach(Button, EVT_Entered, Clicked); 
	
	JWinShow(Window);
	JAppLoop(Appl);
}

void Clicked(void *widget, int type) {
	JTxtAppend(TxtArea,"You clicked the button!\n");
}
