#include <winlib.h>
#include <string.h>
#include <stdlib.h>
#include <xmldom.h>
#include <wgslib.h>
#include <winforms.h>

static int decodeSize(char *size)
{
    char *end;
    uint32 val;
    if (!*size)
	return JTabF_Preferred;
    val = strtoul(size, &end, 0);
    if (end == size)
    {
	if (!strcmp("fill", size))
	    return JTabF_Fill;
	if (!strcmp("min", size))
	    return JTabF_Minimum;
    	return JTabF_Preferred;
    }
    if (*end == '%')
	return -val;
    return val;
}

static int decodeAlign(char *align)
{
    if (!*align)
	return 0;
    if (!strcmp(align, "center"))
	return JTabF_Center;
    if (!strcmp(align, "left"))
	return JTabF_Left;
    if (!strcmp(align, "right"))
	return JTabF_Right;
    return 0;
}

HTMLTable *JFormDoTable(DOMElement *table)
{
    DOMElement *row,*cell;
    uint nrows;
    HTMLTable *Tab;
    HTMLRow *HRow;
    uint rows=0;
    uint maxcol=0;
    uint colup;
    Tab = calloc(sizeof(HTMLTable), 1);
    for (row=table->Elements; row; row = XMLnextElem(row))
    {
	colup=0;
	HRow = calloc(sizeof(HTMLRow), 1);
	Tab->FirstRow = addQueueB(Tab->FirstRow, Tab->FirstRow, HRow);
	HRow->Height = decodeSize(XMLgetAttr(row, "height"));
	for (cell=row->Elements; cell; cell = XMLnextElem(cell))
	{
	    int colspan;
	    int rowspan;
	    HTMLCell *Cell;
	    
	    Cell = calloc(sizeof(HTMLCell), 1);
	    Tab->FirstCell = addQueueB(Tab->FirstCell, Tab->FirstCell, Cell);
	    colspan = atoi(XMLgetAttr(cell, "colspan"));
	    rowspan = atoi(XMLgetAttr(cell, "rowspan"));
	    Cell->Value = ((DOMNode *)cell)->Value;
	    if (cell->Elements)
	    {
		DOMElement *inp = cell->Elements;
		char *type = XMLgetAttr(inp, "type");
		Cell->Value = XMLgetAttr(inp, "value");
		if (!strcmp(type, "text"))
			Cell->Type = 1;
		else
		if (!strcmp(type, "button"))
			Cell->Type = 2;
		else
		if (!strcmp(type, "textarea"))
			Cell->Type = 3;
	    }
	    Cell->TabLay[0] = colup;
	    Cell->TabLay[1] = rows;
	    Cell->TabLay[2] = 1;
	    Cell->TabLay[3] = 1;
	    Cell->TabLay[4] = decodeAlign(XMLgetAttr(cell, "align"));
	    colup++;
	}
	if (colup > maxcol)
	    maxcol = colup;
	rows++;
    }
    Tab->Rows = rows;
    Tab->Cols = maxcol;
    return Tab;
}

JTab *JFormCreate(HTMLTable *Table)
{
    int *Rows;
    int *Cols;
    uint nrows = Table->Rows;
    uint ncols = Table->Cols;
    HTMLCell *head, *cur;
    HTMLRow *hrow, *row;
    JTab *tab;
    JW *comp;
    
    uint i=0;
    Rows = malloc(sizeof(int)*nrows);
    hrow = row = Table->FirstRow;
    if (!hrow)
	return NULL;
    do
    {
	Rows[i] = row->Height;
	i++;
	row = row->Next;
    }
    while (row != hrow);

    Cols = malloc(sizeof(int)*ncols);
    for (i=0; i<ncols; i++)
    {
	Cols[i] = JTabF_Preferred;
    }
    tab = JTabInit(NULL, Cols, Rows, ncols, nrows);
    head = Table->FirstCell;
    cur = head;
    if (!head)
	return NULL;
    do
    {
	printf("Creating %s\n", cur->Value);
	switch (cur->Type)
	{
	    case 0: comp = JStxInit(NULL, cur->Value); break;
	    case 1: comp = JTxfInit(NULL); break;
	    case 2: comp = JButInit(NULL, cur->Value); break;
	}
	comp->LayData = cur->TabLay;
	JCntAdd(tab, comp);
	cur = cur->Next;
    }
    while (cur != head);
    return tab;
}

HTMLTable *JFormLoad(char *name)
{
    DOMElement *root;
    DOMElement *xml;
    DOMElement *cur;
    uint i;
    
    root = XMLloadFile(name);
    xml = XMLgetNode(root, "xml");
    cur = xml->Elements;
    for (cur=xml->Elements; cur; cur = XMLnextElem(cur))
    {
	return JFormDoTable(cur);
    }
}
