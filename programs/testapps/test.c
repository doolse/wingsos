#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <winlib.h>
#include <winforms.h>
#include <wgslib.h>

int main (int argc, char *argv[])
{
	JW *wnd;
	JTab *tab;
	SizeHints sizes;
	HTMLTable *Table;
	HTMLForms *Forms;
	
	chdir(getappdir());
	Forms = JFormLoad("xmltest.xml");
	JAppInit(NULL, 0);
	wnd = JWndInit(NULL, "Title", JWndF_Resizable);
	Table = JFormGetTable(Forms, "main");
	tab = JFormCreate(Table);
	JWinGetHints(tab, &sizes);
	JCntAdd(wnd, tab);
	JWSetBounds(wnd, wnd->X, wnd->Y, sizes.PrefX, sizes.PrefY);
	JWinShow(wnd);
	JAppLoop(NULL);
}
