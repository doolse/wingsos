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
TNode tnode;
char *Name;
} OurModel;
	
int main(int argc, char *argv[])
{
    JW *wnd;
    JCombo *list;
    JScr *scr;
    JLModel *Model;
    OurModel *row;
    uint i=0;
    char testname[16];
    
    JAppInit(NULL, 0);
    Model = JLModelInit(NULL);
    wnd = JWndInit(NULL, "Test list", JWndF_Resizable);
    for (i=0; i<55; i++)
    {
	row = calloc(sizeof(OurModel), 1);
	sprintf(testname, "Row %d", i);
	row->Name = strdup(testname);
	printf("name '%s'\n", testname);
        JLModelAppend(Model, (TNode *)row);
    }
    list = JComboInit(NULL, (TModel *)Model, (uint32)OFFSET(OurModel, Name), JColF_STRING);
//    scr = JScrInit(NULL, list, JScrF_VNotEnd);
//    JListAddColumns(list, NULL, 
//	    "Name", OFFSET(OurModel, Name), 120, JColF_STRING, 
//	    NULL); 
    JCntAdd(wnd, list);
    JWinShow(wnd);
    JAppLoop(NULL);
}

