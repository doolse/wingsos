#include <stdio.h>
#include <wgslib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#define MAXLINE 512

#define debout(a) 

unsigned int linenum=1;
unsigned int globnum;
unsigned int funcnum;
unsigned int locnum;
unsigned int parnum;
unsigned int varnum;
unsigned int strnum;
int glob;

unsigned int markup;

typedef struct syment {
	struct syment *Next;
	struct syment *Prev;
	char *name;
	unsigned int number;
	int assigned;
	unsigned int offset;
} Symbol;

unsigned int marks[256];

typedef struct {
	char *buf;
	char *upto;
	unsigned int size;
	unsigned int used;
} ByteBuf;


Symbol *Globals = NULL;
Symbol *Locals = NULL;
Symbol *Funcs = NULL;
Symbol *StrGlobs = NULL;

ByteBuf *CurBuf;
ByteBuf *MainBuf;
ByteBuf *TempBuf;
ByteBuf *FuncBuf;

enum {
	SVal = 1,
	IVal = 2
} Flags;

enum { 
LDA8, 
LDA16, 
LDA32, 
LDAT,
LDAS,
LDAG,
ADD,
SUB,
MUL,
DIV,
CAT,
STAS,
STAG,
PHA,
PLA,
LOCL,
CMPL,
CMPLE,
CMPEQ,
CMPNE,
CMPG,
CMPGE,
BEQ,
BNE,
BRA,
RET,
JSR,
NOT,
MOD
} opcode;

typedef struct {
	char *name;
	long val;
	unsigned int strsize;
	int valid;
	int type;
} Variable;

typedef struct {
	char *pos;
	void (*func)(Variable *sp, unsigned int args);
} Function;

Function runfunc[256];

Variable a;

Variable stack[256];
Variable globals[256];

enum {
	FOR,
	IF,
	ELSE,
	DO,
	WHILE,
	RETURN,
	FUNC,
	EOI,
	ID,
	INCR,
	DECR,
	EQ,
	NEQ,
	LEQ,
	GEQ,
	ICON,
	SCON,
	OR,
	AND
} keywords;

enum { BLANK=01,  NEWLINE=02, LETTER=04,
       DIGIT=010, HEX=020,    OTHER=040 };

static unsigned char map[256] = { /* 000 nul */	0,
				   /* 001 soh */	0,
				   /* 002 stx */	0,
				   /* 003 etx */	0,
				   /* 004 eot */	0,
				   /* 005 enq */	0,
				   /* 006 ack */	0,
				   /* 007 bel */	0,
				   /* 010 bs  */	0,
				   /* 011 ht  */	BLANK,
				   /* 012 nl  */	NEWLINE,
				   /* 013 vt  */	BLANK,
				   /* 014 ff  */	BLANK,
				   /* 015 cr  */	0,
				   /* 016 so  */	0,
				   /* 017 si  */	0,
				   /* 020 dle */	0,
				   /* 021 dc1 */	0,
				   /* 022 dc2 */	0,
				   /* 023 dc3 */	0,
				   /* 024 dc4 */	0,
				   /* 025 nak */	0,
				   /* 026 syn */	0,
				   /* 027 etb */	0,
				   /* 030 can */	0,
				   /* 031 em  */	0,
				   /* 032 sub */	0,
				   /* 033 esc */	0,
				   /* 034 fs  */	0,
				   /* 035 gs  */	0,
				   /* 036 rs  */	0,
				   /* 037 us  */	0,
				   /* 040 sp  */	BLANK,
				   /* 041 !   */	OTHER,
				   /* 042 "   */	OTHER,
				   /* 043 #   */	OTHER,
				   /* 044 $   */	0,
				   /* 045 %   */	OTHER,
				   /* 046 &   */	OTHER,
				   /* 047 '   */	OTHER,
				   /* 050 (   */	OTHER,
				   /* 051 )   */	OTHER,
				   /* 052 *   */	OTHER,
				   /* 053 +   */	OTHER,
				   /* 054 ,   */	OTHER,
				   /* 055 -   */	OTHER,
				   /* 056 .   */	OTHER,
				   /* 057 /   */	OTHER,
				   /* 060 0   */	DIGIT,
				   /* 061 1   */	DIGIT,
				   /* 062 2   */	DIGIT,
				   /* 063 3   */	DIGIT,
				   /* 064 4   */	DIGIT,
				   /* 065 5   */	DIGIT,
				   /* 066 6   */	DIGIT,
				   /* 067 7   */	DIGIT,
				   /* 070 8   */	DIGIT,
				   /* 071 9   */	DIGIT,
				   /* 072 :   */	OTHER,
				   /* 073 ;   */	OTHER,
				   /* 074 <   */	OTHER,
				   /* 075 =   */	OTHER,
				   /* 076 >   */	OTHER,
				   /* 077 ?   */	OTHER,
				   /* 100 @   */	0,
				   /* 101 A   */	LETTER|HEX,
				   /* 102 B   */	LETTER|HEX,
				   /* 103 C   */	LETTER|HEX,
				   /* 104 D   */	LETTER|HEX,
				   /* 105 E   */	LETTER|HEX,
				   /* 106 F   */	LETTER|HEX,
				   /* 107 G   */	LETTER,
				   /* 110 H   */	LETTER,
				   /* 111 I   */	LETTER,
				   /* 112 J   */	LETTER,
				   /* 113 K   */	LETTER,
				   /* 114 L   */	LETTER,
				   /* 115 M   */	LETTER,
				   /* 116 N   */	LETTER,
				   /* 117 O   */	LETTER,
				   /* 120 P   */	LETTER,
				   /* 121 Q   */	LETTER,
				   /* 122 R   */	LETTER,
				   /* 123 S   */	LETTER,
				   /* 124 T   */	LETTER,
				   /* 125 U   */	LETTER,
				   /* 126 V   */	LETTER,
				   /* 127 W   */	LETTER,
				   /* 130 X   */	LETTER,
				   /* 131 Y   */	LETTER,
				   /* 132 Z   */	LETTER,
				   /* 133 [   */	OTHER,
				   /* 134 \   */	OTHER,
				   /* 135 ]   */	OTHER,
				   /* 136 ^   */	OTHER,
				   /* 137 _   */	LETTER,
				   /* 140 `   */	0,
				   /* 141 a   */	LETTER|HEX,
				   /* 142 b   */	LETTER|HEX,
				   /* 143 c   */	LETTER|HEX,
				   /* 144 d   */	LETTER|HEX,
				   /* 145 e   */	LETTER|HEX,
				   /* 146 f   */	LETTER|HEX,
				   /* 147 g   */	LETTER,
				   /* 150 h   */	LETTER,
				   /* 151 i   */	LETTER,
				   /* 152 j   */	LETTER,
				   /* 153 k   */	LETTER,
				   /* 154 l   */	LETTER,
				   /* 155 m   */	LETTER,
				   /* 156 n   */	LETTER,
				   /* 157 o   */	LETTER,
				   /* 160 p   */	LETTER,
				   /* 161 q   */	LETTER,
				   /* 162 r   */	LETTER,
				   /* 163 s   */	LETTER,
				   /* 164 t   */	LETTER,
				   /* 165 u   */	LETTER,
				   /* 166 v   */	LETTER,
				   /* 167 w   */	LETTER,
				   /* 170 x   */	LETTER,
				   /* 171 y   */	LETTER,
				   /* 172 z   */	LETTER,
				   /* 173 {   */	OTHER,
				   /* 174 |   */	OTHER,
				   /* 175 }   */	OTHER,
				   /* 176 ~   */	OTHER, };


FILE *fp;
int gargc;
char **gargv;
char buf[MAXLINE+1];
char *bufup=buf;
char *limit=buf;
char *ident;
long con;
char scon[MAXLINE+1];
int t;

/* System calls! */

char *strings[256];

typedef struct envs {
	struct envs *Next;
	struct envs *Prev;
	char *name;
	char *val;
} Env;

Env *EnvVars;
Env *QueryVars;
Env *PostVars;

typedef struct {
	char *name;
	void (*func)(Variable *sp, unsigned int args);	
} System;

Symbol *findSymbol(Symbol *head, char *str) {
	Symbol *cur = head;
	if (!head)	
		return NULL;
	do {
		if (!strcmp(cur->name, str))
			return cur;
		cur = cur->Next;
	} while (cur != head);
	return NULL;
}

void decatr(char *str) {
	char *d=str;
	int temp;
	
	while (1) {
		switch (*str) {
		case '%':
			temp = str[3];
			str[3] = 0;
			*d = strtol(str+1, &str, 16);
			*str = temp;
			break;
		case '+':
			*d = ' ';
			str++;
			break;
		case 0:
			*d = 0;
			return;
		default:
			*d = *str;
			str++;
		}
		d++;
	}
}

void prepEnv(char *wstr, Env **theenv) {
	char *cur;
	Env *eup,*head = NULL;

	while (cur = strsep(&wstr, "&")) {
		eup = malloc(sizeof(Env));
		head = addQueue(head, head, eup);
		eup->name = strsep(&cur, "=");
		eup->val = strsep(&cur, "");
		decatr(eup->val);
	}
	*theenv = head;
}

void webinit(Variable *bp, unsigned int args) {
	int opt;
	Env *s;
	
	while ((opt = getopt(gargc, gargv, "i:")) != EOF) {
		switch(opt) {
			case 'i':
				prepEnv(optarg, &EnvVars);
				s = (Env *) findSymbol((Symbol *) EnvVars, "QUERY");
				if (s) 
					prepEnv(strdup(s->val), &QueryVars);
				s = (Env *) findSymbol((Symbol *) EnvVars, "POSTED");
				if (s) 
					prepEnv(strdup(s->val), &PostVars);
				break;
		}
	}
}

void webexist(Variable *bp, unsigned int args) {
	Env *s;
	
	makeNum(&a);
	s = (Env *) findSymbol((Symbol *) QueryVars, bp->name);
	if (!s) s = (Env *) findSymbol((Symbol *) PostVars, bp->name);
	if (!s) a.val = 0;
	else a.val = 1;
	a.valid = IVal;
}

void webparam(Variable *bp, unsigned int args) {
	Env *s;
	
	s = (Env *) findSymbol((Symbol *) QueryVars, bp->name);
	if (!s) s = (Env *) findSymbol((Symbol *) PostVars, bp->name);
	if (!s || !s->val) copystr("", &a);
	else copystr(s->val, &a);
}

void webinfo(Variable *bp, unsigned int args) {
	Env *s;
	
	s = (Env *) findSymbol((Symbol *) EnvVars, bp->name);
	if (!s || !s->val) copystr("", &a);
	else copystr(s->val, &a);
}

void fprint(Variable *bp, unsigned int args) {
	
	makeSure(&bp[1]);
	fprintf((FILE *) bp->val, "%s", bp[1].name);
}

void fprintnl(Variable *bp, unsigned int args) {
	
	makeSure(&bp[1]);
	fprintf((FILE *)bp->val, "%s\n", bp[1].name);
}

void print(Variable *bp, unsigned int args) {
	
	makeSure(bp);
	printf("%s", bp->name);
}

void printnl(Variable *bp, unsigned int args) {
	
	makeSure(bp);
	printf("%s\n", bp->name);
}

void getline_s(Variable *bp, unsigned int args) {
	
	if (getline(&a.name, (int *) &a.strsize, (FILE *)bp->val) != -1)
		a.valid = SVal;
	else copystr("", &a);
}

void fopen_s(Variable *bp, unsigned int args) {
	
	a.val = (long) fopen(bp->name, bp[1].name);
	a.valid = IVal;
}

void fclose_s(Variable *bp, unsigned int args) {
	
	fclose((FILE *)bp->val);
}

void uptime_s(Variable *bp, unsigned int args) {
	
	a.val = time(NULL) - sysup();
	a.valid = IVal;
}

System SysCalls[] = {
{"uptime", uptime_s},
{"webparam", webparam},
{"webinfo", webinfo},
{"webinit", webinit},
{"webexist", webexist},
{"print", print},
{"printnl", printnl},
{"fprint", fprint},
{"fprintnl", fprintnl},
{"fopen", fopen_s},
{"fclose", fclose_s},
{"getline", getline_s},
{NULL, NULL}
};

char *disasm(char *op) {
	int code;
	
	debout(fprintf(stderr, "%06lx ",op));
	code = *op++;
	switch(code) {
		case LOCL:
			debout(fprintf(stderr, "LOCL %d\n",*op));
			return op+1;
		case LDA8:
			debout(fprintf(stderr, "LDA8 %d\n",*op));
			return op+1;
		case LDA16:
			debout(fprintf(stderr, "LDA16 %d\n", * (int *)op));
			return op+2;
		case LDA32:
			debout(fprintf(stderr, "LDA32 %ld\n", * (long *)op));
			return op+4;
		case LDAS:
			debout(fprintf(stderr, "LDAS %d\n", *op));
			return op+1;
		case LDAT:
			debout(fprintf(stderr, "LDAT %d\n", * (int *)op));
			return op+2;
		case ADD:
			debout(fprintf(stderr, "ADD\n"));
			return op;
		case SUB:
			debout(fprintf(stderr, "SUB\n"));
			return op;
		case MUL:
			debout(fprintf(stderr, "MUL\n"));
			return op;
		case DIV:
			debout(fprintf(stderr, "DIV\n"));
			return op;
		case CMPL:
			debout(fprintf(stderr, "CMPL\n"));
			return op;
		case CMPLE:
			debout(fprintf(stderr, "CMPLE\n"));
			return op;
		case CMPG:
			debout(fprintf(stderr, "CMPG\n"));
			return op;
		case CMPGE:
			debout(fprintf(stderr, "CMPGE\n"));
			return op;
		case CMPEQ:
			debout(fprintf(stderr, "CMPEQ\n"));
			return op;
		case CMPNE:
			debout(fprintf(stderr, "CMPNE\n"));
			return op;
		case BRA:
			debout(fprintf(stderr, "BRA %06lx,(%d)\n", (long) *(int *)op + op, * (int *)op));
			return op+2;
		case BEQ:
			debout(fprintf(stderr, "BEQ %06lx,(%d)\n", (long) *(int *)op + op, * (int *)op));
			return op+2;
		case BNE:
			debout(fprintf(stderr, "BNE %06lx,(%d)\n", (long) *(int *)op + op, * (int *)op));
			return op+2;
		case PHA:
			debout(fprintf(stderr, "PHA\n"));
			return op;
		case STAS:
			debout(fprintf(stderr, "STAS %d\n", *op));
			return op+1;
		case STAG:
			debout(fprintf(stderr, "STAG %d\n", * (int *)op));
			return op+2;
		case RET:
			debout(fprintf(stderr, "RET\n"));
			return op;
		case CAT:
			debout(fprintf(stderr, "CAT\n"));
			return op;
		case JSR:
			debout(fprintf(stderr, "JSR %d\n", * (int *)op));
			return op+2;
		default:
			debout(fprintf(stderr, "Unrecognised %d\n",code));
			return op;
	}
}

char *makestr(char *str, unsigned int size) {
	char *ret = malloc(size+1);
	strncpy(ret, str, size);
	ret[size] = 0;
	return ret;
}

void fillbuf() {
	int bsize;
	char *s = buf,*cp = bufup;
	while (cp < limit) {
		*s = *cp;
		s++;
		cp++;
	}
	if (feof(fp)) {
		bsize = 0;
	} else {
		bsize = fread(s, 1, &buf[MAXLINE] - s, fp);
	}
	if (!bsize) {
		*s = -1;
		bsize = 1;
	} 
	limit = s + bsize;
	bufup = buf;
	*limit = 0;
}

char *chkbuf(char *cp) {
	if (limit - cp < 32) {
		bufup = cp;
		fillbuf();
		return bufup;
	}
	return cp;
}

void doscon() {
	char *cp = bufup;
	char *s = scon;
	int cur;
	int exit = t;
	
	while (1) {
		cur = *cp;
		cp++;
		if (!cur) {
			cp = chkbuf(cp);
			cur = *cp;
			cp++;
		}
		if (cur == exit || cur == '\n') {
			if (cur == '\n') {
				*s = '\n';
				s++;
				linenum++;
			}
			*s = 0;
			bufup = cp;
			ident = strdup(scon);
			return; 
		} else 
		if (cur == '\\') {
			cp = chkbuf(cp);
			cur = *cp++;
			switch(cur) {
				case 'n':
					cur = '\n';
					break;
				case 'r':
					cur = '\r';
					break;
				case 'b':
					cur = '\b';
					break;
				case 't':
					cur = '\t';
					break;
			}	
		}
		*s = cur;
		s++;
	}
}


void gettok() {
	char *cp;
	char *token;
	
	cp = bufup;
	while (1) {
		while (map[*cp]&BLANK)
			cp++;
		cp = chkbuf(cp);
		bufup = cp + 1;
		t = *cp;
		cp++;
		switch(t) {
		case '/':
			if (cp[0] == '*') {
				int done = 0, inquote = 0;
				cp++;
				while (!done) {
					switch(*cp) {
					case '"':
						inquote = inquote ^ 1;
						break;
					case '\\':
						if (inquote) {
							cp = chkbuf(cp);
							cp++;
						}
						break;
					case '*':
						if (!inquote) {
							cp = chkbuf(cp);
							cp++;
							if (*cp == '/') {
								done = 1;
								cp++;
							}
						}
						break;
					case '\n':
						linenum++;
						break;
					case 0:
						cp = chkbuf(cp);
						break;
					}
					cp++;
				}
			} else return;
			break;
		case 'i':
			if (cp[0] == 'f' && !(map[cp[1]]&(DIGIT|LETTER))) {
				bufup = cp + 1;
				t = IF;
				return;
			}
			goto id;
		case 'e':
			if (cp[0] == 'l' && cp[1] == 's' && cp[2] == 'e' && !(map[cp[3]]&(DIGIT|LETTER))) {
				bufup = cp + 3;
				t = ELSE;
				return;
			}
			goto id;
		case 'd':
			if (cp[0] == 'o' && !(map[cp[1]]&(DIGIT|LETTER))) {
				bufup = cp + 1;
				t = DO;
				return;
			}
			goto id;
		case 'w':
			if (cp[0] == 'h' && cp[1] == 'i' && cp[2] == 'l' && cp[3] == 'e' && !(map[cp[4]]&(DIGIT|LETTER))) {
				bufup = cp + 4;
				t = WHILE;
				return;
			}
			goto id;
		case 'r':
			if (cp[0] == 'e' && cp[1] == 't' && cp[2] == 'u' && cp[3] == 'r' && cp[4] == 'n' && !(map[cp[5]]&(DIGIT|LETTER))) {
				bufup = cp + 5;
				t = RETURN;
				return;
			}
			goto id;
		case 'f':
			if (cp[0] == 'o' && cp[1] == 'r' && !(map[cp[2]]&(DIGIT|LETTER))) {
				bufup = cp + 2;
				t = FOR;
				return;
			} else
			if (cp[0] == 'u' && cp[1] == 'n' && cp[2] == 'c' && !(map[cp[3]]&(DIGIT|LETTER))) {
				bufup = cp + 3;
				t = FUNC;
				return;
			}
			goto id;
		case '<':
			if (cp[0] == '=') {
				++bufup;
				t = LEQ;
			}
			return;
		case '>':
			if (cp[0] == '=') {
				++bufup;
				t = GEQ;
			}
			return;
		case '=':
			if (cp[0] == '=') {
				++bufup;
				t = EQ;
			}
			return;
		case '!':
			if (cp[0] == '=') {
				++bufup;
				t = NEQ;
			}
			return;
		case '-':
			if (cp[0] == '-') {
				++bufup;
				t = DECR;
			}
			return;
		case '+':
			if (cp[0] == '+') {
				++bufup;
				t = INCR;
			}
			return;
		case '&':
			if (cp[0] == '&') {
				++bufup;
				t = AND;
			}
			return;
		case '|':
			if (cp[0] == '|') {
				++bufup;
				t = OR;
			}
			return;
		case ' ': case '\t': case 0:
			break;
		case '$': case '*': case '(': case ')': case ';':
		case ':': case '{': case '}': case ',': case '%':
		case '@':
			return;			
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9':
			con = strtol(cp-1, &cp, 0);
			bufup = cp;
			t = ICON;
			return;
		case '\n': case '\r':
			linenum++;
			break;
		case '"': case '\'':
			doscon();
			t = SCON;
			return;
		case -1:
			t = EOI;
			return;
		default:
			id:
			token = cp-1;
			while (map[*cp]&(DIGIT|LETTER))
				cp++;
			ident = makestr(token, cp-token);
			bufup = cp;
			t = ID;
			return;
		}
	}
}

char *nametok(int t) {
	static char tokename[2]=".";
	
	switch(t) {
	case FOR:
		return("for");
	case IF:
		return("if");
	case ELSE:
		return("else");
	case DO:
		return("do");
	case WHILE:
		return("while");
	case RETURN:
		return("return");
	case FUNC:
		return("func");
	case EOI:
		return("eoi");
	case ID:
		return("identifier");
	case INCR:
		return("++");
	case DECR:
		return("--");
	case EQ:
		return("==");
	case GEQ:
		return(">=");
	case LEQ:
		return("<=");
	case NEQ:
		return("!=");
	case ICON:
		return("integer constant");
	case SCON:
		return("string constant");
	default:
		tokename[0] = t;
		return tokename;
	}
}

void resetbuf(ByteBuf *bbuf) {
	bbuf->used = 0;
	bbuf->upto = bbuf->buf;
}

void freebuf(ByteBuf *bbuf) {
	if (bbuf->buf)
		free(bbuf->buf);
	free(bbuf);
}

ByteBuf *newbuf() {
	ByteBuf *bbuf;
	bbuf = malloc(sizeof(ByteBuf));
	bbuf->size = 0;
	bbuf->used = 0;
	bbuf->buf = NULL;
	bbuf->upto = NULL;
	return bbuf;
}

void addbyte(unsigned int byte) {
	ByteBuf *cbuf = CurBuf;
	if (cbuf->size <= cbuf->used) {
		cbuf->size += 512;
		cbuf->buf = realloc(cbuf->buf, cbuf->size);
		cbuf->upto = cbuf->buf + cbuf->used;
	}
	*cbuf->upto++ = byte;
	cbuf->used++;
}

void catbuf(ByteBuf *from) {
	ByteBuf *to = CurBuf;
	
	from->upto = from->buf;	
	while (from->used) {
		addbyte(*from->upto++);
		from->used--;
	}
}

void addword(int cnst) {
	addbyte(cnst & 255);
	addbyte(cnst >> 8);
}

void putop(unsigned int byte) {
	addbyte(byte);
}

Symbol *addstrs(char *str) {
	Symbol *sym = findSymbol(StrGlobs, str);
	if (!sym) {
		sym = malloc(sizeof(Symbol));
		StrGlobs = addQueue(StrGlobs, StrGlobs, sym);
		sym->name = str;
		sym->number = strnum++;
	}
	varnum = sym->number;
	return sym;
}

void putcon(long cnst) {
	if (cnst >= -128 && cnst < 128) {
		putop(LDA8);
		addbyte((int) cnst);
		return;
	}
	if (cnst >= -32768 && cnst < 32768) {
		putop(LDA16);
		addbyte((int) cnst & 255);
		addbyte((int) cnst >> 8);
		return;
	}
	putop(LDA32);
	addbyte(cnst & 255);
	addbyte(cnst >> 8);
	addbyte(cnst >> 16);
	addbyte(cnst >> 24);
	return;
}

void error(char *str, ...) {
	char *args;
	
	va_start(args, str);
	fprintf(stderr, "Error on line %d: ",linenum);
	vfprintf(stderr, str, args);
	exit(1);
}

void expect(int type) {
	if (t != type) {
		char *tem = strdup(nametok(t));
		error("Syntax error: Found %s looking for %s\n", tem, nametok(type));
	} else gettok();
}

void cleanLocals() {
	Symbol *cur = Locals, *nxt;
	
	if (!Locals)
		return;
	do {
		nxt = cur->Next;
		free(cur->name);
		free(cur);
		cur = nxt;
	} while (cur != Locals);
	locnum = 0;
	Locals = NULL;
}


Symbol *addfunc(int assign, unsigned int offset) {
	Symbol *sym = findSymbol(Funcs, ident);
	if (!sym) {
		sym = malloc(sizeof(Symbol));
		Funcs = addQueue(Funcs, Funcs, sym);
		sym->name = ident;
		sym->number = funcnum++;
		sym->assigned = 0;
	} else {
		if (assign && sym->assigned)
			fprintf(stderr, "Multiple definition of %s\n", ident);
	}
	if (assign) {
		sym->assigned = assign;
		sym->offset = offset;
	}
	varnum = sym->number;
	return sym;
}

Symbol *addlocal() {
	Symbol *sym = findSymbol(Locals, ident);
	if (!sym) {
		sym = malloc(sizeof(Symbol));
		Locals = addQueue(Locals, Locals, sym);
		sym->name = ident;
		sym->number = locnum++;
	}
	varnum = sym->number;
	return sym;
}

Symbol *addglobal() {
	Symbol *sym = findSymbol(Globals, ident);
	if (!sym) {
		sym = malloc(sizeof(Symbol));
		Globals = addQueue(Globals, Globals, sym);
		sym->name = ident;
		sym->number = globnum++;
	}
	varnum = sym->number;
	return sym;
}

void docomp() {
	expect('{');
	while (t != '}') {
		dostmt();
	}
	expect('}');
}

void relop(int type) {	
	gettok();
	putop(PHA);
	doexpr();
	putop(type);
}

void doterm2() {
	Symbol *sym;
	
	glob = 0;
	if (t == '$') {
		gettok();
		glob = 1;
		if (t != ID)
			expect(ID);
	}
	switch(t) {
		case '(':
			expect('(');
			doexpr2();
			expect(')');
			break;
		case ID:
			gettok();
			if (t == '(') {
				int parms=0;
				int vnum;
			
				addfunc(0, 0);
				vnum = varnum;
				expect('(');
				while(t != ')') {
					doexpr2();
					putop(PHA);
					parms--;
					if (t != ')') 
						expect(',');
				}
				expect(')');
				putop(JSR);
				addword(vnum);
				putop(LOCL);
				addbyte(parms);
			} else {
				if (glob)
					addglobal();
				else addlocal();
				if (t == '=') 
					return;
				if (glob) {
					putop(LDAG);
					addword(varnum);
				} else {
					putop(LDAS);
					addbyte(varnum);
				}
			}
			break;
		case ICON:
			putcon(con);
			gettok();
			break;	
		case SCON:
			{
			char *temp = ident;
			gettok();
			while (t == SCON) {
				temp = realloc(temp, strlen(ident) + strlen(temp) + 1);
				strcat(temp, ident);
				free(ident);
				gettok();
			}
			putop(LDAT);
			addstrs(temp);
			addword(varnum);
			}
			break;	
		default:
			error("Expected term! Found %s\n", nametok(t));
			break;
	}
}

void doterm() {

	if (t == '!') {
		gettok();
		doterm();
		putop(NOT);
		return;
	}
	doterm2();
	while (1) {
		switch(t) {
		case '*':
			gettok();
			putop(PHA);
			doterm2();
			putop(MUL);
			break;
		case '/':
			gettok();
			putop(PHA);
			doterm2();
			putop(DIV);
			break;
		case '%':
			gettok();
			putop(PHA);
			doterm2();
			putop(MOD);
			break;
		default:
			return;
		}
	}
}

int opassign() {
	gettok();
	putop(PHA);
	if (t == '=') {
		gettok();
		doexpr();
		return 1;
	} 
	doterm();
	return 0;
}

void doexpr() {
	int assign = 0;
	int vnum, glob2, done = 0;
	
	doterm();
	vnum = varnum;
	glob2 = glob;
	while (!done) {
		switch(t) {
		case '+':
			assign = opassign();
			putop(ADD);
			break;
		case '-':
			assign = opassign();
			putop(SUB);
			break;
		case '@':
			assign = opassign();
			putop(CAT);
			break;
		default:
			done = 1;
		}
	}
	switch(t) {
		case '=':
			gettok();
			doexpr();
			assign = 1;
			break;
		case INCR:
			gettok();
			putop(PHA);
			putcon(1);
			putop(ADD);
			assign = 1;
			break;
		case DECR:
			gettok();
			putop(PHA);
			putcon(1);
			putop(SUB);
			assign = 1;
			break;
		case '<': 
			relop(CMPL);
			break;
		case '>': 
			relop(CMPG);
			break;
		case EQ: 
			relop(CMPEQ);
			break;
		case LEQ: 
			relop(CMPLE);
			break;
		case GEQ: 
			relop(CMPGE);
			break;
		case NEQ:
			relop(CMPNE);
			break;
	}
	if (assign) {
		if (glob2) {
			putop(STAG);
			addword(vnum);
		} else {
			putop(STAS);
			addbyte(vnum);
		}
	}
}

void doexpr2() {
	int shortcut = -1;
	
	doexpr();
	while (1) {
		switch(t) {
		case AND:
			gettok();
			if (shortcut == -1)
				shortcut = getlabels(1);
			dobra(BNE, shortcut);
			doexpr();
			break;
		case OR:
			gettok();
			if (shortcut == -1)
				shortcut = getlabels(1);
			dobra(BEQ, shortcut);
			doexpr();
			break;
		default:
			if (shortcut != -1)
				markit(shortcut);
			return;
		}
	}
}

int getlabels(int how) {
	int temp = markup;
	markup+=how;
	return temp;
}

void markit(int lab) {
	marks[lab] = FuncBuf->used;
}

void dobra(int type, int label) {
	putop(type);
	addword(label);
}

void dostmt() {
	int lab;
	ByteBuf *temp;
	
	switch(t) {
	case '{':
		docomp();
		break;
	case FOR:
		gettok();
		expect('(');
		doexpr2();
		expect(';');
		
		lab = getlabels(2);
		markit(lab);
		doexpr2();
		expect(';');
		dobra(BNE, lab+1);
		
		temp = newbuf();
		CurBuf = temp;
		doexpr2();
		expect(')');
		
		CurBuf = FuncBuf;
		dostmt();
		catbuf(temp);
		freebuf(temp);
		dobra(BRA, lab);
		markit(lab+1);
		break;
	case IF:
		gettok();
		expect('(');
		lab = getlabels(2);
		doexpr2();
		expect(')');
		dobra(BNE, lab);
		dostmt();
		if (t == ELSE) {
			gettok();
			dobra(BRA, lab+1);
			markit(lab);
			dostmt();
			markit(lab+1);
		} else markit(lab);
		break;
	case DO:
		gettok();
		lab = getlabels(1);
		markit(lab);
		dostmt();
		expect(WHILE);
		expect('(');
		doexpr2();
		expect(')');
		dobra(BEQ, lab);
		break;
	case WHILE:
		gettok();
		expect('(');
		lab = getlabels(2);
		markit(lab);
		doexpr2();
		expect(')');
		dobra(BNE, lab+1);
		dostmt();
		dobra(BRA, lab);
		markit(lab+1);
		break;
	case RETURN:
		gettok();
		doexpr2();
		putop(RET);
		break;
	case ';':
		gettok();
		break;
	default:
		doexpr2();
		expect(';');
		break;
	}
}

void assemble() {
	char *pc = FuncBuf->buf;
	char *end = FuncBuf->upto;
	int temp,offs;
	
	while (pc < end) {
		switch (*pc) {
		case STAS:
		case LDAS:
			*(pc+1) -= parnum;
			break;
		case BRA:
		case BEQ:
		case BNE:
			temp = * (int *)(pc+1);
			offs = pc+1 - FuncBuf->buf;
			*(int *)(pc+1) = marks[temp] - offs;
			break;
		}
		pc = disasm(pc);
	}
}

void dofunc() {
	resetbuf(FuncBuf);
	CurBuf = FuncBuf;
	expect(FUNC);
	addfunc(1, MainBuf->used);
	markup = 0;
	cleanLocals();
	parnum = 0;
	if (t != ID)
		expect(ID);
	gettok();
	expect('(');
	while (t != ')') {
		if (t == ID) {
			addlocal();
			parnum++;
		}
		gettok();
		if (t != ')') {
			expect(',');
			if (t != ID)
				expect(ID);
		}
	}
	expect(')');
	dostmt();
	putop(RET);
	locnum -= parnum;
	CurBuf = TempBuf;
	resetbuf(TempBuf);
	putop(LOCL);
	addbyte(locnum);
	CurBuf = MainBuf;
	catbuf(TempBuf);
	assemble();
	catbuf(FuncBuf);
	
}

void prepfuncs() {
	Symbol *cur = Funcs;
	Function *fpt;
	System *sysup;
	char *str;
	unsigned int i;
	
	do {
		fpt = &runfunc[cur->number];
		if (cur->assigned)
			fpt->pos = MainBuf->buf + cur->offset;
		else {
			sysup = &SysCalls[0];
			i = 0;
			while (sysup->name) {
				if (!strcmp(cur->name, sysup->name)) {
					fpt->func = sysup->func;
					break;
				}
				i++;
				sysup++;
			}
			if (!sysup->name)
				error("Couldn't find call \"%s\"\n", cur->name);
			fpt->pos = NULL;
		}
		cur = cur->Next;
	} while (cur != Funcs);
	cur = StrGlobs;
	if (!cur)
		return;
	do {
		strings[cur->number] = cur->name;
		cur = cur->Next;
	} while (cur != StrGlobs);
}


void makeNum(Variable *a) {
	if (a->valid&IVal)
		return;
	a->val = strtol(a->name, NULL, 0);
	a->valid |= IVal;
	return;
}

void makeSure(Variable *a) {
	if (a->valid & SVal)
		return;
	else {
		makeMin(a, 12);
		sprintf(a->name, "%ld", a->val);
		a->valid |= SVal;
	}
}

void makeMin(Variable *a, unsigned int len) {
	
	len++;
	if (len > a->strsize) {
		len += 32;
		a->name = realloc(a->name, len);
		a->strsize = len;
	}
}

unsigned int copystr(char *str, Variable *a) {
	unsigned int len = strlen(str);
	
	makeMin(a, len);
	strcpy(a->name, str);
	a->valid = SVal;
	return len+1;
}

void assign(Variable *a, Variable *b) {
	if (a->valid == SVal) {
		copystr(a->name, b);
	} else {
		b->val = a->val;
		b->valid = IVal;
	}
}

int execute(char *upto, Variable *bp) {
	int done = 0;
	unsigned int opcode;
	unsigned int len;
	Variable *this, *sp = bp;
	char *temp;
	Function *funp;
	
	while (!done) {
		opcode = *upto;

/*		printf("SP = %lx ", sp);
		disasm(upto); */
		
		upto++;
		switch(opcode) {
		case LOCL:
			sp += *upto;
			upto++;
			break;
		case LDA8:
			a.val = (long) *upto;
			a.valid = IVal;
			upto++;
			break;
		case LDA16:
			a.val = (long) * (int *) upto;
			a.valid = IVal;
			upto += 2;
			break;
		case LDA32:
			a.val = * (long *) upto;
			a.valid = IVal;
			upto += 4;
			break;
		case LDAS:
			assign(bp + *upto, &a);
			upto++;
			break;
		case LDAT:
			copystr(strings[* (int *)upto], &a);
			upto += 2;
			break;
		case CAT:
			sp--;
			makeSure(sp);
			makeSure(&a);
			makeMin(&a, strlen(a.name) + strlen(sp->name));
			temp = strdup(a.name);
			strcpy(a.name, sp->name);
			strcat(a.name, temp);
			free(temp);
			a.valid = SVal;
			break;
		case ADD:
			sp--;
			makeNum(&a);
			makeNum(sp);
			a.val = sp->val + a.val;
			a.valid = IVal;
			break;
		case SUB:
			sp--;
			makeNum(&a);
			makeNum(sp);
			a.val = sp->val - a.val;
			a.valid = IVal;
			break;
		case MUL:
			sp--;
			makeNum(&a);
			makeNum(sp);
			a.val = sp->val * a.val;
			a.valid = IVal;
			break;
		case DIV:
			sp--;
			makeNum(&a);
			makeNum(sp);
			a.val = sp->val / a.val;
			a.valid = IVal;
			break;
		case MOD:
			sp--;
			makeNum(&a);
			makeNum(sp);
			a.val = sp->val % a.val;
			a.valid = IVal;
			break;
		case CMPL:
			sp--;
			makeNum(&a);
			makeNum(sp);
			a.val = (sp->val < a.val);
			a.valid = IVal;
			break;
		case CMPLE:
			sp--;
			makeNum(&a);
			makeNum(sp);
			a.val = (sp->val <= a.val);
			a.valid = IVal;
			break;
		case CMPEQ:
			sp--;
			if (a.valid&IVal || sp->valid&IVal) {
				makeNum(&a);
				makeNum(sp);
				a.val = (sp->val == a.val);
			} else {
				makeSure(&a);
				makeSure(sp);
				a.val = !strcmp(a.name, sp->name);
			}
			a.valid = IVal;
			break;
		case CMPNE:
			sp--;
			if (a.valid&IVal || sp->valid&IVal) {
				makeNum(&a);
				makeNum(sp);
				a.val = (sp->val != a.val);
			} else {
				makeSure(&a);
				makeSure(sp);
				a.val = strcmp(a.name, sp->name);
				if (a.val)
					a.val = 1;
			}
			a.valid = IVal;
			break;
		case CMPG:
			sp--;
			makeNum(&a);
			makeNum(sp);
			a.val = (sp->val > a.val);
			a.valid = IVal;
			break;
		case CMPGE:
			sp--;
			makeNum(&a);
			makeNum(sp);
			a.val = (sp->val >= a.val);
			a.valid = IVal;
			break;
		case BRA:
			upto += (long) *(int *)upto;
			break;
		case BEQ:
			if (a.val)
				upto += (long) *(int *)upto;
			else
				upto+=2;
			break;
		case BNE:
			if (!a.val)
				upto += (long) *(int *)upto;
			else
				upto+=2;
			break;
		case PHA:
			assign(&a, sp);
			sp++;
			break;
		case STAS:
			assign(&a, bp + *upto);
			upto++;
			break;
		case JSR:
			funp = &runfunc[* (int *)upto];
			if (funp->pos)
				execute(funp->pos, sp);
			else funp->func(sp + *(upto+3), *(upto+3));
			upto+=2;
			break;
		case NOT:
			makeNum(&a);
			a.val = !a.val;
			a.valid = IVal;
			break;
		case RET:
			done = 1;
			break;
		default:
			printf("Unrecognised opcode\n");
		}
	}
}

int main(int argc, char *argv[]) {
	int ch;
	
	gargc = argc;
	gargv = argv;
	if (argc<2) {
		fprintf(stderr, "Usage: an script\n");
		exit(1);
	}
	fp = fopen(argv[1],"rb");
	if (!fp) {
		perror("an");
		exit(1);
	}
	if ((ch = fgetc(fp)) == '#') {
		do {
			ch = fgetc(fp);
		} while (ch != EOF && ch != '\n');
		linenum++;
	}
	MainBuf = newbuf();
	FuncBuf = newbuf();
	TempBuf = newbuf();
	fillbuf();
	gettok();
	while (t != EOI) {
		dofunc();
	}
	prepfuncs();
	
	execute(MainBuf->buf, &stack[0]);
	exit(1);
}
