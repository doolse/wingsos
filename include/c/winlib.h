
#ifndef _WINLIB_H_
#define _WINLIB_H_

#include <sys/types.h>

#ifndef _COLDEF_
#define _COLDEF_
#define COL_Black	0
#define COL_White	1
#define COL_Red		2
#define COL_Cyan	3
#define COL_Purple	4
#define COL_Green	5
#define COL_Blue	6
#define COL_Yellow	7
#define COL_Orange	8
#define COL_Brown	9
#define COL_Pink	10
#define COL_DarkGrey	11
#define COL_MedGrey	12
#define COL_LightGreen	13
#define COL_LightBlue	14
#define COL_LightGrey	15
#endif

#define MJW_Init	0
#define MJW_Kill	2
#define MJW_Draw	4
#define MJW_Show	6
#define MJW_Handle	8
#define MJW_ReDraw	10
#define MJW_Focus	12
#define MJW_Select	14
#define MJW_KeyDown	16
#define MJW_AddChild	18
#define MJW_Button	20
#define MJW_Motion	22
#define MJW_Bound	24
#define MJW_Notice	26
#define MJW_Geom	28
#define MJW_Resize	30
#define MJW_RButton	32
#define MJW_ChNotice	34
#define MJW_SIZE	36

#define GEOM_TopLeft	0
#define GEOM_BotLeft	1
#define GEOM_TopRight	2
#define GEOM_BotRight	3

#define GEOM_TopLeft2	0
#define GEOM_BotLeft2	4
#define GEOM_TopRight2	8
#define GEOM_BotRight2	12

#include <wgsipc.h>

typedef struct {
	int X;
	int Y;
	unsigned int XSize;
	unsigned int YSize;
	int Parent;
	unsigned int Sense;
	unsigned int Opaque;
	unsigned int Flags;
	void *Data;
} Region;

typedef struct {
	Region Reg;
	int Hasprop;
	char *Name;
	int Flags;
	unsigned int MinX;
	unsigned int MinY;
	unsigned int MaxX;
	unsigned int MaxY;
	void *Icon;
} RegInfo;

typedef struct mendata {
	char *name;
	int shortcut;
	void *icon;
	int flags;
	int command;
	void *data;
	struct mendata *submenu;
} MenuData;

typedef void JWin;

typedef struct JObj {
	void *VMT;
	void *Class;
} JObj;

typedef struct JObjClass {
	void *VMT;
	unsigned int ObjSize;
	unsigned int MethSize;
	char *Name;
} JObjClass;

void *JSubclass(void *Class, unsigned int size, ...);
void *JNew(void *Class);

typedef struct JW {
	JObj Object;
	int X;
	int Y;
	unsigned int XSize;
	unsigned int YSize;
	int Flags;
	void *Parent;
	void *Next;
	void *Prev;
	void *FrontCh;
	void *BackCh;
	int Sense;
	int Opaque;
	int RegID;
	int Con;
	int RegFlags;
	void *Focused;
	void *TopLevel;
	int Anchors;
	void *Data;
	int XG;
	int YG;
	unsigned int XSizeG;
	unsigned int YSizeG;
	unsigned int Colours;
	int Font;
	int FStyle;
	int HasCh;
	int MinXS;
	int MinYS;
	int XScrld;
	int YScrld;
	int HideCnt;
} JW;

extern JWin *JWinInit(JWin *self, int x, int y, int xsize, int ysize, JWin *parent, int sense, int Flags);
extern void JWinSetPen(JWin *self, int col);
extern void JWinSetBack(JWin *self, int col);
extern void JWinKill(JWin *self);
extern void JWinShow(JWin *self);
extern void JWinHide(JWin *self);
extern void JWinGeom(JWin *self, int x, int y, int x2, int y2, int kind);
extern void JWinMove(JWin *self, int x, int y, int kind);
extern void JWinSize(JWin *self, int xsize, int ysize);
extern void JWinSetData(JWin *self, void *data);
extern void *JWinGetData(JWin *self);
extern void JWinSelCh(JWin *self, JWin *widget);
extern void JWinReq(JWin *self);
extern void JWinRePare(JWin *self, int region);

extern void JWinClass;

typedef struct JWClass
{
	JObjClass PClass;
} JWClass;

/* JBut */

typedef struct JBut {
	JW JWParent;
	char *Label;
	int Flags;
	int EntCol;
	int TextX;
	int TextY;
	void (*Clicked)();
	void (*DblClicked)();
} JBut;

extern JWin *JButInit(JWin *self, JWin *parent, int flags, char *title);

typedef struct JIbt {
	JBut JButParent;
	void *IconUp;
	void *IconDown;
	uint BitSize;
	void *ExtData;
} JIbt;

extern JWin *JIbtInit(JWin *self, JWin *parent, int flags, int xsize, int ysize, unsigned char *iconup, unsigned char *icondown);


/* JFra */

typedef struct JFra {
	JW JWParent;
	char *Label;
	int Flags;
} JFra;

typedef struct JBmp {
	JW JWParent;
	uchar *Bitmap;
	uchar *Cols;
} JBmp;

extern JWin *JBmpInit(JWin *self, JWin *parent, int x, int y, int xsize, int ysize, void *bitmap);
extern void JBmpClass;

typedef struct JChk {
	JW JWParent;
	char *Label;
	int Status;
} JChk;

extern JWin *JChkInit(JWin *self, JWin *parent, int flags, char *title);

typedef struct JWnd {
	JW JWParent;
	void (*RightClick)();
	char *Label;
	int Flags;
} JWnd;

extern JWin *JWndInit(JWin *self, JWin *parent, int flags, char *title, int wndflags);
extern JWin JWndDefault(JWin *self, int type, int command, void *data);

typedef struct JTxf {
	JW JWParent;
	void (*Entered)();
	char *String;
	int XCur;
	int Size;
	int AcSize;
	int CursX;
	int OffsX;
} JTxf;

extern JWin *JTxfInit(JWin *self, JWin *parent, int flags, char *initial);
extern void JTxfSetText(JWin *self, char *text);
extern char *JTxfGetText(JWin *self);

typedef struct JBar {
	JW JWParent;
	void (*Changed)();
	uint32 CSize;
	uint32 Max;
	uint32 Value;
	int Orient;
	int BarSize;
	int Offs;
	uint32 ButStep;
	uint32 PageStep;
	int Mode;
} JBar;

extern JWin *JBarInit(JWin *self, JWin *parent, int flags, int orient, int side);
extern int JBarSetVal(JWin *self, unsigned long val, int invoke);

#define JWinCallback(a,b,c,d) ((b *)a)->c = d
	
#define MENF_Disabled	1
#define MENF_Tickable	2
#define MENF_Ticked	4
#define MENF_Line	8

extern void *JAppInit(void *self, int channel);
extern void JAppDrain(void *self);
extern void JAppLoop(void *self);
extern void JAppSetMain(void *self, void *main);

extern int VMC(int, void *, ...);
/* Virtual method call function */


extern int JRegInfo(int region, RegInfo *props);
extern int JShow(int region);
extern int JEGeom(int region, int x, int y, unsigned int xsize, unsigned int ysize);

extern JWin *JScrInit(JWin *self, JWin *parent, int flags);

extern JWin *JManInit(JWin *self, JWin *parent, int flags, char *title, int wndflags, int region);
extern void JManClass;


#define MJTxf_Entered	MJW_SIZE+0

extern JWin *JStxInit(JWin *self, JWin *parent, int flags, char *text, int mode);

extern JWin *JTxtInit(JWin *self, JWin *parent, int flags, char *initial);
extern void JTxtAppend(JWin *self, char *str);
extern JWin *JTxtVBar(JWin *self);

extern JWin *JLstInit(JWin *self, JWin *parent, int flags);
extern void JLstInsert(JWin *self, char *label, void *insertp, void *data);

extern JWin *JCntInit(JWin *self, JWin *parent, int flags, int cflags);

extern JWin *JMnuInit(JWin *self, JWin *parent, MenuData *themenu, int x, int y, void callback());

extern JWin *JIcoInit(JWin *self, JWin *parent, int flags, int xsize, int ysize, void *bitmap);

extern JWin *JFslInit(JWin *self, JWin *parent, int flags, char *dir);

#ifndef NULL
#define NULL ((void *)0)
#endif

#define STXM_CenterX	1
#define STXM_CenterY	2

#define SBAR_Vert	0
#define SBAR_Horiz	1

#define WEV_Draw	1
#define WEV_Expose	2
#define WEV_Keyboard	4
#define WEV_Boundary	8
#define WEV_Button	16
#define WEV_Motion	32
#define WEV_MotionBut	64
#define WEV_Focus	128
#define WEV_Notice	256

#define WIN_EventRecv	(WINMSG+12)

#define EVT_Entered	0
#define EVT_Command	0
#define EVT_Changed	0

#define EVS_But1Up	0
#define EVS_But1Down	1
#define EVS_But1Double	5
#define EVS_But2Up	8
#define EVS_But2Down	2
#define EVS_But2Double	6
#define EVS_ButsDown	3
#define EVS_But2Mask	10

#define EVS_User	0
#define EVS_Added	1
#define EVS_Deleted	2
#define EVS_PropChange	3
#define EVS_Hidden	4
#define EVS_Changed	5
#define EVS_ReqChange	6
#define EVS_ReqShow	7
#define EVS_LostMouse	8
#define EVS_Shown	9

#define CMD_EXIT	1
#define CMD_CLOSE	2
#define CMD_MAX		3
#define CMD_MINI	4
#define CMD_RESTORE	5

#define JF_Added	1
#define JF_Hide		2
#define JF_Resized	4
#define JF_Selected	8
#define JF_Focused	16
#define JF_Selectable	32
#define JF_Front	64
#define JF_Visible	128
#define JF_ModalMenu	256
#define JF_InParent	512

#define JWndF_Resizable	1

extern void GfxSetPen(int x, int y);
extern void GfxString(unsigned char *str, ...);
extern void GfxClear();
extern unsigned char *GfxGetPtr();
extern void GfxChar(int ch);
extern void GfxSetMode(int mode);
extern void GfxSetFont(int font);
extern void GfxStyle(int style);
extern void GfxSetCol(int col);
extern void GfxText(char *text);
extern void GfxFlush();

#define BITT_Mono	0
#define BITT_Inter	1
#define BITT_Seper	2

#define GMOD_SameBack	1
#define GMOD_SamePen	2
#define GMOD_Ora	4
#define GMOD_Inverted	8
#define GMOD_Masked	16

#define CHAR_Col	0xf0
#define CHAR_Rep	0xf1
#define CHAR_YRep	0xf2
#define CHAR_YEnd	0xf3
#define CHAR_Mode	0xf4
#define CHAR_Skip	0xf5
#define CHAR_End	0xff

#define GFX_End		0
#define GFX_Charset	1
#define GFX_Bitmap	2
#define GFX_Text	3
#define GFX_Pen		4
#define GFX_Font	5
#define GFX_Mode	6
#define GFX_Col		7
#define GFX_Clear	8
#define GFX_Style	9
#define GFX_Box		10
#define GFX_ESC		0xef

typedef struct JItem {
struct JItem *Next;
struct JItem *Prev;
struct JItem *Parent;
struct JItem *Children;
unsigned int Height;
int Flags;
void *Data;
} JItem;

enum {
JItemF_Selected=1,
JItemF_Expanded=2,
JItemF_Expandable=4,
};

enum {
JColF_CHARS=1,
JColF_STRING,
JColF_MASK=0x0f,
JColF_Indent=0x10,
};

extern void *JColInit(void *self, void *parent, int flags, char *title, int width, void *offs, int type, void *model);
extern void *JTreInit(void *self, void *parent, int flags, JItem *model, void meth());
extern void JTreAddColumns(void *self, void **cols, ...);

#define OFFSET(a,b) (&((a *)0)->b)

#endif
