#include <winlib.h>

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
	int x,y;
	MJTre *VMT;
	JObjClass *Class;
	
	GfxClear();
	Class = (JObjClass *)(((JW *)Tree)->Object.Class);
	VMT = (MJTre *)(Class->VMT);
	VMT->GetIter(Tree, &iter);
	y = -Tree->YScroll;
	while (!VMT->NextItem(Tree, &iter))
	{
/*		printf("Doing %lx\n", iter.ItemP); */
		if ((int)iter.Height+y >= 0)
		{
			iter.DataP += Self->Offset;
			x = 0;
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
	}
}
