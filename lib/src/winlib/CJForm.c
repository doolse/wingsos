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
	return -(int32)val;
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

static int decodeFillWidth(char *width)
{
    uint32 val;
    char *end;
    if (!*width)
	return -1;
    val = strtoul(width, &end, 0);
    if (end == width)
	return -1;
    return val;
}

static HTMLTable *JFormDoTable(DOMElement *table, HTMLForms *forms)
{
    DOMElement *row,*cell;
    uint nrows;
    HTMLTable *Tab;
    HTMLRow *HRow;
    uint rows=0;
    uint maxcol=0;
    uint colup;
    Tab = calloc(sizeof(HTMLTable), 1);
    forms->FirstTable = addQueueB(forms->FirstTable, forms->FirstTable, Tab);
    Tab->Name = XMLgetAttr(table, "name");
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
	    if (!colspan)
		colspan++;
	    if (!rowspan)
		rowspan++;
	    Cell->Width = decodeSize(XMLgetAttr(cell, "width"));
	    Cell->Value = ((DOMNode *)cell)->Value;
	    Cell->Name = "";
	    Cell->Hints.MinX = -1;
	    Cell->Hints.MinY = -1;
	    Cell->Hints.PrefX = -1;
	    Cell->Hints.PrefY = -1;
	    Cell->Hints.MaxY = -1;
	    Cell->Hints.MaxY = -1;
	    if (cell->Elements)
	    {
		DOMElement *inp = cell->Elements;
		char *type;
		int ctype=0;
		
		if (!strcmp(((DOMNode *)inp)->Name, "table"))
		{
		    ctype = 1;
		    Cell->Inner = JFormDoTable(inp, forms);
		}
		else 
		{
    	    	    Cell->Name = XMLgetAttr(inp, "name");
		    Cell->Hints.MinX = decodeFillWidth(XMLgetAttr(inp, "minx"));
		    Cell->Hints.MinY = decodeFillWidth(XMLgetAttr(inp, "miny"));
		    Cell->Hints.PrefX = decodeFillWidth(XMLgetAttr(inp, "prefx"));
		    Cell->Hints.PrefY = decodeFillWidth(XMLgetAttr(inp, "prefy"));
		    Cell->Hints.MaxY = decodeFillWidth(XMLgetAttr(inp, "maxx"));
		    Cell->Hints.MaxY = decodeFillWidth(XMLgetAttr(inp, "maxy"));
		    if (!strcmp(((DOMNode *)inp)->Name, "fill"))
		    {
			ctype = 6;
		    }
		    else
		    {
			type = XMLgetAttr(inp, "type");
			Cell->Value = XMLgetAttr(inp, "value");
			if (!strcmp(type, "text"))
				ctype = 2;
			else
			if (!strcmp(type, "button"))
				ctype = 3;
			else
			if (!strcmp(type, "textarea"))
				ctype = 4;
			else
			if (!strcmp(type, "custom"))
				ctype = 5;

		    }
		}
		Cell->Type = ctype;
	    }
	    Cell->TabLay[0] = colup;
	    Cell->TabLay[1] = rows;
	    Cell->TabLay[2] = colspan;
	    Cell->TabLay[3] = rowspan;
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

JTab *JFormCreate(HTMLTable *Table, void(*create)(HTMLCell *Cell, void *state), void *state)
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
    head = Table->FirstCell;
    cur = head;
    if (!head)
	return NULL;
    do
    {
	if (cur->Width != JTabF_Preferred)
	    Cols[cur->TabLay[0]] = cur->Width;
	cur = cur->Next;
    }
    while (cur != head);
    
    cur = head;
    tab = JTabInit(NULL, Cols, Rows, ncols, nrows);
    do
    {
	comp = NULL;
	switch (cur->Type)
	{
	    case 0: comp = JStxInit(NULL, cur->Value); break;
	    case 1: comp = (JW *)JFormCreate(cur->Inner, create, state);break;
	    case 2: comp = JTxfInit(NULL); break;
	    case 3: comp = JButInit(NULL, cur->Value); break;
	    case 4: comp = JTxtInit(NULL); break;
	    case 6: comp = JFilInit(NULL, 0); break;
	}
	if (comp)
	{
	    if (cur->Hints.MinX != -1)
		comp->MinXS = cur->Hints.MinX;
	    if (cur->Hints.MinY != -1)
		comp->MinYS = cur->Hints.MinY;
	    if (cur->Hints.PrefX != -1)
		comp->PrefXS = cur->Hints.PrefX;
	    if (cur->Hints.PrefY != -1)
		comp->PrefYS = cur->Hints.PrefY;
	    if (cur->Hints.MaxX != -1)
		comp->MaxXS = cur->Hints.MaxX;
	    if (cur->Hints.MaxY != -1)
		comp->MaxYS = cur->Hints.MaxY;
	}
	cur->Win = comp;
	if (create)
	{
	    create(cur, state);
	    comp = cur->Win;
	}
	if (comp)
	{
	    comp->LayData = cur->TabLay;
	    JCntAdd(tab, comp);
	}
	cur = cur->Next;
    }
    while (cur != head);
    return tab;
}

HTMLForms *JFormLoad(char *name)
{
    DOMElement *root;
    DOMElement *xml;
    DOMElement *cur;
    HTMLForms *forms;
    uint i;
    
    root = XMLloadFile(name);
    xml = XMLgetNode(root, "xml");
    cur = xml->Elements;
    forms = calloc(sizeof(HTMLForms), 1);
    for (cur=xml->Elements; cur; cur = XMLnextElem(cur))
    {
	JFormDoTable(cur, forms);
    }
    return forms;
}

HTMLTable *JFormGetTable(HTMLForms *forms, char *name)
{
    HTMLTable *head = forms->FirstTable;
    HTMLTable *cur = head;
    
    if (!head)
	return NULL;
    do
    {
	if (!strcmp(name, cur->Name))
	    return cur;
	cur = cur->Next;
    } while (cur != head);
    return NULL;
}

JWin *JFormGetControl(HTMLTable *table, char *name)
{
    HTMLCell *head = table->FirstCell;
    HTMLCell *cur = head;
    
    if (!head)
	return NULL;
    do
    {
	if (!strcmp(name, cur->Name))
	    return cur->Win;
	cur = cur->Next;
    } while (cur != head);
    return NULL;
}

void JMapBind(HTMLTable *table, XMLGuiMap *mappings, uint nrmappings)
{
    HTMLCell *head = table->FirstCell;
    HTMLCell *cur = head;
    XMLGuiMap *map;
    uint i;
    
    if (!head)
	return;
    do
    {
	char *guiname = cur->Name;
	if (strlen(guiname))
	{
	    map = mappings;
	    for (i=0; i<nrmappings; i++)
	    {
		if (!strcmp(map->GuiName, guiname))
		{
		    JWin *win = cur->Win;
		    map->Win = win;
		    break;
		}
		map++;
    	    }
	}
	cur = cur->Next;
    } while (cur != head);    
}

void JMapToGUI(void *record, XMLGuiMap *mappings, uint nrmappings)
{
    XMLGuiMap *map;
    uint i;
    uchar *data;
    
    map = mappings;
    for (i=0; i<nrmappings; i++)
    {
	JWin *win = map->Win;
	data = ((uchar *)record)+map->Field;
	
	switch (map->Type)
	{
	    case T_INT32:
	    case T_INT16:
		printf("Unsupported yet");
		break;
	    case T_STRING:
		JTxfSetText(win, *(char **)data);
		break;
	}
	
	map++;
    }
}

void JMapFromXML(void *record, DOMElement *root, XMLGuiMap *mappings, uint nrmappings)
{
    XMLGuiMap *map;
    uint i;
    uchar *data;
    
    map = mappings;
    for (i=0; i<nrmappings; i++)
    {
	char *node = map->XMLNode;
	char *value;
	uint32 ival=0;
	
	if (node[0] == '@')
	{
	    node++;
	    value = XMLgetAttr(root, node);
	}
	else value = XMLget(root, node);
	if (!value)
	    value = "";
	data = ((uchar *)record)+map->Field;
	switch (map->Type)
	{
	    case T_INT32:
	    case T_INT16:
		ival = strtoul(value, NULL, 0);
		if (map->Type == T_INT16)
		    *(uint16 *)data = ival;
	    	else
		    *(uint32 *)data = ival;
		break;
	    case T_STRING:
		*(char **)data = strdup(value);
		break;
	}
	map++;
    }
}
