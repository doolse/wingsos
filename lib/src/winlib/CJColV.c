#include <winlib.h>
#include <string.h>
#include <stdlib.h>

static unsigned char expstring[] = {GFX_Charset, '%', 'D', 1, 1, '%', 'b', CHAR_End, '%', 'E'};
static unsigned char ChSet[] = {
0,0,0,0,0,0,0,0,
0,0,0x7c,0x6c,0x47,0x6c,0x7c,0,
0,0,0x7c,0x7c,0x47,0x6c,0x7c,0
};
static unsigned char icostring[] = {GFX_Bitmap, '%', 'D', 2,1, BITT_Seper, '%', 'D', '%', 'E'};

void JColVDraw(JColV *Self)
{
	TreeIter iter;
	unsigned int Type = Self->Type;
	JTre *Tree = Self->Tree;
	char *str;
	int x,y,xsize,ysize;
	MJTre *VMT;
	JObjClass *Class;
	
	Class = (JObjClass *)(((JW *)Tree)->Object.Class);
	VMT = (MJTre *)(Class->VMT);
	VMT->GetIter(Tree, &iter);
	y = -Tree->YScroll;
	xsize = ((JW *)Self)->XSize;
	ysize = ((JW *)Self)->YSize;
	while (!VMT->NextItem(Tree, &iter))
	{
		if ((int)iter.Height+y >= 0)
		{
			iter.DataP += Self->Offset;
			x = 0;
			GfxSetPen(0,y);
			GfxBox(xsize,8, 0);
			if (Type&JColF_Indent)
			{
				unsigned int expander = 0;
				x = iter.Indent*8;
				GfxSetPen(x,y);
				GfxSetCol(((JW *)Self)->Colours&0x0f | 0xb0);
				if (iter.Flags&JItemF_Expandable)
				{
					expander++;
					if (iter.Flags&JItemF_Expanded)
						expander++;
				}
				GfxString(expstring, ChSet, expander);
				x+=8;

			}
			if (Type&JColF_Icon)
			{
				unsigned char **IconP = (unsigned char **)iter.DataP;
				unsigned char *Icon;
				GfxSetPen(x,y);
				if ((Type&JColF_2Icons) && (iter.Flags&JItemF_Expanded))
				{
					IconP++;
				}
				if (Type&JColF_2Icons) iter.DataP+=8;
				else iter.DataP+=4;
				Icon = *IconP;
				GfxString(icostring, Icon, Icon+16);
				x+=16;
			}
			GfxSetPen(x+1, y+7);
			GfxSetCol(((JW *)Self)->Colours);
			if ((Type&JColF_MASK) == JColF_STRING)
			{
				str = *((char **)iter.DataP);
			} else str = iter.DataP;
			GfxText(str);
		}
		y += iter.Height;
		if (y>ysize)
			break;
	}
	if (y<ysize)
	{
		GfxSetPen(0,y);
		GfxBox(xsize, ysize-y, 0);
	}
}

int compare(JColV *Self, JRowView *view1, JRowView *view2)
{
	unsigned int offs = Self->Offset+8;
	char *val1 = *(char **)(((char *)((JRowView*)view1)->data)+offs);
	char *val2 = *(char **)(((char *)((JRowView*)view2)->data)+offs);
//	printf("comparting %s to %s\n", val1, val2);
	return strcmp(val1, val2);
}

void oursort(JColV *Self, JRowView **views, unsigned int len)
{
	unsigned int i,j;
	for (i=0; i<len; i++)
	{
		for (j=i; j>0; j--)
		{
			JRowView *view;
			if (compare(Self, views[j-1], views[j])<0)
				break;
			view = views[j];
			views[j] = views[j-1];
			views[j-1] = view;
		}
	}	
}

void sortrows(JColV *Self, JRowView *view)
{
	JTreeRow *head = view->treerow.Children;
	JTreeRow *next = head;
	JTreeRow **views;
	unsigned int i=0;
	unsigned int j;
	if (!head)
		return;
	views = malloc(sizeof(JRowView *)*100);
	do
	{
		views[i] = next;
		next = next->Next;
		i++;
	}
	while (next != head);
	oursort(Self, (JRowView**)views, i);
	view->treerow.Children = views[0];
	next = views[0];
	for (j=1;j<i;j++)
	{
		next->Next = views[j];
		next = views[j];
		next->Prev = views[j];
	}
	next->Next = views[0];
	views[0]->Prev = next;
	free(views);
}


