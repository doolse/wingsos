#include <sys/types.h>

#ifndef __JOS__
typedef long int32;
typedef unsigned long uint32;
typedef short int16;
typedef unsigned short uint16;
typedef unsigned char uchar;
#ifdef _WIN32
typedef unsigned int uint;
#endif
#endif

#define MAXLINE 512
#define MAXLABEL 64

extern uchar map[256];
extern char *mnetab[];

enum {
	O_LIBRARY=1,
	O_STAY=2
};

enum {
	S_NOCROSS=1,
	S_PALIGN=2,
	S_DBR=4,
	S_BLANK=8,
	S_RO=16,
	S_PIC=32
};

enum {
	SUNDEF=0,
	SABS,
	SFOPT,
	SCODE,
	SDATA,
	SBSS
};
enum {
	INFO=1,
	IMPORT,
	EXPORT,
	LINKS,
	SEGMENTS,
	ENDFILE
};

enum {
	EOI=128,
	VALUE,
	LABEL,
	MNEMO,
	PSUEDO,
	MACRO,
	STRING,
	EQ,
	NEQ,
	LEQ,
	GEQ,
	OR,
	AND,
	JUMP,
	SHL,
	SHR,
	LINENO,
	NFILE,
	SEGMENT,
	SETABSPC,
	EXEND
};

typedef struct lab {
	struct lab *next;
	struct lab *prev;
	char *str;
	int32 value;
	int seg;
	int impnum;
	int canredef;
	int hasdef;
	int exported;
} Label;

typedef struct mc {
	struct mc *next;
	struct mc *prev;
	char *str;
	char *mbuf;
} Macro;

typedef struct {
	uchar *relbuf;
	uint32 relrsize;
	uint32 relsize;
	uchar *bufptrs;
	uchar *sbufups;
	uchar *lastrel;
	uint flags;
	uint32 startpc;
	uint32 curpc;
	uint32 size;
} Segment;

typedef struct ibuf{
	int type;
	struct ibuf *next;
	char *bufp;
	char *bufupp;
	char *limp;
	FILE *fp;
	char *name;
	char *thisdir;
	uint linenum;
	uint multi;
} Inbuf;

typedef struct ll {
	struct ll *next;
	char *name;
	uint version;
} LLink;

enum { BLANK=01,  NEWLINE=02, LETTER=04,
       DIGIT=010, HEX=020,    OTHER=040 };

enum {
RERR=0,
RWORD,
RHIGH,
RLOW,
RSEGADR,
RSEG,
RSOFFL,
RSOFFH,
ROFFS,
RLONG
};

enum {
	Opcodes=95,
	IMPL=0, ZP, ZPX, ZPY, ZPI, ZPIX, ZPIY, IMM, ABS, ABSX, ABSY, REL, 
	ABSI, ABSIX, DUNNO, DUNNO2, RELL, ABSL, ABSLX, SREL, SRELIY,
	ZPIL, ZPILY, ABSIL, Admodes,
	P24=0, PAS, PAL, PASC, PABS, PASS, PBYTE, PBSS, PBIN, PCODE, PDO, PDATA, 
	PDSB, PDEF, PDFT, PELSE, PENDIF, PEXPORT, PFOPT, PIF, PINCLUDE, PLONG, 
	PLINK, PMAC, PMEND, PNOMUL, POPT, PPSC, PPIC, PSCR, PSTACK, PSTRUCT, PSTEND, 
	PTEXT, PUNTIL, PWORD, PWHILE, PWEND, PXL, PXS, PZERO, PBLOCK, PBEND, PIFN
};

extern int optab[Opcodes][Admodes];

extern uchar toxindex[Admodes];
extern uchar toyindex[Admodes];
extern uchar tozp[Admodes];
extern uchar tolong[Admodes];
extern uchar tostack[Admodes];
extern uchar adlen[Admodes];

extern int getmne(char *);
extern int getpsu(char *);
extern uchar *makemin(uint size);
extern void inittarget();
extern void procopt(int opt, char *arg);

extern char *sysdir;
extern void *mymalloc(uint32);
extern uchar ident[MAXLABEL+1];
extern uint linenum;
extern uchar *parsed;
extern Segment segbufs[16];
extern Segment *curseg;
extern uint cursegnum;
extern uint numsegs;
extern Inbuf *curfile;
extern Label *LabTable[256];
extern uchar *outbuf;
extern uint32 realsize;
extern uint32 outsize;
extern int deftrans;
extern int verbose;
extern int noglobs;
extern int norel;
extern int c64exec;
extern int nodfname;
extern char *outname;
extern uint stacksize;

