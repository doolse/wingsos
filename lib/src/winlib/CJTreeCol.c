#include <winlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

static unsigned char expstring[] = {GFX_Charset, '%', 'D', 1, 1, '%', 'b', CHAR_End, '%', 'E'};
static unsigned char ChSet[] = {
0,0,0,0,0,0,0,0,
0,0,0x7c,0x6c,0x47,0x6c,0x7c,0,
0,0,0x7c,0x7c,0x47,0x7c,0x7c,0
};
static unsigned char icostring[] = {GFX_Bitmap, '%', 'D', 2,1, BITT_Seper, '%', 'D', '%', 'E'};

void renderCell(JTree *Self, uint Type, int Flags, unsigned char *DataP, int col)
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

static int paintkids(JTreeCol *Self, VNode *Node, int y, int indent)
{
    JTree *Tree = Self->Tree;
    uint Type = Self->Type;
    Vec *vec = Node->Children;
    char *str;
    int x,xsize,ysize,col,ocol;
    uint size;
    VNode **Ptrs;
    
    if (!vec)
	return y;
    xsize = ((JW *)Self)->XSize;
    ysize = ((JW *)Self)->YSize;
    ocol = ((JW *)Self)->Colours;
    
    size = vec->size;
    Ptrs = (VNode **)vec->Ptrs;
    while (size)
    {
	int Height = 8;
	VNode *cur = Ptrs[0];
	int Flags = cur->Flags;
        uchar *DataP = (uchar *)cur->Value;
	
	if (Height+y >= 0)
	{
	    DataP += Self->Offset;
	    if (Type&JColF_LongSort)
	    {
    	    	DataP+=4;
	    }
	    x = 0;
	    col = ocol;
	    
	    GfxSetOffs(0,y);
	    GfxSetPen(0,0);
	    if (Flags&JItemF_Selected)
		    col = (col&0xf0) + 14;
	    GfxSetCol(col);
	    GfxBox(xsize,8, 0);
	    if (Type&JColF_Indent)
	    {
		unsigned int expander = 0;
		
		x = indent*8;
		GfxSetPen(x,0);
		GfxSetCol(col&0x0f | 0xb0);
		if (Flags&JItemF_Expandable)
		{
			expander++;
			if (Flags&JItemF_Expanded)
				expander++;
		}
		GfxString(expstring, ChSet, expander);
		x+=8;
		GfxSetOffs(x,y);
	    }
	    renderCell(Tree, Type, Flags, DataP, col);
	}
	y+=Height;
	if (Flags&JItemF_Expanded)
	{
	    y = paintkids(Self, cur, y, indent+1);
	}
	Ptrs++;
	size--;
    }
    return y;
}

static VNode *findrow(JTreeCol *Self, VNode *Node, int *yptr, int *indent)
{
    Vec *vec = Node->Children;
    uint size;
    int y = *yptr;
    VNode **Ptrs;

    size = vec->size;
    Ptrs = (VNode **)vec->Ptrs;
    while (size)
    {
	int Height = 8;
	VNode *cur = Ptrs[0];
	int Flags = cur->Flags;
	if (y>=0 && y<Height)
	    return cur;
	y-=Height;
	if (Flags&JItemF_Expanded)
	{
	    *yptr = y;
	    (*indent)++;
	    cur = findrow(Self, cur, &y, indent);
	    if (cur)
		return cur;
	    (*indent)--;
	}
	Ptrs++;
	size--;
    }
    *yptr = y;
    return NULL;
}

VNode *JTreeColRow(JTreeCol *Self, int y, int *indent)
{
    JTree *Tree = Self->Tree;

    y += Tree->YScroll;
    *indent = 0;
    return findrow( Self, Tree->Root, &y, indent);
}

void JTreeColDraw(JTreeCol *Self)
{
    JTree *Tree = Self->Tree;
    int y,ysize;

    ysize = ((JW *)Self)->YSize;
    y = -Tree->YScroll;
    
    y = paintkids( Self, Tree->Root, y, 0);
    if (y<ysize)
    {
	GfxSetOffs(0,y);
	GfxSetPen(0,0);
	GfxSetCol(((JW *)Self)->Colours);
	GfxBox(((JW *)Self)->XSize, ysize-y, 0);
    }
}



static int compare(JTreeCol *Self, VNode *view1, VNode *view2)
{
    unsigned int offs = Self->Offset;
    char *val1 = *(char **)(((char *)view1->Value)+offs);
    char *val2 = *(char **)(((char *)view2->Value)+offs);
//    printf("comparing '%s' to '%s'\n", val1, val2);
    if (Self->Type&JColF_LongSort)
	    return val1-val2;
    return strcmp(val1, val2);
}

static void oursort(JTreeCol *Self, VNode **views, unsigned int len, int desc)
{
    VNode **upto;
    unsigned int i,j;
    for (i=1; i<len; i++)
    {
	upto = &views[i-1];
	for (j=i; j>0; j--)
	{
	    VNode *view;
	    int val = compare(Self, upto[0], upto[1]);
	    if (desc)
	    {
		if (val>0)
		    break;
	    }
	    else if (val<0)
		break;
	    view = upto[1];
	    upto[1] = upto[0];
	    upto[0] = view;
	    upto--;
	}
    }	
}

void JTreeSort(JTree *Self, VNode *view)
{   
    uint count=0;
    uint i;
    Vec *vec;
    VNode **kids;

    if (view == NULL)
	    view = Self->Root;
    vec = view->Children;
    if (!vec)
	return;
    count = vec->size;
    kids = (VNode **)vec->Ptrs;
//    printf("Sorting %lx with %d, %lx, %lx\n", kids, count, Self->SortCol, view);
    oursort(Self->SortCol, kids, count, Self->SortDesc);
    while (count)
    {
	if (kids[0]->Children)
	    JTreeSort(Self, kids[0]);
	count--;
	kids++;
    }
}

void JTreeTogSort(JTree *Self, JTreeCol *sort)
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
    JTreeSort(Self, Self->Root);
    JTreeReDrawCols(Self);
}

