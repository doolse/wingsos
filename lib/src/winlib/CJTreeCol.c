#include <winlib.h>
#include <string.h>
#include <stdlib.h>

static unsigned char expstring[] = {GFX_Charset, '%', 'D', 1, 1, '%', 'b', CHAR_End, '%', 'E'};
static unsigned char ChSet[] = {
0,0,0,0,0,0,0,0,
0,0,0x7c,0x6c,0x47,0x6c,0x7c,0,
0,0,0x7c,0x7c,0x47,0x7c,0x7c,0
};
static unsigned char icostring[] = {GFX_Bitmap, '%', 'D', 2,1, BITT_Seper, '%', 'D', '%', 'E'};

void renderCell(JList *Self, uint Type, uint Flags, unsigned char *DataP, int col)
{
    uint x=0;
    if (Type&JColF_Icon)
    {
	unsigned char **IconP = (unsigned char **)(DataP+4);
	unsigned char *Icon;
	GfxSetPen(0,0);
	if ((Type&JColF_2Icons) && (Flags&JItemF_Expanded))
	{
		IconP++;
	}
	Icon = *IconP;
	GfxString(icostring, Icon, Icon+16);
	x+=16;
    }
    GfxSetCol(col);
    GfxSetPen(x+1, 7);
    GfxText(*((char **)DataP));
}

void JTreeColDraw(JCol *Self)
{
    TreeIter iter;
    uint Type = Self->Type;
    JTree *Tree = (JTree *)Self->List;
    char *str;
    int x,y,xsize,ysize,col,ocol;

    JTreeGetIter(Tree, &iter);
    y = -((JList *)Tree)->YScroll;
    xsize = ((JW *)Self)->XSize;
    ysize = ((JW *)Self)->YSize;
    ocol = ((JW *)Self)->Colours;
    while (!JTreeNextItem(Tree, &iter))
    {	    
	if ((int)iter.Height+y >= 0)
	{
	    iter.DataP += Self->Offset;
	    if (Type&JColF_LongSort)
	    {
		    iter.DataP+=4;
	    }
	    x = 0;
	    col = ocol;
	    
	    GfxSetOffs(0,y);
	    GfxSetPen(0,0);
	    if (iter.Flags&JItemF_Selected)
		    col = (col&0xf0) + 14;
	    GfxSetCol(col);
	    GfxBox(xsize,8, 0);
	    if (Type&JColF_Indent)
	    {
		unsigned int expander = 0;
		x = iter.Indent*8;
		GfxSetPen(x,0);
		GfxSetCol(col&0x0f | 0xb0);
		if (iter.Flags&JItemF_Expandable)
		{
			expander++;
			if (iter.Flags&JItemF_Expanded)
				expander++;
		}
		GfxString(expstring, ChSet, expander);
		x+=8;
		GfxSetOffs(x,y);

	    }
	    renderCell((JList *)Tree, Type, iter.Flags, iter.DataP, col);
	}
	y += iter.Height;
	if (y>ysize)
		break;
    }
    if (y<ysize)
    {
	GfxSetOffs(0,y);
	GfxSetPen(0,0);
	GfxSetCol(ocol);
	GfxBox(xsize, ysize-y, 0);
    }
}


void JListColDraw(JCol *Self)
{
    TreeIter iter;
    uint Type = Self->Type;
    JList *List = Self->List;
    char *str;
    int x,y,xsize,ysize,col,ocol;

    JListGetIter(List, &iter);
    y = -List->YScroll;
    xsize = ((JW *)Self)->XSize;
    ysize = ((JW *)Self)->YSize;
    ocol = ((JW *)Self)->Colours;
    while (!JListNextItem(List, &iter))
    {	    
	if ((int)iter.Height+y >= 0)
	{
	    iter.DataP += Self->Offset;
	    if (Type&JColF_LongSort)
	    {
		    iter.DataP+=4;
	    }
	    x = 0;
	    col = ocol;
	    
	    GfxSetOffs(0,y);
	    GfxSetPen(0,0);
	    if (iter.Flags&JItemF_Selected)
		    col = (col&0xf0) + 14;
	    GfxSetCol(col);
	    GfxBox(xsize,8, 0);
	    renderCell(List, Type, iter.Flags, iter.DataP, col);
	}
	y += iter.Height;
	if (y>ysize)
		break;
    }
    if (y<ysize)
    {
	GfxSetOffs(0,y);
	GfxSetPen(0,0);
	GfxSetCol(ocol);
	GfxBox(xsize, ysize-y, 0);
    }
}

static int compare(JCol *Self, JListRowV *view1, JListRowV *view2)
{
	unsigned int offs = Self->Offset;
	char *val1 = *(char **)(((char *)view1->data)+offs);
	char *val2 = *(char **)(((char *)view2->data)+offs);
//	printf("comparing '%s' to '%s'\n", val1, val2);
	if (Self->Type&JColF_LongSort)
		return val1-val2;
	return strcmp(val1, val2);
}

static void oursort(JCol *Self, JListRowV **views, unsigned int len, int desc)
{
	unsigned int i,j;
	for (i=0; i<len; i++)
	{
		for (j=i; j>0; j--)
		{
			JListRowV *view;
			int val = compare(Self, views[j-1], views[j]);
			if (desc)
			{
				if (val>0)
					break;
			}
			else if (val<0)
				break;
			view = views[j];
			views[j] = views[j-1];
			views[j-1] = view;
		}
	}	
}

void JTreeSort(JList *Self, JTreeRowV *view)
{
	JListRow *head;
	JListRow *next;
	JListRow **views;
	unsigned int count=0;
	unsigned int i;
	
	if (view == NULL)
		view = Self->Model;
	head = (JListRow*)view->Children;
	next = head;
	if (!head)
		return;
	views = malloc(sizeof(JListRow *)*100);
	do
	{
		views[count] = next;
		next = next->Next;
		count++;
	}
	while (next != head);
	oursort(Self->SortCol, (JListRowV**)views, count, Self->SortDesc);
	view->Children = (JTreeRowV *)views[0];
	next = views[0];
	for (i=1;i<count;i++)
	{
		next->Next = views[i];
		next = views[i];
		next->Prev = views[i-1];
	}
	next->Next = views[0];
	views[0]->Prev = next;
	for (i=0; i<count; i++)
	{
		if (((JTreeRowV *)views[i])->Children)
			JTreeSort(Self, (JTreeRowV*)views[i]);
	}
	free(views);
}

void JTreeTogSort(JList *Self, JCol *sort)
{
	if (sort == Self->SortCol)
	{
		Self->SortDesc ^= 1;
	}
	else
	{
		Self->SortCol = sort;
		Self->SortDesc = 0;
	}
	JTreeSort(Self, Self->Model);
	JTreeReDrawCols(Self);
}
