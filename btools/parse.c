#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "asm.h"

/* ---------------------------------------

		Parsing
		
--------------------------------------- */

/* Tokeniser */

uchar *parsed;
Inbuf *Bufstack[32];
uchar *MacParam[8];
uint BSUpto;

uchar *newbuf;
uint32 nrsize;
uint32 nsize;

uchar *buf;
uchar *bufup;
uchar *limit;
int buftype;
FILE *curfp;

uchar ident[MAXLABEL+1];
uchar nlab[MAXLABEL+1];

int t;
int mnemo;
int psuedo;
int32 gval;
uint scope;
int canred;
uint lastline;
char *structn;
char *sysdir=".";

Inbuf *first;
Inbuf *curfile;
Inbuf *lastfile;

Macro *MacTable[32];

#define NO_CHAR 0x2d

uint Asc2Pet(uint c)
{
	if(c==0x100-0x23)      return 0x5c;			//pound -> 5c
	else if(c <0x41)  return c;			//20 - 40 : same
	else if(c <0x5b)  return c+0x80;		//41 - 5a -> c0 - da
	else if(c==0x5b || c==0x5d)  return c;		//5b, 5d -> 5b, 5d
	else if(c==0x5c)  return 0x6d;			//backslash -> shift M
	else if(c==0x5e)  return c;			//^ -> arrow up
	else if(c==0x5f)  return 0xa4;			//_ -> C= @
	else if(c==0x60)  return NO_CHAR;		//` -> nothing
	else if(c <0x7b)  return c-0x20;		//61 - 7a -> 41 - 5a
	else if(c==0x7c)  return 0xdd;			//| -> C= -
	else		  return NO_CHAR;		//7b, 7d, 7e, 7f -> nothing
}


uint Asc2Scr(uint c)
{
	if(c==0x100-0x23)      return 0x1c;			//pound -> 1c
	else if(c <0x20)  return c;		//00 - 1f : nothing
	else if(c==0x40)  return 0;			//@ -> 00
	else if(c <0x5b)  return c;			//20 - 5a : same
	else if(c==0x5b || c==0x5d)  return c-0x40;	//5b, 5d -> 1b, 1d
	else if(c==0x5c)  return 0x4d;			//backslash -> shift M
	else if(c==0x5e)  return 0x1e;			//^ -> arrow up
	else if(c==0x5f)  return 0x64;			//_ -> C= @
	else if(c==0x60)  return NO_CHAR;		//` -> nothing
	else if(c <0x7b)  return c-0x60;		//61 - 7a -> 01 - 1a
	else if(c==0x7c)  return 0x5d;			//| -> C= -
	else		  return NO_CHAR;		//7b, 7d, 7e, 7f -> nothing
}

void opFile(char *str) {
	Inbuf *this=first;
	uint len;
	char *strs;
	
	while (this) {
		if (!strcmp(this->name, str))
			break;
		this = this->next;
	}
	if (this && !this->multi)
		return;
	if (!this) {
		this = first;
		while (this) {
			if (!this->fp && this->multi) {
				free(this->name);
				this->name = strdup(str);
				buf = this->bufp;
				break;
			}
			this = this->next;
		}
	}
	if (!this) {
		this = mymalloc(sizeof(Inbuf));
		this->next = first;
		first = this;
		this->name = strdup(str);
		this->multi = 1;
		this->bufp = buf = mymalloc(MAXLINE+1);
	}
	limit = bufup = buf;

	strs = strrchr(str, '/');
	if (!strs) {
		this->thisdir = ".";
	} else {
		len = strs-str;
		this->thisdir = mymalloc(len+1);
		strncpy(this->thisdir, str, len);
		this->thisdir[len] = 0;
	}

	curfp = fopen(str, "r");
	this->fp = curfp;
	if (!curfp)
		exerr("Unable to open file: %s", str);
	curfile = this;
	buftype = 0;
	linenum = 1;
	Bufstack[BSUpto++] = this;
}

void saveBuf() {
	Inbuf *inp = Bufstack[BSUpto-1];

	inp->type = buftype;
	inp->limp = limit;
	inp->bufp = buf;
	inp->bufupp = bufup;
	inp->fp = curfp;
	inp->linenum = linenum;
}

uchar *minbuf(uint size) {
	uchar *ret;
	if (nsize+size >= nrsize) {
		if (nrsize)
			nrsize *= 2;
		else nrsize = 4096;
		newbuf = realloc(newbuf, nrsize);
	}
	ret = newbuf+nsize;
	nsize += size;
	return ret;
}

void memBuf() {
	char *cp = minbuf(2);
	Inbuf *this;
	
	this = mymalloc(sizeof(Inbuf));
	
	cp[0] = -1;
	cp[1] = 0;
	buf = newbuf;
	bufup = buf;
	limit = cp+1;
	buftype = 1;
	Bufstack[BSUpto++] = this;
}

void fillbuf() {
	int bsize;
	uchar *s,*cp;
	
	if (buftype)
		return;
	s = buf;
	cp = bufup;
	while (cp < limit) {
		*s = *cp;
		s++;
		cp++;
	}
	if (feof(curfp)) 
		bsize = 0;
	else bsize = fread(s, 1, &buf[MAXLINE] - s, curfp);
	if (!bsize) {
		*s = 255;
		bsize = 1;
	} 
	limit = s + bsize;
	bufup = buf;
	*limit = 0;
}

uchar *chkbuf(uchar *cp) {
	if (!buftype && (limit - cp < MAXLABEL)) {
		bufup = cp;
		fillbuf();
		return bufup;
	}
	return cp;
}

int doscon(int exit, int skip) {
	uchar *cp = bufup;
	uchar *s = ident;
	uint cur,exit2=exit;
	if (exit == ' ')
		exit2 = '\t';
	if (skip) {
		cp = chkbuf(cp);
		while (map[*cp]&BLANK)
			cp++;
	}
	cp = chkbuf(cp);
	while (1) {
		cur = *cp;
		cp++;
		if (!cur) {
			cp = chkbuf(cp);
			cur = *cp;
			cp++;
		}
		if (cur == exit || cur == '\n' || cur == exit2) {
			if (skip) {
				while (s > ident) {
					s--;
					if (!(map[*s]&BLANK)) {
						s++;
						break;
					}
				}
			}
			*s = 0;
			if (cur == '\n')
				cp--;
			bufup = cp;
			return cur; 
		} else 
		if (cur == '\\') {
			cp = chkbuf(cp);
			cur = *cp;
			cp++;
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
	uchar *cp;
	uint slen;
	uint done, inquote;
	
	cp = bufup;
	while (1) {
		while (map[*cp]&BLANK)
			cp++;
		cp = chkbuf(cp);
		t = *cp;
		cp++;
		bufup = cp;
		if (map[t]&DIGIT) {
			gval = strtoul(cp-1, (char **)&cp, 10);
			bufup = cp;
			t = VALUE;
			return;
		}
		if (map[t]&LETTER) {
			t = LABEL;
			goto id;
		}
		switch(t) {
		case '.':
			t = PSUEDO;
			goto id2;
		case '/':
			if (cp[0] == '*') {
				inquote = done = 0;
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
							}
						}
						break;
					case 10:
						linenum++;
						break;
					case 0:
						cp = chkbuf(cp);
						continue;
					}
					cp++;
				}
				break;
			} else return;
		case '%':
			done = 0;
			gval = 0;
			while (cp[0] == '0' || cp[0] == '1') {
				gval <<= 1;
				gval |= cp[0] - '0';
				cp++;
				done = 1;
			}
			bufup = cp;
			if (done)
				t = VALUE;
			return;
		case '$':
			gval = strtoul(cp, (char **)&cp, 16);
			bufup = cp;
			t = VALUE;
			return;
		case '<':
			if (cp[0] == '=') {
				t = LEQ;
				goto retplus;
			} else if (cp[0] == '<') {
				t = SHL;
				goto retplus;
			}
			return;
		case '>':
			if (cp[0] == '=') {
				t = GEQ;
				goto retplus;
			} else if (cp[0] == '>') {
				t = SHR;
				goto retplus;
			}
			return;
		case '=':
			if (cp[0] == '=') {
				t = EQ;
				goto retplus;
			}
			return;
		case '!':
			if (cp[0] == '=') {
				t = NEQ;
				goto retplus;
			} 
			return;
		case '&':
			if (cp[0] == '&') {
				t = AND;
				goto retplus;
			}
			return;
		case '|':
			if (cp[0] == '|') {
				t = OR;
				goto retplus;
			}
			return;
			retplus:
			++bufup;
			return;
		case ' ': case '\t': case '\r': case 0:
			break;
		case ';':
			done=0;
			while(!done) {
				switch(*cp) {
				case '\n':
				case -1:
					done = 1;
					break;
				case 0:
					cp = chkbuf(cp);
				default:
					cp++;
					break;
				}
			}
			break;
		case '"': case '\'':
			doscon(t, 0);
			t = STRING;
			return;
		case 10:
			linenum++;
			return;
		case 255:
			if (!buftype) {
				Inbuf *this = curfile;

				fclose(curfp);
				if (!this->multi) {
					free(buf);
				}
				this->fp = NULL;
			} else {
				free(buf);
			}
			BSUpto--;
			if (!BSUpto) {
				t = EOI;
				return;
			} else {
				Inbuf *inp = Bufstack[BSUpto-1];
				
				buftype = inp->type;
				if (!buftype)
					curfile = inp;
				limit = inp->limp;
				bufup = cp = inp->bufupp;
				curfp = inp->fp;
				buf = inp->bufp;
				linenum = inp->linenum;
			}
			break;
		default:
			return;
		}
		
	}
	id:
	cp--;
	id2:
	slen = 0;
	while (map[*cp]&(DIGIT|LETTER)) {
		ident[slen] = *cp;
		cp++;
		slen++;
	}
	ident[slen] = 0;
	bufup = cp;
	if (t == LABEL && slen == 3) {
		mnemo = getmne(ident);
		if (mnemo != -1)
			t = MNEMO;
	} else
	if (t == PSUEDO) {
		if (!slen && *cp == '(') {
			psuedo = PBLOCK;
			bufup = cp+1;
		} else if (!slen && *cp == ')'){
			psuedo = PBEND;
			bufup = cp+1;
		} else psuedo = getpsu(ident);
	}
	return;
}

/* Get a hashcode for a label */

uint hashcode(uchar *str) {
	uint count=0;
	while (*str) {
		count += *str;
		str++;
	}
	return count&31;
}

/* Create/find label with scope, and maybe redefine */

Label *searchList(Label *cur, char *str) {
	while (cur) {
		if (!strcmp(cur->str, str))
			break;
		else cur = cur->next;
	}
	return cur;
}

Label *getlabel(int labscope, int define, char *str) {
	uint hash = hashcode(str);
	uint list;
	Label *cur,*head;
	uchar *out;

	if (define && labscope) {
		cur = searchList(LabTable[hash], str);
		if (cur && cur->exported)
			goto gotlab;
	}
	list = labscope*32 | hash;
	head = LabTable[list];
	cur = searchList(head, str);
	gotlab:
	if (cur && define && cur->hasdef && !(canred || cur->canredef)) {
		exerr("Redefining label '%s'", str);
	}
	if (!cur) {
		cur = (Label *) mymalloc(sizeof(Label));
		LabTable[list] = cur;
		cur->next = head;
		cur->prev = NULL;
		if (head)
			head->prev = cur;
		cur->str = strdup(str);
		cur->canredef = canred;
		cur->hasdef = define;
		cur->impnum = -1;
		cur->seg = 0;
		cur->value = 0;
		cur->exported = 0;
	} else {
		if (define) {
			cur->canredef = 1;
			cur->hasdef = 1;
		}
	}
	out = makemin(1+sizeof(Label *));
	out[0] = LABEL;
	*(Label **)(out+1) = cur;
	return cur;
}

void parexpr() {
	uchar *upto;
	Label *lab;
	int bracks=0;
	int labscope=scope;
	uint32 offs = outsize;
	
	if (t == '\n' || t == ':')
		exerr("Syntax error");
	makemin(1);
	while (1) {
		switch (t) {
		case VALUE:
			isval:
			upto = makemin(1+sizeof(int32));
			upto[0] = VALUE;
			* (int32 *)(upto+1) = gval;
			break;
		case LABEL:
			getlabel(labscope, 0, ident);
			labscope = scope;
			break;
		case STRING:
			if (!ident[1] && ident[0]) {
				uint ch = ident[0];
				if (deftrans == 1)
					ch = Asc2Pet(ch);
				else if (deftrans == 2)
					ch = Asc2Scr(ch);
				gval = ch;
				goto isval;
			} else {
				upto = makemin(1+sizeof(char *));
				upto[0] = STRING;
				* (char **)(upto+1) = strdup(ident);
			}
			break;
		case '?':
			if (labscope)
				labscope--;
			break;
		case ')':
			if (bracks) {
				bracks--;
				goto single;
			}
			goto expar;
		case '(':
			bracks++;
		case '^':case '+':case '-':case '*':case '&':case '|':
		case '/':case '!':case AND:case OR:case EQ:case NEQ:
		case '{':case '}':
		case GEQ:case LEQ:case '<':case '>':case SHL:case SHR:
			single:
			upto = makemin(1);
			*upto = t;
			break;
		default:	
			goto expar;
		}
		gettok();
	}
	expar:
	upto = makemin(1);
	*upto = EXEND;
	*(outbuf+offs) = outsize-offs;
	return;
}

void domnemo() {
	uint force=0;
	uint admode=ABS;
	uint ourmen = mnemo;
	int hasbr=0;
	int *curop = &optab[ourmen][0];
	int comma=0;
	uchar *upto;
	int32 val;
	uint32 offs = outsize;
	
	makemin(3);
	gettok();
	if (curop[ABS] == -1 && curop[ABSL] != -1)
		admode = ABSL;
	switch (t) {
		case ':':
		case '\n':
			if (curop[IMPL] != -1)
				admode = IMPL;
			else exerr("Syntax error - Requires operand");
			break;
		case '#':
			gettok();
			if (t == '!')
				gettok();
			admode = IMM;
			break;
		case '!':
			gettok();
			admode = ABS;
			force = 0x80;
			break;
		case '(':
			if (curop[ZPI] != -1 || curop[ZPIX] != -1 
			|| curop[ZPIY] != -1 || curop[ABSI] != -1 
			|| curop[ABSIX] != -1) {
				gettok();
				admode = ABSI;
				hasbr=1;
			}
			break;
		case '[':
			gettok();
			admode = ABSIL;
			break;
		case '@':
			gettok();
			admode = ABSL;
			force=0x80;
			break;
	}
	if (curop[admode] == -1 && curop[RELL] != -1)
		admode = RELL;
	else if (admode == ABS && curop[REL] != -1)
		admode = REL;				
	if (admode != IMPL) {
		parexpr();
	}
	if (t == ')') {
		gettok();
		hasbr=0;
	} else 
	if (t == ']') {
		gettok();
	}
	while (t == ',') {
		gettok();
		if (t == LABEL)
			comma = tolower(ident[0]);
		gettok();
		if (comma == 'x')
			admode = toxindex[admode];
		else if (comma == 'y')
			admode = toyindex[admode];
		else if (comma == 's')
			admode = tostack[admode];
		if (hasbr && t == ')') {
			gettok();
			hasbr=0;
		}
	}
	if (curop[admode] == -1) {
		force = 0x80;
		admode = tozp[admode];
		if (curop[admode] == -1) {
			exerr("Illegal addressmode");		
		}
	}
	if (hasbr && t == ')')
		gettok();
	upto = outbuf+offs;
	upto[0] = MNEMO;
	upto[1] = ourmen;
	upto[2] = admode | force;
}

void dosetabspc() {
	uchar *upto;

	upto = makemin(2);
	upto[0] = PSUEDO;
	upto[1] = SETABSPC;
	if (t != '\n') {
		parexpr();
	} else {
		upto = makemin(1);
		upto[0] = 0;
	}
}

void doseg(int seg) {
	uchar *upto;
	
	upto = makemin(3);
	upto[0] = PSUEDO;
	upto[1] = SEGMENT;
	cursegnum = seg;
	upto[2] = seg;
	curseg = &segbufs[seg];
	gettok();
	if (psuedo == PABS && t != '\n') {
		dosetabspc();
	} else if (t == VALUE) {
		curseg->startpc = gval;
		gettok();
	}
}

int parseline(int mustterm);

void dowhile() {
	uint32 offs,offs2;
	uchar *upto;
	
	offs2 = outsize;
	upto = makemin(2);
	upto[0] = PSUEDO;
	upto[1] = PIF;
	gettok();
	parexpr();
	offs = outsize;
	makemin(sizeof(uint32));
	lastline=0;
	while (1) {
		if (!parseline(0)) {
			if (t == PSUEDO) {
				if (psuedo == PWEND) {
					gettok();
					upto = makemin(1+sizeof(uint32));
					upto[0] = JUMP;
					* (uint32 *)(upto+1) = offs2;
					upto = outbuf+offs;
					* (uint32 *)upto = outsize;
					lastline=0;
					return;
				} else goto synerr;
			} else goto synerr;
		}
	}
	synerr:
	exerr("Syntax error in .while");
}

void dodo() {
	uint32 offs;
	uchar *upto;
	
	offs = outsize;
	lastline=0;
	while (1) {
		if (!parseline(0)) {
			if (t == PSUEDO) {
				if (psuedo == PUNTIL) {
					gettok();
					upto = makemin(2);
					upto[0] = PSUEDO;
					upto[1] = PIFN;
					parexpr();
					upto = makemin(sizeof(uint32));
					* (uint32 *)upto = offs;
					return;
				} else goto synerr;
			} else goto synerr;
		}
	}
	synerr:
	exerr("Syntax error in .do");
}

void doif() {
	uint32 offs;
	int hadelse=0;
	uchar *upto;
	
	upto = makemin(2);
	upto[0] = PSUEDO;
	upto[1] = PIF;
	gettok();
	parexpr();
	offs = outsize;
	makemin(sizeof(uint32));
	lastline=0;
	while (1) {
		if (!parseline(0)) {
			if (t == PSUEDO) {
				if (psuedo == PENDIF) {
					gettok();
					upto = outbuf+offs;
					* (uint32 *)upto = outsize;
					lastline=0;
					return;
				} else if (psuedo == PELSE) {
					if (!hadelse) {
						gettok();
						hadelse=1;
						upto = outbuf+offs;
						* (uint32 *)upto = outsize+1+sizeof(uint32);
						offs = outsize+1;
						upto = makemin(1+sizeof(uint32));
						upto[0] = JUMP;
						lastline=0;
					} else goto synerr;
				} else goto synerr;
			} else goto synerr;
		}
	}
	synerr:
	exerr("Syntax error in .if");
}

void dobytes() {
	uchar *upto;
	int did=0;
	
	upto = makemin(2);
	upto[0] = PSUEDO;
	upto[1] = psuedo;
	gettok();
	if (t == '!')
		gettok();
	if (t == '\n')
		exerr("Syntax error");
	while(1) {
		parexpr();
		if (t == '\n') 
			break;
		else if (t == ',') {
			gettok();
			continue;
		} else exerr("Syntax error");
	}
	upto = makemin(1);
	upto[0] = 0;
}

void domacout() {
	Macro *mac;
	char *in,*s;
	uint i,ch;
	
	ch = doscon(' ', 1);
	if (ident[0]) {	
		mac = MacTable[hashcode(ident)];
		while (mac) {
			if (!strcasecmp(ident, mac->str))
				break;
			mac = mac->next;
		}
		if (mac) {
			i=0;
			while (i<8 && ch != '\n') {
				ch = doscon(',', 1);
				if (ident[0])
					MacParam[i++] = strdup(ident);
			}
			nsize = 0;
			nrsize = 0;
			newbuf = NULL;
			in = mac->mbuf;
			while (ch = *in) {
				in++;
				if (ch == '\\') {
					ch = *in;
					in++;
					if (!ch)
						break;
					if (ch > '0' && ch < '8') {
						ch = ch - '1';
						if (ch >= i)
							exerr("Invalid macro param");
						s = minbuf(strlen(MacParam[ch]));
						strcpy(s, MacParam[ch]);
					} else {
						s = minbuf(2);
						*s = '\\';
						s[1] = ch;
					}
				} else {
					s = minbuf(1);
					*s = ch;
				}
			}
			saveBuf();
			memBuf();
			t = '\n';
		} else exerr("Undefined macro");
	}
}

void domac() {
	uint hash;
	Macro *mac,*head;
	uchar *cp,*s;
	int cur,quote=0,gotdot=0,slash=0;
	uint bsize;
	
	doscon(' ',1);
	if (ident[0]) {
		hash = hashcode(ident);
		mac = (Macro *)mymalloc(sizeof(Macro));
		head = MacTable[hash];
		mac->next = head;
		mac->prev = NULL;
		if (head)
			head->prev = mac;
		MacTable[hash] = mac;
		mac->str = strdup(ident);
		nsize = 0;
		nrsize = 0;
		newbuf = NULL;
		cp = bufup;
		while (1) {
			cp = chkbuf(cp);
			bsize = limit-cp;
			s = minbuf(bsize);
			if (gotdot) {
				if (!strncasecmp(cp, "mend", 4)) {
					s--;
					*s = 0;
					cp += 4;
					break;
				}
				gotdot = 0;
			}
			while (bsize) {
				bsize--;
				cur = *cp;
				cp++;
				if (cur == -1)
					exerr("Unexpected EOF");
				if (cur == '\n')
					linenum++;
				if (!quote) {
					if (cur == '\'' || cur == '"')
						quote = cur;
					else if (cur == '.') {
						gotdot = 1;
					}
				} else {
					if (!slash) {
						if (cur == quote)
							quote = 0;
						else if (cur == '\\')
							slash = 1;
					} else slash = 0;
				}
				*s = cur;
				s++;
				if (gotdot)
					break;
			}
			nsize = s-newbuf;
		}
		mac->mbuf = newbuf;
		bufup = cp;
	} else exerr("Syntax error");
	gettok();
}

void procopt(int opt, char *arg) {
	uint seg;
	
	switch(opt) {
	case 'v':
		verbose = 1;
		break;
	case 'o':
		outname = arg;
		nodfname = 1;
		break;
	case 'G':
		noglobs = 1;
		break;
	case 'e':
		c64exec = 1;
		break;
	case 'R':
		norel = 1;
		break;
	default:
		printf("Unknown option '%c'\n", opt);
		break;
	}
}

uint32 getaval(uint32 ret) {	
	if (t == ',') {
		gettok();
		if (t == VALUE) {
			ret = gval;
			gettok();
		}
	}
	return ret;
}

char *makenew(char *incdir) {
	char *newname;
	
	newname = mymalloc(strlen(incdir)+2+strlen(ident));
	strcpy(newname, incdir);
	strcat(newname, "/");
	strcat(newname, ident);
	return newname;
}

void dopsuedo() {
	uchar *upto;
	uint ch;
	Label *cur,*next,*head;
	
	switch (psuedo) {
		case -1:
			exerr("Illegal psuedo-op");
		case PINCLUDE:
			gettok();
			if (t == STRING) {
				upto = curfile->thisdir;
				goto gotdir;
			} else if (t == '<') {
				doscon('>', 0);
				upto = sysdir;
				gotdir:
				upto = makenew(upto);
				saveBuf();
				opFile(upto);
				free(upto);
			} else break;
			t = ':';
			break;
		case PNOMUL:
			curfile->multi=0;
			gettok();
			break;
		case PMAC:
			domac();
			break;
		case PEXPORT:
			gettok(); 
			while (t != '\n') {
				if (t == LABEL) {
					uint32 offs;
					Label *lab;
					
					offs = outsize;
					lab = getlabel(0, 0, ident);
					lab->exported = 1;
					outsize = offs;
				}
				gettok();
			}
			break;
		case PCODE:
		case PTEXT:
			doseg(SCODE);
			break;
		case PDATA:
			doseg(SDATA);
			break;
		case PBSS:
			doseg(SBSS);
			break;
		case PABS:
			doseg(SABS);
			break;
		case PFOPT:
			doseg(SFOPT);
			break;
		case PPIC:
			curseg->flags |= S_PIC;
			gettok();
			break;
		case PIF:
			doif();
			break;
		case PWHILE:
			dowhile();
			break;
		case PDO:
			dodo();
			break;
		case PUNTIL:
		case PWEND:
		case PELSE:
		case PENDIF:
			break;
		case PPSC:
		case PSCR:
		case PASC:
		case PBYTE:
		case PWORD:
		case P24:
		case PLONG:
			dobytes();
			break;
		case PLINK:
			do {
				char *ver;
				uint libver;
				
				ch = doscon(',', 1);
				if (ident[0]) {
					ver = strrchr(ident, ':');
					if (ver) {
						*ver = 0;
						libver = strtol(ver+1, NULL, 16);
					} else libver=0x100;
					upto = makemin(8);
					upto[0] = PSUEDO;
					upto[1] = PLINK;
					* (char **)(upto+2) = strdup(ident);
					* (uint16 *)(upto+6) = libver;
				}
			} while (ch == ',');
			t = '\n';
			break;			
		case PAS:
		case PAL:
		case PXS:
		case PXL:
			upto = makemin(2);
			upto[0] = PSUEDO;
			upto[1] = psuedo;
			gettok();
			break;
		case PBLOCK:
			scope++;
			gettok();
			break;
		case PBEND:
			if (scope) {
				uint i,t=32*scope;
				scope--;
				for (i=0;i<32;i++) {
					cur = LabTable[t+i];
					while (cur) {
						next = cur->next;
						if (!cur->hasdef) {
							Label *prv;
							head = LabTable[t-32+i];
							prv = searchList(head, cur->str);
							if (!prv) {
								LabTable[t-32+i] = cur;
								cur->next = head;
								cur->prev = NULL;
								if (head)
									head->prev = cur;
							} else {
								cur->next = prv;
								cur->seg = -1;
							}
						}
						cur = next;
					}
					LabTable[t+i] = NULL;
				}
			}
			else exerr("Too many closed blocks");
			gettok();
			break;
		case PDSB:
			upto = makemin(2);
			upto[0] = PSUEDO;
			upto[1] = PDSB;
			gettok();
			parexpr();
			if (t == ',') {
				gettok();
				parexpr();
			} else {
				upto = makemin(1);
				upto[0] = 0;
			}
			break;
		case POPT:
			do {
				ch = doscon(',', 1);
				if (ident[0]) {
					if (ident[1] | ident[2]) {
						upto = strdup(&ident[2]);
					} else upto = NULL;
					procopt(ident[0], upto);
				}
			} while (ch == ',');
			t = '\n';
			break;
		case PASS:
			t = doscon('\n', 1);
			upto = makemin(3);
			ch = ident[0]-'0';
			upto[0] = PSUEDO;
			upto[1] = PASS;
			upto[2] = ch;
			break;
		case PDFT:
			t = doscon('\n', 1);
			upto = makemin(3);
			ch = 0;
			if (ident[0] == 'p')
				ch = 1;
			else if (ident[0] == 's')
				ch = 2;				
			upto[0] = PSUEDO;
			upto[1] = PDFT;
			upto[2] = ch;
			deftrans = ch;
			break;
		case PSTACK:
			gettok();
			if (t == VALUE)
				stacksize = gval;
			else exerr("Expected value");
			gettok();
			break;
		case PSTRUCT:
			ch = doscon(',', 1);
			if (ident[0]) {
				structn = strdup(ident);
				upto = makemin(3);
				upto[0] = PSUEDO;
				upto[1] = SEGMENT;
				upto[2] = 1;
				if (ch == ',') {
					gettok();
					dosetabspc();
				} else {
					t = VALUE;
					gval = 0;
					dosetabspc();
				}
			} else exerr("Syntax error");
			break;
		case PSTEND:
			if (structn) {
				free(structn);
				structn = NULL;
			} else exerr("Syntax error");
			doseg(cursegnum);
			break;
		case PBIN:
			gettok();
			if (t == STRING) {
				FILE *fp;
				uint32 offs=0;
				uint16 size=1024;
				uchar *bbuf;
				char *newname;
				
				newname = makenew(curfile->thisdir);
				fp = fopen(newname, "r");
				if (fp) {
					struct stat sbuf;

					gettok();
					if (!fstat(fileno(fp), &sbuf)) {
						size = sbuf.st_size;
					}
					offs = getaval(0);
					size = getaval(size);
					bbuf = mymalloc(size);
					fseek(fp, offs, SEEK_SET);
					size = fread(bbuf, 1, size, fp);
					upto = makemin(2+sizeof(uint16)+sizeof(uint32));
					upto[0] = PSUEDO;
					upto[1] = PBIN;
					* (uint16 *) (upto+2) = size;
					* (uchar **) (upto+2+sizeof(uint16)) = bbuf;
					fclose(fp);
				} else exerr("Couldn't open file %s", newname);
				free(newname);
			}
			break;
		default:
			exerr("Unfinished psuedo op");
	}
	return;
}

int parseline(int mustterm) {
	int labscope=scope;
	uchar *upto;
	char *lname;
	
	if (curfile != lastfile) {
		upto = makemin(5);
		upto[0] = NFILE;
		* (Inbuf **) (upto+1) = curfile;
		lastfile = curfile;
	}
	if (!buftype && linenum != lastline) {
		if (linenum == lastline+1) {
			upto = makemin(1);
			upto[0] = 10;
		} else {
			upto = makemin(3);
			upto[0] = LINENO;
			* (uint16 *) (upto+1) = linenum;
		}
		lastline = linenum;
		
	}
	canred = 0;
	while (t == '&' || t == AND) {
		if (labscope)
			labscope--;
		if (t == AND && labscope)
			labscope--;
		gettok();
	}
	if (t == '-') {
		canred = 1;
		gettok();
	}
	if (t == '+') {
		gettok();
		labscope = 0;
	}
	if (t == '?') {
		doscon(' ', 1);
		t = LABEL;
	}
	if (t == LABEL) {
		if (structn) {
			lname = strcpy(nlab, structn);
			strcat(lname, ident);
		} else lname = ident;
		getlabel(labscope, 1, lname);
		canred = 0;
		gettok();
		if (t == '=') {
			gettok();
			parexpr();
			return;
		} else {
			upto = makemin(3);
			upto[0] = 3;
			upto[1] = '*';
			upto[2] = EXEND;
		}
	}
	if (t == '*') {
		gettok();
		if (t == '=') {
			gettok();
			dosetabspc();
		} else exerr("Syntax error");
	}
	switch (t)  {
		case MNEMO:
			domnemo();
			break;
		case PSUEDO:
			dopsuedo();
			break;
		case '!':
			domacout();
			break;
	}
	if (t == 10 || t == ':') {
		gettok();
		return 1;
	} else {
		if (mustterm)
			exerr("Syntax error found %d", t);
	}
	return 0;	
}

void parse() {
	uchar *upto;
	
	fillbuf();
	gettok();
	curseg = &segbufs[SCODE];
	cursegnum = SCODE;
	while (t != EOI) {
/*		printf("%d %s\n", linenum, curfile->name); */
		parseline(1);
	}
	if (scope)
		exerr("Blocks left open at end");
	upto = makemin(1);
	upto[0] = EOI;
}
