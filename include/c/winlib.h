
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
#include <wgs/util.h>

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
	uint MinX;
	uint MinY;
	uint MaxX;
	uint MaxY;
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
	uint MinX;
	uint MinY;
	uint PrefX;
	uint PrefY;
	uint MaxX;
	uint MaxY;
} SizeHints;

typedef void JWin;

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
	
	void *LayData;
	void *Data;
	
	unsigned int Colours;
	int Font;
	int FStyle;
	
	uint PrefXS;
	uint PrefYS;
	uint MinXS;
	uint MinYS;
	uint MaxXS;
	uint MaxYS;	
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


typedef struct JTab {
    JCnt jcnt;
    int *Rows;
    int *Cols;
    unsigned int RowsNum;
    unsigned int ColsNum;
    int *RowColData;
} JTab;

typedef struct MJView {
MJCnt mjcnt;
void *Scrolled;
} MJView;

extern JWin *JWInit(JWin *self, int xsize, int ysize, int sense, int Flags);
extern void JWSetPen(JWin *self, int col);
extern void JWSetBack(JWin *self, int col);

extern void JWSetMin(JWin *self, unsigned int xsize, unsigned int ysize);
extern void JWSetPref(JWin *self, unsigned int xsize, unsigned int ysize);
extern void JWSetMax(JWin *self, unsigned int xsize, unsigned int ysize);
extern void JWSetAll(JWin *self, unsigned int xsize, unsigned int ysize);
extern void JWSetBounds(JWin *Self, int x, int y, unsigned int xsize, unsigned int ysize);
extern void JWToFront(JWin *Self);
extern void JWAbs(JWin *Self, int *xy);

extern void JWinKill(JWin *self);
extern void JWinShow(JWin *self);
extern void JWinHide(JWin *self);
extern void JWinGeom(JWin *self, int x, int y, int x2, int y2, int kind);
extern void JWinMove(JWin *self, int x, int y, int kind);
extern void JWinSize(JWin *self, int xsize, int ysize);
extern void JWSetData(JWin *self, void *data);
extern void *JWGetData(JWin *self);
extern void JWinSelCh(JWin *self, JWin *widget);
extern void JWinReq(JWin *self);
extern void JWinRePare(JWin *self, int region);


typedef struct JWClazz
{
	JObjClass PClass;
} JWClazz;

extern JWClazz JWClass;

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

extern JWin *JButInit(JWin *self, const char *title);

typedef struct JIbt {
	JBut JButParent;
	void *IconUp;
	void *IconDown;
	uint BitSize;
	void *ExtData;
} JIbt;

extern JWin *JIbtInit(JWin *self, int xsize, int ysize, unsigned char *iconup, unsigned char *icondown);


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
extern JWClazz JBmpClass;

typedef struct JChk {
	JW JWParent;
	char *Label;
	int Status;
} JChk;

extern JWin *JChkInit(JWin *self, char *title);

typedef struct JTop {
    JCnt cnt;
    struct JTop *PrevTop;
} JTop;

typedef struct JWnd {
	JTop top;
	void (*RightClick)();
	char *Label;
	int Flags;
} JWnd;

typedef struct JDlg {
	JWnd wnd;
	int Modal;
	int Done;
	JCnt Top;
	JCnt Bottom;
} JDlg;

extern JWin *JWndInit(JWin *self, char *title, int wndflags);
extern JWin JWndDefault(JWin *self, int type, int command, void *data);

extern JWin *JDlgInit(JWin *self, char *title, int modal, int wndflags);
extern void JDlgAddButtons(JDlg *Self, char *button, ...);
extern void JDlgAddContent(JDlg *Self, JWin *content);
extern int JDlgExec(JWin *self);

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
JScrF_HNotEnd	= 16,
JScrF_VNever	= 4,
JScrF_VAlways	= 8,
JScrF_VNotEnd	= 32
};

extern JView *JViewWinInit(JWin *Self, JWin *Win);

extern JWin *JScrInit(JWin *self, JWin *parent, int flags);
extern void JScrMax(JWin *self, long maxx, long maxy);

// JWinCallback(JW *, class, callback, func);
#define JWinCallback(a,b,c,d) ((b *)a)->c = d
	
#define MENF_Disabled	1
#define MENF_Tickable	2
#define MENF_Ticked	4
#define MENF_Line	8

extern void *JAppInit(void *self, int channel);
extern void JAppDrain(void *self);
extern void JAppLoop(void *self);
extern void JAppSetMain(void *self, void *main);

typedef struct JEvent 
{
	int Type;
	int SubType;
	int Sender;
	int Recver;
	int TransX;
	int TransY;
	unsigned int NumRects;
	unsigned int DataSz;
	void *Data;
} JEvent;

extern void JPost(JEvent *event, void *data);

extern int VMC(int, void *, ...);
/* Virtual method call function */


extern int JRegInfo(int region, RegInfo *props);
extern int JShow(int region);
extern int JEGeom(int region, int x, int y, unsigned int xsize, unsigned int ysize);
extern void JWSetData(JWin *self, void *data);
extern void *JWGetData(JWin *self);

#define MJTxf_Entered	MJW_SIZE+0

extern JWin *JFilInit(JWin *self, int type);
enum {
    JFil_Stretchy=0,
    JFil_Rigid=1
};

extern JWin *JStxInit(JWin *self, char *text);

extern JWin *JTxtInit(JWin *self);
extern void JTxtAppend(JWin *self, char *str);
extern void JTxtClear(JWin *self);
extern void JTxtScrolled(JWin *self, long x, long y);

extern JWin *JLstInit(JWin *self, JWin *parent, int flags);
extern void JLstInsert(JWin *self, char *label, void *insertp, void *data);

extern JWin *JCntInit(JWin *self);
extern void JWinGetHints(JWin *self, SizeHints *sizes);
extern void JCntAdd(JWin *Self, JWin *child);
extern JWClazz JCntClass;
enum {
	JCntF_LeftRight=0,
	JCntF_RightLeft=1,
	JCntF_TopBottom=2,
	JCntF_BottomTop=3
};
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

#define CMD_EXIT	1
#define CMD_CLOSE	2
#define CMD_MAX		3
#define CMD_MINI	4
#define CMD_RESTORE	5

enum {
JF_Added = 1,
JF_Valid = 2,
JF_Selected = 4,
JF_Focused = 8,
JF_Selectable = 16,
JF_Front = 32,
JF_InParent = 64,
JF_Manage = 128,
JF_Repainted = 256
};

#define JWndF_Resizable	1

extern void GfxSetPen(int x, int y);
extern void GfxString(unsigned char *str, ...);
extern void GfxGetOffs(int *xy);
extern void GfxSetOffs(int x, int y);
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

extern void JViewSync(JWin *self);

typedef struct JTreeCol {
	JCnt jcnt;
	struct JTree *Tree;
	JW *Title;
	unsigned long Offset;
	unsigned int Type;
} JTreeCol;

typedef struct JTree {
	JView JViewParent;
	struct TModel *Model;
	struct VNode *Root;
	void (*Clicked)();
	int YScroll;
	JTreeCol *SortCol;
	int SortDesc;
	int HasCols;
	int SelPolicy;
} JTree;

enum {
    JTreeP_Multiple=0,
    JTreeP_Single=1,
    JTreeP_None=2
};

typedef struct MJTre {
MJView mjview;
} MJTre;


/* ----------------------------
   Tree/List Model interfaces 
---------------------------- */

typedef struct TNode {
    struct VNode *NextView;
    int Flags;
} TNode;

typedef struct VNode {
    struct VNode *NextView;
    int Flags;
    JTree *Tree;
    Vec *Children;
    struct VNode *Parent;
    TNode *Value;    
} VNode;

typedef struct TModel {
    JObj jobj;
} TModel;

typedef struct MTModel {
TNode *(*Root)(TModel *Self);
JIter *(*Iter)(TModel *Self, TNode *node);
void (*FinIter)(TModel *Self, JIter *iter);
int (*Count)(TModel *Self, TNode *node);
void (*Expand)(TModel *Self, TNode *node);
} MTModel;

TNode *TModelRoot(TModel *Self);
JIter *TModelIter(TModel *Self, TNode *node);
void TModelFinIter(TModel *Self, JIter *iter);
int TModelCount(TModel *Self, TNode *node);
void TModelExpand(TModel *Self, TNode *node);


/* ----------------------------
    Default Tree/List Model
---------------------------- */

typedef struct DefNode {
    TNode tnode;
    Vec *Children;
    struct DefNode *Parent;
} DefNode;

typedef void(TreeExpander)(struct JTModel *, void *);

typedef struct JTModel {
    TModel tmodel;
    DefNode *Root;
    void(*Expander)(struct JTModel *, void *);
} JTModel;

typedef struct JLModel {
    TModel tmodel;
    TNode Root;
    Vec *Vec;
} JLModel;

extern JTModel *JTModelInit(JTModel *Self, DefNode *RootNode, TreeExpander *expander);
extern void JTModelAppend(JTModel *Self, DefNode *Parent, DefNode *Node);
extern void JTModelRemove(DefNode *Node);

extern JLModel *JLModelInit(JLModel *Self);
extern void JLModelAppend(JLModel *Self, TNode *Node);
extern void JLModelRemove(JLModel *Self, DefNode *Node);

/* ----------------------------
---------------------------- */

extern JTreeCol *JTreeColInit(JTreeCol *self, JTree *tree, char *title, int width, void *offs, int type);

extern JTree *JTreeInit(void *self, TModel *model);
extern void JTreeAddColumns(void *self, void **cols, ...);
extern VNode *JTreeAddView(JTree *Self, VNode *Parent, TNode *Node);

typedef struct JCombo {
    JCnt jcnt;
    JTree *Tree;
    JW *Popup;
    JScr *ListScr;
    int Type;
    uint32 Offs;
    TNode *Value;
    void (*Changed)(struct TNode *);
} JCombo;

extern JCombo *JComboInit(JCombo *Self, TModel *model, uint32 offs, int Type);

#define OFFSET(a,b) (&((a *)0)->b)
#define OFFSET32(a,b) ((unsigned long)(&((a *)0)->b))
#define OFFSET16(a,b) ((unsigned int)(unsigned long)(&((a *)0)->b))
#define METHOD(a,b) OFFSET16(a,b)

enum {
    JTabF_Fill = -1000,
    JTabF_Preferred = -1001,
    JTabF_Minimum = -1002,
    JTabF_Center = 1,
    JTabF_Left = 2,
    JTabF_Right = 3
};
    
extern JTab *JTabInit(JTab *Self, int *Cols, int *Rows, int ncols, int nrows);
extern JObjClass JTabClass;

enum {
EVS_User = 0,
EVS_Added,
EVS_Deleted,
EVS_PropChange,
EVS_Hidden,
EVS_Changed,
EVS_ReqChange,
EVS_ReqShow,
EVS_LostMouse,
EVS_Shown
};

#define JW(a) ((JW *)(a))

/* 
JSTd
----
Standard labels
*/

typedef struct JStdItem 
{
    char *ID;
    char *Label;
} JStdItem;

extern int JStdLookup(const char *ID, JStdItem *ret);

#define JSTD_OK     "!OK"
#define JSTD_CANCEL "!CANCEL"
#define JSTD_YES    "!YES"
#define JSTD_NO     "!NO"
#define JSTD_APPLY  "!APPLY"

#endif
