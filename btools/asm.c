#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include "getopt.h"
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include "asm.h"

int eval0(int32 *out);
uint Asc2Pet(uint c);
uint Asc2Scr(uint c);
void opFile(char *str);
void parse();

uchar o65head[] = {2, 8, 'J', 'o', 's', 0};

Label *LabTable[256];

/* Segments (including parsing buffer) */

/* 
Default segments 
0 - Undefined
1 - Absolute
2 - fopt
3 - Code
4 - Data
5 - Blank Data
*/

Segment segbufs[16];
Segment *curseg;
uint cursegnum;
uint numsegs=6;

uchar *outbuf;
uint32 realsize;
uint32 outsize;
uint32 curpc;
uint32 abspc;
int useabs;
int iscurbuf;

/* Expressions */

uchar *expr;
uchar *curexpr;

int assdbr;
int acc16;
int ind16;
int needpass;
int outputting;
int nodfname;
int deftrans;
uint relocmode;
uint base;
uint lbase;

uint impupto;
uint expupto;
uint uneval;
uint valchange;
uint stacksize;
uint linenum;
uint passup;
int verbose;
int noglobs;
int norel;
int c64exec;
Label *glab;
char *outname;
LLink *links;
uint linksize;
uint numlinks;
FILE *fp;

void prepSegments() {	
	segbufs[SUNDEF].flags = S_BLANK;
	segbufs[SABS].flags = S_BLANK;
	segbufs[SBSS].flags = S_BLANK|S_DBR|S_NOCROSS;
	segbufs[SDATA].flags = S_DBR|S_NOCROSS;
	segbufs[SCODE].flags = S_RO|S_NOCROSS;
	segbufs[SFOPT].flags = S_RO;
	segbufs[SCODE].startpc = 0x1000;
	segbufs[SDATA].startpc = 0x4000;
	segbufs[SBSS].startpc = 0x8000;
}

void exerr(char *str, ...) {
	va_list args;
	
	va_start(args, str);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, str, args);
	if (curfile)
		fprintf(stderr," in line %d of file \"%s\"\n", linenum, curfile->name);
	else fprintf(stderr,"\n");
	exit(1);
}

/* return value if ascii has no c64 equivalent */

void *mymalloc(uint32 size) {
	void *val = malloc(size);
	if (val)
		return val;
	fprintf(stderr,"Ran out of memory!\n");
	exit(1);
}

/* buffer sizing routines */

uchar *makemin(uint size) {
	uchar *ret;
	if (outsize+size >= realsize) {
		if (realsize)
			realsize <<= 1;
		else realsize = 512;
		outbuf = realloc(outbuf, realsize);
	}
	ret = outbuf+outsize;
	outsize += size;
	return ret;
}

void savesegbuf(Segment *cseg) {
	cseg->relbuf = outbuf;
	cseg->relrsize = realsize;
	cseg->relsize = outsize;
	iscurbuf = 0;
}

void newsegbuf(Segment *cseg) {
	outbuf = cseg->relbuf;
	realsize = cseg->relrsize;
	outsize = cseg->relsize;
	iscurbuf = 1;
}

/* Output bytes (with relocation entries */

void outbytes(uint size, int32 val, uint reloc, uint tbase) {
	int32 outval;
	uint extra;
	Segment *cseg = curseg;
	Segment *dseg;
	uchar *out;
	
	if (cseg->flags&S_BLANK)
		return;
	if (reloc) {
		if (reloc == RLONG && (size<4 || tbase))
			reloc = RSEGADR;
		if (reloc == RSEGADR && size<3)
			reloc = RWORD;
		if (reloc == RWORD && size == 1) {
			reloc = RLOW;
		}
		extra = 0;
		dseg = &segbufs[tbase];
		switch (reloc) {
			case RSOFFL:
				outval = 0;
				break;
			case RSOFFH:
				outval = 2;
				reloc = RSOFFL;
				break;
			case RSEG:
				outval = val >> 16;
				if (!(dseg->flags&S_NOCROSS))
					extra = 2;
				break;
			case RHIGH:
				outval = val >> 8;
				if (!(dseg->flags&S_PALIGN))
					extra = 1;
				break;
			default:
				outval = val;
				break;
		}
		if (tbase != 1) {
			uint dif = cseg->sbufups - cseg->lastrel;
		
			if (!iscurbuf)
				newsegbuf(cseg);
			while (dif > 254) {
				out = makemin(1);
				out[0] = 255;
				dif -= 254;
			}
			out = makemin(2);
			out[0] = dif;
			if (!tbase) {
				out[1] = reloc<<4;
				if (glab->impnum == -1) {
					savesegbuf(cseg);
					newsegbuf(&segbufs[0]);
					out = makemin(strlen(glab->str)+1);
					strcpy(out, glab->str);
					savesegbuf(&segbufs[0]);
					newsegbuf(cseg);
					glab->impnum = impupto;
					impupto++;
				}
				out = makemin(2);
				out[0] = glab->impnum&0xff;
				out[1] = glab->impnum>>8;
			} else out[1] = (reloc<<4)|(tbase-SCODE+1);
			if (extra) {
				out = makemin(extra);
				switch (extra)
				{
					case 4:
						out[3] = val>>24;
					case 3:
						out[2] = val>>16;
					case 2:
						out[1] = val>>8;
					case 1:
						out[0] = val&0xff;
				}				
			}
			cseg->lastrel = cseg->sbufups;
		}
	} else outval = val;
	out = cseg->sbufups;
	switch (size)
	{
		case 4:
			out[3] = outval>>24;
		case 3:
			out[2] = outval>>16;
		case 2:
			out[1] = outval>>8;
		case 1:
			out[0] = outval&0xff;
	}
	cseg->sbufups += size;
}

void addlink(char *name, uint ver) {
	LLink *lib=links;
	
	while (lib) {
		if (!strcmp(name, lib->name))
			break;
		lib = lib->next;
	}
	if (!lib) {
		linksize += strlen(name)+3;
		lib = malloc(sizeof(LLink));
		lib->name = strdup(name);
		lib->version = ver;
		lib->next = links;
		links = lib;
		numlinks++;
	} else {
		if (ver > lib->version)
			lib->version = ver;
	}
}

void showexpr(uchar *upto) {
	Label *lab;
	uint ch;
	char *s;
	
	upto++;
	while (1) {
		ch = *upto++;
		switch (ch) {
		case VALUE:
			printf("%ld", * (int32 *)upto);
			upto+=sizeof(int32);
			break;
		case LABEL:
			lab = * (Label **)upto;
			printf("%s(%ld,%d)", lab->str, lab->value, lab->seg);
			upto+=sizeof(Label *);
			break;
		case STRING:
			s = * (char **)upto;
			printf("%s", s);
			upto+=sizeof(char *);
			break;
		case EXEND:
			putchar('\n');
			return;
		default:	
			printf("(%c %d)", ch, ch);
			break;
		}
	}
}

int eval4(int32 *out) {
	int32 val=0;
	int tmpbase;
	int not=0;
	Label *lab;
	
	lbase = 1;
	if (*expr == '!') {
		expr++;
		not = 1;
	}
	switch(*expr) {
		case VALUE:
			expr++;
			val = * (int32 *)expr;
			expr += sizeof(int32);
			break;
		case '(':
			expr++;
			tmpbase = base;
			base = 1;
			if (eval0(&val)) {
				if (*expr != ')')
					exerr("Missing ')'");
				expr++;
				lbase = base;
				base = tmpbase;
			} else return 0;
			break;
		case LABEL:
			lab = * (Label **)(expr+1);
			while (lab->seg == -1) {
				lab = lab->next;
				* (Label **)(expr+1) = lab;
			}
			expr += 1+sizeof(Label *);
			val = lab->value;
			lbase = lab->seg;
			if (!lbase) {
				glab = lab;
				if (lab->hasdef)
					return 0;
			}
			break;
		case '*':
			expr++;
			if (useabs) {
				val = abspc;
				lbase = 1;
			} else {
				val = curpc;
				lbase = cursegnum;
			}
			break;
	}
	if (not) {
		val = !val;
		lbase = 1;
		base = 1;
	}
	if (!base && lbase != 1) {
		return 0;
	}
	if (base == 1)
		base = lbase;
	else if (lbase != 1 && lbase != base) {
		exerr("Illegal pointer arithmetic");
	}
	*out = val;
	return 1;
}

int eval3(int32 *out) {
	int32 val, val2;
	int tbase,tm;
	Label *lab;
	
	if (eval4(&val)) {
		while (1) {
			tbase = lbase;
			lab = glab;
			switch(*expr) {
			case '*':
			case '/':
			case '&':
			case '|':
			case SHL:
			case SHR:
			case '^':
				tm = *expr;
				expr++;
				if (eval4(&val2)) {
					if ((lbase > 1 || tbase > 1) || (!lbase && !glab->hasdef) || (!tbase && !lab->hasdef)) {
						exerr("Illegal pointer arithmetic");
					}
					if (!lbase && !tbase)
						return 0;
					switch(tm) {
					case '*':
						val *= val2;
						break;
					case '/':
						val /= val2;
						break;
					case '&':
						val &= val2;
						break;
					case '|':
						val |= val2;
						break;
					case '^':
						val ^= val2;
						break;
					case SHL:
						val <<= val2;
						break;
					case SHR:
						val >>= val2;
						break;
					}
				} else return 0;
				break;
			default:
				*out = val;
				return 1;
			}
		}
	} else return 0;
}

int eval2(int32 *out) {
	int32 val, val2;
	int tbase,tm;
	
	if (eval3(&val)) {
		while (1) {
			tbase = lbase;
			switch(*expr) {
			case '+':
				expr++;
				if (eval3(&val2)) {
					val = val + val2;
					if (tbase > 1 && lbase > 1)
						exerr("Illegal pointer arithmetic");
				} else return 0;
				break;
			case '-':
				expr++;
				if (eval3(&val2)) {
					val = val - val2;
					if (tbase > 1 && tbase == lbase)
						lbase = base = 1;
				} else return 0;
				break;
			default:
				*out = val;
				return 1;
			}
		} 
	} else {
		return 0;
	}
}

int eval1(int32 *out) {
	int32 val, val2;
	
	if (eval2(&val)) {
		while (1) {
			switch(*expr) {
			case LEQ:
				expr++;
				if (eval2(&val2)) {
					val = val <= val2;
					base = 1;
				} else return 0;
				break;
			case GEQ:
				expr++;
				if (eval2(&val2)) {
					val = val >= val2;
					base = 1;
				} else return 0;
				break;
			case '<':
				expr++;
				if (eval2(&val2)) {
					val = val < val2;
					base = 1;
				} else return 0;
				break;
			case '>':
				expr++;
				if (eval2(&val2)) {
					val = val > val2;
					base = 1;
				} else return 0;
				break;
			case NEQ:
				expr++;
				if (eval2(&val2)) {
					val = val != val2;
					base = 1;
				} else return 0;
				break;
			case EQ:
				expr++;
				if (eval2(&val2)) {
					val = val == val2;
					base = 1;
				} else return 0;
				break;
			default:
				*out = val;
				return 1;
			}
		}
	} else return 0;
}

int eval0(int32 *out) {
	int32 val, val2;
	
	if (eval1(&val)) {
		while (1) {
			switch(*expr) {
			case AND:
				expr++;
				if (eval1(&val2)) {
					val = val && val2;
					base = 1;
					if (!val) {
						*out = 0;
						return 1;
					}
				} else return 0;
				break;
			case OR:
				expr++;
				if (eval1(&val2)) {
					val = val || val2;
					base = 1;
					if (val) {
						*out = 1;
						return 1;
					}
				} else return 0;
				break;
			case EXEND:
			case ')':
				*out = val;
				return 1;
			default:
				exerr("Error in expression");
			}
		} 
	} else return 0;
}

int evalexpr(int32 *val, uchar **end) {
	int ret;
	
	base = 1;
	if (!**end)
		exerr("Empty evaluation");
	curexpr = expr = *end + 1;
	relocmode = RLONG;
	switch (*expr) {
		case '^':
			relocmode = RSEG;
			goto isrel;
		case '<':
			relocmode = RLOW;
			goto isrel;
		case '{':
			relocmode = RSOFFL;
			goto isrel;
		case '}':
			relocmode = RSOFFH;
			goto isrel;
		case '>':
			relocmode = RHIGH;
			isrel:
			expr++;
			break;
	}
	ret = eval0(val);
	if (!ret) {
		needpass = 1;
		uneval++;
		if (passup>10)
			showexpr(*end);
	}
	*end += **end;
	return ret;
}

/* --------------------------------------
		
		Extra Passes
		
--------------------------------------- */


void dopmne(uchar **up) {
	uchar *upto = *up;
	uint mnemo = upto[0];
	uint admode = upto[1];
	uint force = admode & 0x80;
	int *curop = &optab[mnemo][0];
	int32 val;
	uint opcode;
	uint len;
	
	admode &= 0x7f;
	*up = upto + 2;
	if (admode != IMPL) {
		uint lngmode = tolong[admode];
		len = evalexpr(&val, up);
		
		if (len && !force && base == 1) {
			if (val<256 && curop[tozp[admode]] != -1)
				admode = tozp[admode];
			else if (val>65535 && (curop[lngmode] != -1)) 
				admode = lngmode; 
		} else 
		if (len && base == 3 && assdbr && !(curop[admode]&0x800) && (admode == ABS || admode == ABSX) && curop[lngmode] != -1) {
			admode = lngmode;
		}
	}
	len = adlen[admode];
	if (admode == IMM) {
		if (acc16 && (curop[IMM]&0x400))
			len++;
		else if (ind16 && (curop[IMM]&0x800))
			len++;
	}
	if (outputting) {
		if (curop[admode] == -1)
			exerr("Illegal address mode %d", admode);
		opcode = curop[admode] & 0xff;
		outbytes(1, opcode, 0, 1);
		if (len > 1) {
			if (admode == REL || admode == RELL) {
				if (base != 1 && base != cursegnum) 
					exerr("Illegal arithmetic in branch");
				relocmode = 0;
				if (useabs)
					 val -= abspc;
				else val -= curpc;
				val -= len;
				if ((admode == REL && (val < -128 || val > 127)) || (val < -32768 || val > 32767)) 
					exerr("Branch out of reach");
			}
			outbytes(len-1, val, relocmode, base);
		}
	}
	curpc += len;
	if (useabs)
		abspc += len;
}

void doppsu(uchar **up) {
	uchar *upto = *up;
	uint perexpr,len=0;
	Segment *cseg = curseg;
	int32 val;
	uint newseg;
	int trans=deftrans;
	int tm=*upto;
	
	upto++;
	switch (tm) {
		case SEGMENT:
			newseg = *upto;
			upto++;
			useabs = 0;
			if (newseg != cursegnum) {
				cseg->curpc = curpc;
				if (iscurbuf)
					savesegbuf(cseg);
				cursegnum = newseg;
				curseg = &segbufs[newseg];
				curpc = curseg->curpc;
			}
			break;
		case SETABSPC:
			useabs = 1;
			if (*upto) {
				evalexpr(&val, &upto);
				abspc = val;
			} else {
				upto++;
				abspc = curpc;
			}
			break;
		case PASC:
			trans=0;
			goto bytes;
		case PPSC:
			trans=1;
			goto bytes;
		case PSCR:
			trans=2;
		case PBYTE:
			bytes:
			perexpr = 1;
			goto dobytes;
		case PWORD:
			perexpr = 2;
			goto dobytes;
		case P24:
			perexpr = 3;
			goto dobytes;
		case PLONG:
			perexpr = 4;
			goto dobytes;
		case PAS:
			acc16 = 0;
			break;
		case PAL:
			acc16 = 1;
			break;
		case PXS:
			ind16 = 0;
			break;
		case PXL:
			ind16 = 1;
			break;
		case PASS:
			assdbr = *upto;
			upto++;
			break;
		case PBIN:
			len = * (uint16 *) upto;
			if (outputting && !(cseg->flags&S_BLANK)) {
				uchar *bp = * (uchar **)(upto+2);
				memcpy(cseg->sbufups, bp, len);
				cseg->sbufups += len;
			}
			upto += 2+sizeof(uint32);
			break;
		case PLINK:
			addlink(*(char **)(upto), *(uint16 *)(upto+4));
			upto += 6;
			break;
		case PDSB:
			evalexpr(&val, &upto);
			len = val;
			val = 0;
			if (*upto) {
				evalexpr(&val, &upto);
			} else upto++;
			if (outputting && !(cseg->flags&S_BLANK)) {
				memset(cseg->sbufups, val, len); 
				cseg->sbufups += len;
			}
			break;
		case PIFN:
			if (evalexpr(&val, &upto) && !val) {
				upto+=4;
			} else {
				upto = parsed+(*(uint32 *)upto);
			}
			break;
		case PIF:
			if (evalexpr(&val, &upto) && val) {
				upto+=4;
			} else {
				upto = parsed+(*(uint32 *)upto);
			}
			break;
		case PDFT:
			deftrans = *upto;
			upto++;
			break;
		default:
			exerr("Corrupt Psuedo %d!", tm);
	}
	curpc += len;
	if (useabs)
		abspc += len;
	*up = upto;
	return;
		
	dobytes:
	while (*upto) {
		if (upto[0] == 7 && upto[1] == STRING) {
			uchar *str = *(uchar **)(upto+2);
			int ch;
			if (outputting) {
				len = 0;
				while (ch = *str) {
					if (trans == 1)
						ch = Asc2Pet(ch);
					else if (trans == 2)
						ch = Asc2Scr(ch);
					val = ch;
					outbytes(perexpr, val, 0, 1);
					str++;
					len += perexpr;
				}
			} else len = strlen(str) * perexpr;
			upto += 7;
		} else {
			len = perexpr;
			if (evalexpr(&val, &upto)) {
				if (outputting) {
					outbytes(len, val, relocmode, base);
				}
			}
		}
		curpc += len;
		if (useabs)
			abspc += len;
	}
	*up = upto+1;
	return;
}

void preppass() {
	uint i;
	Segment *cseg = &segbufs[0];
	
	for (i=0;i<numsegs;i++) {
		cseg->curpc = cseg->startpc;
		cseg++;
	}
	curpc = segbufs[SCODE].curpc;
	curseg = &segbufs[SCODE];
	cursegnum = SCODE;
	useabs = 0;
	acc16=0;
	ind16=0;
	deftrans=0;
	uneval=0;
	valchange=0;
	needpass = 0;
}

void dopass() {
	uchar *upto = parsed;
	int32 val;
	Label *lab;
	int tm;
	
	preppass();
	linenum = 1;
	while ((tm = *upto) != EOI) {
		upto++;
		switch (tm) {
			case LABEL:
				lab = * (Label **) upto;
				upto += sizeof(Label *);
				if (evalexpr(&val, &upto)) {
					if (!lab->seg) {
						lab->value = val;
						lab->seg = base;
					} else {
						if (val != lab->value || lab->seg != base) {
							lab->seg = base;
							lab->value = val;
							if (!lab->canredef) {
								valchange++;
								needpass = 1;
							}
						}
					}
				}
				break; 
			case MNEMO:
				dopmne(&upto);
				break;
			case PSUEDO:
				doppsu(&upto);
				break;
			case 10:
				linenum++;
				break;
			case JUMP:
				upto = parsed+(* (uint32 *)upto);
				break;
			case NFILE:
				curfile = * (Inbuf **)upto;
				upto += sizeof(Inbuf *);
				break;
			case LINENO:
				linenum = * (uint16 *)upto;
				upto += sizeof(uint16);
				break;
			default:
				exerr("Corrupt %d!", tm);				
		}
/*		printf("%d %d %lx\n", cursegnum, linenum, curpc); */
	}
	curseg->curpc = curpc;
	if (iscurbuf)
		savesegbuf(curseg);
}

uint getglobs() {
	uint i;
	uchar *out;
	Label *cur;
	uint numexp=0;
	
	newsegbuf(&segbufs[0]);
	for (i=0;i<32;i++) {
		cur = LabTable[i];
		while (cur) {
			if (cur->seg) {
				uint len = strlen(cur->str);
				uint ch = cur->seg;

				out = makemin(len+6);
				out = strcpy(out, cur->str)+len+1;
				if (ch == 1)
					ch=0;
				else ch = ch-SCODE+1;
				out[0] = ch;
				*(uint32 *)(out+1) = cur->value;
				numexp++;
			}
			cur = cur->next;
		}
	}
	savesegbuf(&segbufs[0]);
	return numexp;
}

void f16(uint16 val) {
	fputc(val&0xff, fp);
	fputc(val>>8, fp);
}

void f32(uint32 val) {
	fputc(val&0xff, fp);
	fputc(val>>8, fp);
	fputc(val>>16, fp);
	fputc(val>>24, fp);
}

void output() {
	uint i, numexp=0;
	uint32 blsize;
	Segment *cseg = &segbufs[0];

	for (i=0;i<numsegs;i++) {
		uchar *buf;
		
		cseg->size = cseg->curpc - cseg->startpc;
		if (!(cseg->flags&S_BLANK)) {
			buf = mymalloc(cseg->size);
			cseg->sbufups = cseg->bufptrs = buf;
			cseg->lastrel = buf-1;
		}
		cseg++;
	}
	preppass();	
	outputting = 1;
	dopass();
	fp = fopen(outname, "wb");
	if (fp) {
		
		if (c64exec)
			goto do64;
		/* Jos Magic */
		fwrite(o65head, 1, sizeof(o65head), fp);
		
		/* Flags */
		f16(0);	
		
		/* Version */
		f16(0x0100);	
		
		/* Stacksize */
		f16(stacksize);
		
		/* Write fopt's */
		if (segbufs[SFOPT].size) {
			fputc(INFO, fp);
			f32(segbufs[SFOPT].size);
			fwrite(segbufs[SFOPT].bufptrs, 1, segbufs[SFOPT].size, fp);
		}
		
		if (numlinks) {
			LLink *lib = links;
			fputc(LINKS, fp);
			f32(linksize);
			f16(numlinks);
			while (lib) {
				fputs(lib->name, fp);
				fputc(0, fp);
				f16(lib->version);
				lib = lib->next;
			}
		}

		/* Write imports */
		if (impupto) {
			fputc(IMPORT, fp);
			f32(segbufs[SUNDEF].relsize+2);
			f16(impupto);
			fwrite(segbufs[SUNDEF].relbuf, 1, segbufs[SUNDEF].relsize, fp);
		}
		
		/* Write segments */
		blsize = (numsegs-SCODE) * 14 + 2;
		cseg = &segbufs[SCODE];
		for (i=SCODE;i<numsegs;i++) {
			if (!(cseg->flags&S_BLANK)) {
				if (cseg->relsize && !norel)
					blsize += cseg->relsize+1;
				blsize += cseg->size;
			}
			cseg++;
		}
		fputc(SEGMENTS, fp);
		f32(blsize);
		f16(numsegs-SCODE);
		cseg = &segbufs[SCODE];
		for (i=SCODE;i<numsegs;i++) {
			f32(cseg->startpc);
			f32(cseg->size);
			if (norel)
				blsize = 0;
			else {
				if (blsize = cseg->relsize)
					blsize++;
			}
			f32(blsize);
			f16(cseg->flags);
			cseg++;
		}
		do64:
		cseg = &segbufs[SCODE];
		for (i=SCODE;i<numsegs;i++) {
			if (c64exec && i==SCODE)
				f16(cseg->startpc);
			if (!(cseg->flags&S_BLANK)) {
				fwrite(cseg->bufptrs, 1, cseg->size, fp);
				if (!c64exec && cseg->relsize && !norel) {
					fwrite(cseg->relbuf, 1, cseg->relsize, fp);
					fputc(0, fp);
				}
			}
			cseg++;
		} 
		
		/* Write globals */
		segbufs[SUNDEF].relsize = 0;
		if (!c64exec && !noglobs && (numexp = getglobs()) ) {
			fputc(EXPORT, fp);
			f32(segbufs[SUNDEF].relsize+2);
			f16(numexp);
			fwrite(segbufs[SUNDEF].relbuf, 1, segbufs[SUNDEF].relsize, fp);
		}
		if (!c64exec)
			fputc(ENDFILE, fp);
		fclose(fp);
	} else perror(outname);
}

void showins() {
	printf("ja - Jos Assembler\n"
	"Usage: ja [-v] [-o outname] [-R] [-G] [-e] source.a65\n"
	"-v           - Be verbose\n"
	"-o outname   - Output filename\n"
	"-G           - Suppress writing of globals\n"
	"-R           - Don't write reloc entries\n"
	"-e           - Produce C64 executable\n"
	"Default output name is \"source.o65\"\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	int ch;
	char *str;
	uchar *upto;
	uint len;
	char *strs,*stre;	
	
	inittarget();
	prepSegments();
	while ((ch = getopt(argc, argv, "o:RGev")) != EOF) 
		procopt(ch, optarg);
	if (optind == argc)
		showins();
	str = argv[optind];
	strs = strrchr(str, '/');
	if (!strs)
		strs = str;
	else strs++;
	if (!nodfname) {
		stre = strrchr(str, '.');
		if (!stre)
			stre = strrchr(str,0);
		len = stre-strs;
		outname = mymalloc(len+5);
		strncpy(outname, strs, len);
		strcpy(outname+len, ".o65");
	}
	opFile(str);
	parse();		
	parsed = outbuf;
	outputting = 0;
	passup=1;
	do {
		if (verbose)
			printf("------------\nPass %d\n------------\n", passup);
		dopass();
		passup++;
		if (verbose) {
			if (uneval) 
				printf("Unable to evaluate %d expressions\n", uneval);
			if (valchange) 
				printf("%d labels changed value\n", valchange);
		}
	} while (needpass && passup<=20);
	if (needpass)
		exerr("Unable to finish resolving");
	output();
        return 0;
}
