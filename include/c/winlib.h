
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

typedef struct SizeHints {
	unsigned int MinX;
	unsigned int MinY;
	unsigned int PrefX;
	unsigned int PrefY;
	unsigned int MaxX;
	unsigned int MaxY;
} SizeHints;

typedef void JWin;

typedef struct JObj {
	void *VMT;
	void *Class;
} JObj;

typedef struct JObjClass {
	void *VMT;
	void *VMCode;
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
	
	int Sense;
	int Opaque;
	int RegID;
	int Con;
	int RegFlags;
	int HideCnt;
	
	void *TopLevel;
	void *Data;
	
	unsigned int Colours;
	int Font;
	int FStyle;
	
	unsigned int PrefXS;
	unsigned int PrefYS;
	unsigned int MinXS;
	unsigned int MinYS;
	unsigned int MaxXS;
	unsigned int MaxYS;	
} JW;
	
typedef struct JCnt {
	JW JWParent;
	void *FrontCh;
	void *BackCh;
	void *Selected;
	unsigned int NumChildren;
	int HasCh;
	int LayFlags;
	int Orient;
} JCnt;

typedef struct MJW {
JW *(*Init)();
void (*Kill)(JW *Self);
void (*Draw)(JW *Self);
void (*Show)(JW *Self);
void (*Hide)(JW *Self);
void (*Handle)(JW *Self, void *Event);
void (*Notify)(JW *Self, int why);
void (*GetHints)(JW *Self, SizeHints *hints);

void *KeyD;
void *Button;
void *RButton;
void *Motion;
void *Bound;
void *Notice;
} MJW;

typedef struct MJCnt {
MJW mjw;
void (*Add)();
void *Remove;
void *Layout;
} MJCnt;

typedef struct MJView {
MJCnt mjcnt;
void *Scrolled;
} MJView;

extern JWin *JWinInit(JWin *self, int x, int y, int xsize, int ysize, JWin *parent, int sense, int Flags);
extern void JWinSetPen(JWin *self, int col);
extern void JWinSetBack(JWin *self, int col);

extern void JWSetMin(JWin *self, unsigned int xsize, unsigned int ysize);
extern void JWSetPref(JWin *self, unsigned int xsize, unsigned int ysize);
extern void JWSetMax(JWin *self, unsigned int xsize, unsigned int ysize);
extern void JWSetAll(JWin *self, unsigned int xsize, unsigned int ysize);
extern void JWSetBounds(JWin *Self, int x, int y, unsigned int xsize, unsigned int ysize);

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

extern JWin *JButInit(JWin *self, char *title);

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

extern JWin *JBmpInit(JWin *self, int xsize, int ysize, void *bitmap);
extern void JBmpClass;

typedef struct JChk {
	JW JWParent;
	char *Label;
	int Status;
} JChk;

extern JWin *JChkInit(JWin *self, char *title);

typedef struct JWnd {
	JCnt JCntParent;
	void (*RightClick)();
	char *Label;
	int Flags;
} JWnd;

extern JWin *JWndInit(JWin *self, char *title, int wndflags);
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

extern JWin *JTxfInit(JWin *self);
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
} JBar;

extern JWin *JBarInit(JWin *self, int flags);
extern int JBarSetVal(JWin *self, unsigned long val, int invoke);

typedef struct JScr {
	JCnt JCntParent;
	JBar *VBar;
	JBar *HBar;
	JWin *Corner;
	struct JView *View;
	int Flags;
} JScr;

typedef struct JView {
	JCnt JCntParent;
	unsigned long XScrld;
	unsigned long YScrld;
	unsigned long MaxX;
	unsigned long MaxY;
	unsigned int VisX;
	unsigned int VisY;
	unsigned int HeaderX;
	unsigned int HeaderY;
	JScr *Scroller;
} JView;

enum {
JScrF_HNever	= 1,
JScrF_HAlways	= 2,
JScrF_HGoEnd	= 16,
JScrF_VNever	= 4,
JScrF_VAlways	= 8,
JScrF_VGoEnd	= 32
};

extern JWin *JScrInit(JWin *self, JWin *parent, int flags);
extern void JScrMax(JWin *self, long maxx, long maxy);

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
extern void JWSetData(JWin *self, void *data);
extern void *JWGetData(JWin *self);

extern JWin *JManInit(JWin *self, char *title, int wndflags, int region);
extern void JManClass;


#define MJTxf_Entered	MJW_SIZE+0

extern JWin *JFilInit(JWin *self);

extern JWin *JStxInit(JWin *self, char *text);

extern JWin *JTxtInit(JWin *self);
extern void JTxtAppend(JWin *self, char *str);
extern void JTxtScrolled(JWin *self, long x, long y);

extern JWin *JLstInit(JWin *self, JWin *parent, int flags);
extern void JLstInsert(JWin *self, char *label, void *insertp, void *data);

extern JWin *JCntInit(JWin *self);
extern void JCntGetHints(JWin *self, SizeHints *sizes);
extern void JCntClass;
extern JWin *JCardInit(JWin *self);

extern JWin *JMnuInit(JWin *self, MenuData *themenu, int x, int y, void callback());

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
extern void GfxBox(int xsize, int ysize, int fill);
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

typedef struct JListRow {
struct JListRow *Next;
struct JListRow *Prev;
struct JListRow *NextView;
int Flags;
} JListRow;

typedef struct JListRowV {
JListRow jlr;
struct JListRowV *NextSel;
struct JTre *Tree;
void *data;
unsigned int Height;
} JListRowV;

typedef struct JTreeRow {
JListRow jlr;
struct JTreeRow *Parent;
struct JTreeRow *Children;
} JTreeRow;

typedef struct JTreeRowV
{
JListRowV jlrv;
struct JTreeRowV *Parent;
struct JTreeRowV *Children;
} JTreeRowV;

enum {
JItemF_Selected=1,
JItemF_Expanded=2,
JItemF_Expandable=4,
};

enum {
JColF_STRING=1,
JColF_LONG,
JColF_MASK=0x0f,
JColF_Indent=0x10,
JColF_Icon=0x20,
JColF_2Icons=0x40,
JColF_LongSort=0x80
};

typedef struct SortData
{
	void *Row;
	void *Data;
} SortData;

typedef struct TreeIter
{
unsigned char *DataP;
int Indent;
int Flags;
unsigned int Height;
JTreeRowV *ItemP;
JTreeRowV *PareP;
} TreeIter;

extern void *JColInit(void *self, void *tree, char *title, int width, void *offs, int type, void *model);

extern void JViewSync(JWin *self);

typedef struct JCol {
	JCnt jcnt;
	JW *Title;
	struct JColV *Colv;
	unsigned long Offset;
	unsigned int Type;
} JCol;

typedef struct JTre {
	JView JViewParent;
	JTreeRowV *Model;
	void (*Expander)();
	void (*Clicked)();
	int YScroll;
	JCol *SortCol;
	int SortDesc;
} JTre;

typedef struct MJTre {
MJView mjview;
void (*GetIter)(JTre *Self, struct TreeIter *iter);
int (*NextItem)(JTre *Self, struct TreeIter *iter);
} MJTre;

typedef struct JColV {
	JW jw;
	JTre *Tree;
	unsigned long Offset;
	unsigned int Type;
} JColV;

extern JTre *JTreInit(void *self, void *model, void meth());
extern void JTreAddColumns(void *self, void **cols, ...);
extern void JTreAppendRow(void *Parent, void *Cur);
extern void JTreRemoveRow(void *Cur);

#define OFFSET(a,b) (&((a *)0)->b)
#define METHOD(a,b) (unsigned int)(&((a *)0)->b)

#endif
