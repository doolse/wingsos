#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <winlib.h>
#include <winforms.h>
#include <wgslib.h>

/*
void clicked()
{
    printf("You just clicked it!\n");
}

int main (int argc, char *argv[])
{
	JW *wnd;
	JTab *tab;
	SizeHints sizes;
	HTMLTable *Table;
	HTMLForms *Forms;
	JBut *button;
	
	chdir(getappdir());
	Forms = JFormLoad("xmltest.xml");
	JAppInit(NULL, 0);
	wnd = JWndInit(NULL, "Title", JWndF_Resizable);
	Table = JFormGetTable(Forms, "main");
	tab = JFormCreate(Table);
	button = JFormGetControl(Table, "ok");
	JWinCallback(button, JBut, Clicked, clicked);
	JWinGetHints(tab, &sizes);
	JCntAdd(wnd, tab);
	JWSetBounds(wnd, wnd->X, wnd->Y, sizes.PrefX, sizes.PrefY);
	JWinShow(wnd);
	JAppLoop(NULL);
}
*/

typedef struct OurModel {
JListRow treerow;
char *Name;
} OurModel;

JTreeRow Model;
	
int main(int argc, char *argv[])
{
    JW *wnd;
    JList *list;
    JScr *scr;
    OurModel *row;
    uint i=0;
    char testname[16];
    
    JAppInit(NULL, 0);
    wnd = JWndInit(NULL, "Test list", JWndF_Resizable);
    for (i=0; i<55; i++)
    {
	row = calloc(sizeof(OurModel), 1);
	sprintf(testname, "Row %d", i);
	row->Name = strdup(testname);
	printf("name '%s'\n", testname);
        JListAppend(&Model, row);
    }
    list = JListInit(NULL, &Model);
    scr = JScrInit(NULL, list, JScrF_VNotEnd);
    JListAddColumns(list, NULL, 
	    "Name", OFFSET(OurModel, Name), 120, JColF_STRING, 
	    NULL); 
    JCntAdd(wnd, scr);
    JWinShow(wnd);
    JAppLoop(NULL);
}
