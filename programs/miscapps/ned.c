
/* ned.c 	A simple four-function text editor
 * 
 * ned is a simple text editor for those who can't be bothered with the
 * finickiness of vi and don't like waiting for emacs to drag its dripping
 * elephantine carcass into core just for the sake of making a quick edit
 * to a file. 
 *
 * ned was originally written for MS-DOS because I needed a simple editor 
 * that would work from a session CTTY'd down a COM port.  It can still
 * fulfill that role.
 *
 * Usage: ned filename [line]
 *
 * Simple commands:
 *	PF1, Ctrl/V		Function shift key (SHIFT below)
 *	Up, Down, Left, Right	Movement keys
 *	Ctrl/A			Beginning of line
 *	Ctrl/E			End of line
 *	PrevScreen, Ctrl/U	Up a screen
 *	NextScreen, Ctrl/N	Down a screen
 *	Ctrl/D			Delete character to right
 *	DEL, Ctrl/H		Delete character to left
 *	Ctrl/K			Delete to end of line
 *	Select, Ctrl/B		Mark start of selection
 *	Remove, Ctrl/W		Cut from mark to cursor
 *	InsertHere, Ctrl/Y	Paste cut text
 *	Ctrl/J			Justify line 
 *	Find, Ctrl/F		Find text
 *	Ctrl/G			Go to specific line number
 *	Ctrl/^			Insert control character
 *	Ctrl/L			Refresh screen
 *	Ctrl/C			Quit without saving
 *	Ctrl/X			Quit with save
 *
 * Shifted commands, key PF1 or Ctrl/V then command:
 *	PrevScreen, Ctrl/U	Go to top of file
 *	NextScreen, Ctrl/N	Go to bottom of file
 *	Find, Ctrl/F		Find text backwards
 *	i, I			Include file
 *	w, W			Save file under new name
 *	r, R			Replace found text with contents of cut buffer
 *
 * Above keys are DEC LK201/401 names; PC-101/104 equivalents are:
 *	PrevScreen	PageUp
 *	NextScreen	PageDn
 *	Find		Home
 *	Select		End
 *	Remove		Delete
 *	InsertHere	Insert
 *	DEL		Backspace
 *	PF1		NumLock (not on DOS version -- use Ctrl/V)
 *
 * Current version tested by me under NetBSD 1.*, Linux, MS-DOS (Turbo C V2.0).
 * Also seen working under Solaris 2.6 and HPUX 10.20.  Older version tested
 * with Digital Unix; should work with most Unices.
 *
 * Compiling under Unix:
 *	cc -o ned ned.c -lcurses -ltermcap
 *	or just sh ned.c
 *
 * Compiling under DOS (Turbo C v2.0; later versions should be similar -- 
 * switches are just to turn off some overly paranoid warnings, to use
 * the large memory model, and enable emulation of curses functions)
 *	tcc -DDOS -ml -w-pia -w-par ned
 *
 * Note: Under DOS requires ANSI.SYS or equivalent. 
 *
 * Author:
 *	Don Stokes
 *	Daedalus Consulting Services
 *	Email: don@daedalus.co.nz
 *
 * Modifications (since v0.7):
 *	8/12/98/dcs
 *		Added horizontal panning 
 *		Justify line fixed to work on last line of file, also
 *		move cursor to end of line.
 */

/* Copyright 1996, 1997, 1998 Don Stokes.  All rights reserved.
 *
 * Permission granted for individual use.  Unauthorised re-distribution 
 * prohibited.  (That is, please ask permission before placing in public 
 * archive or including in other non-commercial packages -- it will almost 
 * certainly be given.  Arrangements can be reached for commercial 
 * distribution.)
 *
 * No warranty of fitness expressed or implied.  No liability will be accepted
 * for loss or damage caused or contributed to by any use or misuse of this 
 * program.
 *
 * All copies, regardless of individual arrangements, must retain this notice.
 */


#include <stdio.h>
#define JOS  
/* Uncomment for MSDOS (or use -DDOS) */

#define VERSION "ned v0.8u"

#define MAXBUF 4096

unsigned char *buffer = 0;
unsigned char *cutbuffer = 0;
unsigned int cursor,scrtop,bufsize,cutbufsize,cutpoint,bufalloc;
char *showmessage = 0;
char scrbuf[256];
int row,col,actualcol,leftmargin;
int ccol;
int refstate;
unsigned int refpos;
int selactive;
char *filename;
int modified = 0;

#include <console.h>

#define TRACE(m)

#define REFEOL 1
#define REFEOS 2
#define REFSCR 4
#define REFSTA 8

#define UPARR 256
#define DOWN 257
#define LEFT 258
#define RIGHT 259
#define END 5		/* Ctrl/E */
#define HOME 1		/* Ctrl/A */
#define PGUP 21		/* Ctrl/U */
#define PGDOWN 14	/* Ctrl/N */
#define DEL 127		/* DEL */
#define BKSP 8		/* Backspace */
#define DELF 4		/* Ctrl/D */
#define DELEOL 11	/* Ctrl/K */
#define TAB 9		/* Tab */
#define CRET 13		/* CR */
#define LFEED 10	/* LF (bloody unix!) */
#define REFR 12		/* Ctrl/L */
#define EXIT 24		/* Ctrl/X */
#define SELECT 2	/* Ctrl/B */
#define CUT 23		/* Ctrl/W */
#define PASTE 25	/* Ctrl/Y */
#define FIND 6		/* Ctrl/F */
#define ABORT 3		/* Ctrl/C */
#define QUOTE 30	/* Ctrl/^ */
#define SHIFT 22	/* Ctrl/V */
#define GOTO 7		/* Ctrl/G */

/*
Hackery to emulate what little of curses this thing uses on a DOS box without
a curses library, and to ignore unixisms
*/
#ifdef JOS

#include <stdlib.h>
#include <string.h> 

#define clrtoeol() con_clrline(0)

void
standout(void) {
	printf("\033[7m");
}
void
standend(void) {
	printf("\033[m");
}

#define addstr(s) fputs(s, stdout)
#define addch(c) putchar(c)
#define touchwin(s)
#define move(y,x) con_gotoxy(x,y) 
#define noraw()
#define raw()
#define nonl()
#define nodraw()
#define nl()
#define echo()
#define noecho()
#define endwin()
#define refresh() con_update()
#define LINES con_ysize
#define COLS con_xsize
#define chmod(f,m)
#define chown(f,o,g)
#define lstat(f,b) (0)
#define signal(s,a)

void initscr(void) {con_init();con_nosig();}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
struct stat statbuf;
#include <curses.h>

#endif /* DOS */


unsigned int
findbol(unsigned int c) {
	TRACE("findbol")
	unsigned char *buf = buffer;
	
	while (c) {
		c--;
		if (buf[c] == '\n')  {
			c++;
			break;
		}
	}
	return c;
}

int
findpos(unsigned int pos) {
	unsigned int c;
	int r,rr,i;
	unsigned char *buf = buffer;

	TRACE("findpos")
	r = 0;
	if(scrtop > pos) {
		rr = 0;
		for(c = pos; c < scrtop; c++) if(buf[c] == '\n') rr--;
		scrtop = findbol(pos);
		row = 0;
		col = 0;
		for(c = scrtop; c < pos; c++) {
			col++;
			if(buf[c] == TAB) for(; (col % 8) != 0; col++);
			else if(buf[c] < 32 || buf[c] == 255 ||
				(buf[c] > 126 && buf[c] < 160)) col++;
		}
	} else {
		col = 0;
		rr = 0;
		for(c = scrtop; c < bufsize && c != pos; c++) {
			col++;
			if(buf[c] == '\n') {
				r++;
				col = 0;
			} else if(buf[c] == TAB) {
				for(; (col % 8) != 0; col++);
			} else if(buf[c] < ' ' || buf[c] == 255 ||
				  (buf[c] > 126 && buf[c] < 160)) col++;
		}
		row = r;
		if(r >= LINES-1) {
			c = scrtop;
			scrtop = findbol(pos);
			for(; c < bufsize && c != pos; c++) {
				if(buf[c] == '\n') {
					r--;
					if(r < LINES-1) {
						rr = r;
						scrtop = c + 1;
						break;
					}
				}
			}
			rr = r;
		}
	}
	actualcol = col;
	col -= leftmargin;
	if(col < 0) col = 0;
	if(col >= COLS) col = COLS-1;
	return rr;
}

void
setref(int state) {
	unsigned int c;
	unsigned char *buf = buffer;

	TRACE("setref")
	if(state == REFSCR || state == REFSTA) {
		refstate |= state;
		return;
	}
	if(state == REFEOS) {
		if((refstate & REFEOS) && cursor > refpos) return;
		refstate |= REFEOS;
		if((refstate & REFEOL) && cursor > refpos) return;
		refpos = cursor;
	} else if(state == REFEOL) {
		if(refstate & REFEOS) {
			if(cursor < refpos) refpos = cursor;
			return;
		}
		if(refstate & REFEOL) {
			if(cursor < refpos) {
				for(c = cursor; c < refpos; c++) {
					if(buf[c] == '\n') {
						state |= REFEOS;
						refpos = cursor;
						return;
					}
				}
			} else if(cursor > refpos) {
				for(c = refpos; c < cursor; c++) {
					if(buf[c] == '\n') {
						refstate |= REFEOS;
						return;
					}
				}
			}
		}
		refpos = cursor;
		refstate |= REFEOL;
	}
}

void
message(char *msg, int sts) {
	showmessage = msg;
	setref(REFSTA);
}


void
refrscr(void) {
	int rstate;
	unsigned int c,ch;
	int i,r, co, cos, oc;
	char outbuf[32];
	unsigned char *buf = buffer;

	TRACE("refrscr")
	if(refstate & REFSTA || showmessage) {
		move(LINES-1,0);
		standout();
		if(showmessage) if(!showmessage[0]) showmessage = 0;
		if(showmessage) {
			if(showmessage != scrbuf) {
				strncpy(scrbuf, showmessage, 255);
				scrbuf[255] = 0;
			}
			if((i = strlen(scrbuf)) < 39) {
				for(; i < 40; i++) scrbuf[i] = ' ';
				strncpy(&scrbuf[40], filename, 200);
			}
			showmessage = "";
		} else sprintf(scrbuf, "%-10s%-30s%s",
			VERSION, " (Ctrl/X to exit and save)", filename);
		for(i = strlen(scrbuf); i < 255 && i < COLS-1; i++)
			scrbuf[i] = ' ';
		if(i > COLS-1) i = COLS-1;
		scrbuf[i] = 0;
		addstr(scrbuf);
		standend();
	}

	rstate = refstate & (REFSCR|REFEOS|REFEOL);
	if(findpos(cursor)) rstate = REFSCR;
	if(leftmargin && actualcol < COLS && ccol < COLS) {
		for(c = cursor; c < bufsize && buf[c] != '\n'; c++) ;
		if((c - cursor) + actualcol < COLS) {
			leftmargin = 0;
			rstate = REFSCR;
		}
	}
	while(actualcol >= leftmargin + COLS) {
		rstate = REFSCR;
		leftmargin += 8;
	}
	while(actualcol < leftmargin) {
		rstate = REFSCR;
		leftmargin -= 8;
	}
	col = actualcol - leftmargin;

	if(!rstate) {
		findpos(cursor);
		return;
	}
	if(rstate & REFSCR) refpos = scrtop;
	if(findpos(refpos)) {
		rstate = REFSCR;
		refpos = scrtop;
		row = col = actualcol = 0;
	}

	r = row;
	co = actualcol;
	move(row, col);

	for(c = refpos; r < LINES-1 && c < bufsize; c++) {
		cos = co;
		co++;
		ch = buf[c];
		if(ch == '\n') {
			if(cos < COLS+leftmargin) clrtoeol();
			if(rstate == REFEOL) {
				findpos(cursor);
				refstate = 0;
				return;
			}
			r++;
			move(r,0);
			co = 0;
			continue;
		}
		oc = 0;
		if (ch == TAB) {
			outbuf[0] = ' ';
			oc++;
			i = co % 8;
			if (i) {				
				for(; i < 8; co++) {
					outbuf[oc] = ' ';
					oc++;
					i++;
				}
			}
		} else if (ch < 32) {
			outbuf[0] = '^';
			outbuf[1] = ch + '@';
			goto did2;
		} else if (ch < 127) {
			goto did1;
		} else if (ch == 127) {
			outbuf[0] = '^';
			outbuf[1] = '?';
			goto did2;
		} else if (ch < 160) {
			outbuf[0] = '&';
			outbuf[1] = ch - 128 + '@';
			goto did2;
		} else if (ch == 255) {
			outbuf[0] = '&';
			outbuf[1] = '?';
			did2:
			oc = 2;
			co++;
		} else {
			did1:
			outbuf[0] = ch;
			oc++;
		}
		for(i = 0; i < oc; i++) 
			if(cos+i >= leftmargin && cos+i < leftmargin+COLS)
				addch(outbuf[i]);
	}
	if(c == bufsize && r < LINES-1) {
		standout();
		addstr("[eof]");
		standend();
	}
	if(r < LINES-1) clrtoeol();
	while(++r < LINES-1) {
		move(r, 0);
		clrtoeol();
	}
	findpos(cursor);
	refstate = 0;
}

void
redraw(int k) {
	TRACE("redraw")
	echo();
	noraw();
	nl();
	endwin();
	initscr();
	noecho();
	raw();
	nonl();
	setref(REFSCR);
	setref(REFSTA);
	refrscr();
	touchwin(stdscr);
	move(row,col);
	refresh();
}



unsigned char *
setbufsize(unsigned int newsize) {
	unsigned char *b;
	TRACE("setbufsize")
	if(!buffer) {
		buffer = (unsigned char *)malloc(MAXBUF);
		bufalloc = MAXBUF;
	}
	if(newsize >= bufalloc) {
		if(!(b = (unsigned char *)realloc(buffer, newsize+MAXBUF))) {
			message("Insufficient memory", 1);
			return 0;
		}
		buffer = b;
		bufalloc = newsize+MAXBUF;
	}
	bufsize = newsize;
	return buffer;
}

unsigned char *
setcutbufsize(unsigned int newsize) {
	unsigned char *b;
	TRACE("setcutbufsize")
	if(cutbuffer) free(cutbuffer);
	if(!(b = (unsigned char *) malloc(newsize))) {
		message("Insufficent memory", 1);
		return 0;
	}
	cutbuffer = b;
	cutbufsize = newsize;
	return cutbuffer;
}

unsigned int
findeol(unsigned int c) {
	TRACE("findeol")
	unsigned char *buf = buffer;
	for(; buf[c] != '\n' && c < bufsize; c++);
	return c;
}

int
left(void) {
	TRACE("left")
	if(!cursor) {
		message("At top of file",1);
		return 0;
	}
	cursor--;
	ccol = 0;
	return 1;
}

int
right(void) {
	TRACE("right")
	if(cursor == bufsize) {
		message("At bottom of file",1);
		return 0;
	}
	cursor++;
	ccol = 0;
	return 1;
}

unsigned int
newcol(unsigned int c) {
	unsigned int d;
	int i;
	unsigned char *buf = buffer;

	TRACE("newcol")
	d = findbol(cursor);
	if(!ccol) {
		for(ccol = 0; d < cursor; d++) {
			if(buf[d] == TAB) while(++ccol % 8);
			else if(buf[d] < ' ' || buf[d] == 127) ccol += 2;
			else ccol++;
		}
	}
	for(i = 0; i < ccol; c++) {
		if(buf[c] == '\n' || c == bufsize) return c;
		if(buf[c] == TAB) while(++i % 8);
		else if(buf[c] < ' ' || buf[c] == 255 ||
			(buf[c] >= 127 && buf[c] < 160)) i += 2;
		else		     i++;
	}
	if(i != ccol) c--;
	return c;
}

int
up(void) {
	unsigned int c;

	TRACE("up")
	c = findbol(cursor);
	if(!c) {
		message("At top of file",1);
		return 0;
	}
	cursor = newcol(findbol(c-1));
	return 1;
}

int
down(void) {
	unsigned int c;

	TRACE("down")
	c = findeol(cursor);
	if(c++ == bufsize) {
		message("At bottom of file",1);
		return 0;
	}
	cursor = newcol(c);
	return 1;
}

void
startselect(void) {
	TRACE("startselect")
	cutpoint = cursor;
	selactive = 1;
	message("Mark set",0);
}

void
insert(unsigned char *k, int l) {
	unsigned int c;
	unsigned char *buf;

	TRACE("insert")

	if(!setbufsize(bufsize+l)) return;
	buf = buffer;
	setref(REFEOL);
	for(c = bufsize; c >= cursor + l; c--) buf[c] = buf[c-l];
	if(cutpoint > cursor) cutpoint += l;
	for(c = 0; c < l; c++) {
		buf[cursor++] = k[c];
		if(k[c] == '\n') setref(REFEOS);
	}
	ccol = 0;
	modified = 1;
}

void
delete(unsigned int n) {
	unsigned int c;
	unsigned char *buf = buffer;

	TRACE("delete")
	setref(REFEOL);
	if(n > (bufsize - cursor)) n = bufsize - cursor;
	for(c = cursor; c<cursor+n; c++) if(buf[c] == '\n') setref(REFEOS);
	for(c = cursor; c+n < bufsize; c++) buf[c] = buf[c+n];
	bufsize -= n;
	if(cutpoint > cursor) cutpoint -= n;
	ccol = 0;
	modified = 1;
}

void
cut(void) {
	unsigned int c,d;
	unsigned char *buf = buffer;

	TRACE("cut")
	if(!selactive) {
		message("No mark set", 1);
		return;
	}
	if(cursor > cutpoint) {
		c = cutpoint;
		cutpoint = cursor;
		cursor = c;
	}
	if(cutpoint > bufsize) cutpoint = bufsize;
	if(!setcutbufsize(cutpoint-cursor)) return;
	for(c = cursor,d = 0; c < cutpoint;) cutbuffer[d++] = buf[c++];
	delete(cutpoint-cursor);
	cutpoint = cursor;
	selactive = 0;
	modified = 1;
}

void
deleol(void) {
	TRACE("deleol")
	if(buffer[cursor] == '\n') delete(1); 
	else {
		startselect();
		cursor = findeol(cursor);
		cut();
	}
}

void
paste(void) {
	unsigned int c,l;

	TRACE("paste")
	insert(cutbuffer, cutbufsize);
}


unsigned int
find(char *f) {
	unsigned int c;
	int l;

	TRACE("find")
	if(!(*f)) return cursor;
	l = strlen(f);

	for(c = cursor+1; c + l < bufsize; c++)
		if(!memcmp(f, &buffer[c], l)) return c; 

	message("Not found",1);
	return cursor;
}

unsigned int
findreverse(char *f) {
	unsigned int c;
	int l;

	TRACE("find")
	if(!(*f)) return cursor;
	l = strlen(f);

	if(cursor) for(c = cursor-1;;) {
		if(!memcmp(f, &buffer[c], l)) return c; 
		if(!c--) break;
	}

	message("Not found",1);
	return cursor;
}


int
ask(char *prompt, char *buf, int siz) {
	int r,c,k,first,i;

	TRACE("ask")
	r = 0;
	move(LINES-1, 0);
	standout();
	sprintf(scrbuf,"%s%s", prompt, buf);
	for(i = strlen(scrbuf); i < 255 && i < COLS-1; i++) scrbuf[i] = ' ';
	if(i > COLS-1) i = COLS-1;
	scrbuf[i] = 0;
	addstr(scrbuf);
	r = strlen(prompt);
	c = 0;
	first = 1;
	while(1) {
		move(LINES-1,r);
		refresh();
		k = getch();
		switch(k) {
		case FIND:
		case CRET:
		case LFEED:
			if(!first) buf[c] = 0;
			standend();
			setref(REFSTA);
			return c;
		case BKSP:
		case DEL:
			if(c) {
				c--; r--;
				move(LINES-1,r);
				addch(' ');
			}
			break;
		default:
			if(k >= ' ' && k < DEL && c < siz) {
				addch(k);
				if(first) {
					for(i = strlen(buf)-1; i > 0; i--) 
						addch(' ');
					first = 0;
				}
				buf[c++] = k;
				r++;
			}
		}
	}
}


int
getkey(void) {
	int k;

	TRACE("getkey")
	refrscr();
	move(row,col);
	refresh();
	k = getch();
	if(k == 0) {
		k = getch();
		switch(k) {
		case 'H': k = UPARR; break;
		case 'P': k = DOWN; break;
		case 'M': k = RIGHT; break;
		case 'K': k = LEFT; break;
		case 'I': k = PGUP; break;
		case 'Q': k = PGDOWN; break;
		case 'O': k = END; break;
		case 'G': k = HOME; break;
		case 'S': k = CUT; break;
		case 'R': k = PASTE; break;
		default:  k = 0;
		}
		return k;
	}
	if(k == 27) {
		k = getch();
		if(k == '[' || k == 'O') k = getch();
		if(k >= '1' && k <= '9') {
			int j;
			j = k - '0';
			k = getch();
			if(k >= '0' && k <= '9') {
				j = j * 10 + (k - '0');
				k = getch();
			}
			if(k == '~') switch(j) {
			case 1: k = FIND; break;
			case 2: k = PASTE; break;
			case 3: k = CUT; break;
			case 4: k = SELECT; break;
			case 5: k = PGUP; break;
			case 6: k = PGDOWN; break;
			default:  k = 0;
			}
		} else switch(k) {
		case 'A': k = UPARR; break;
		case 'B': k = DOWN; break;
		case 'C': k = RIGHT; break;
		case 'D': k = LEFT; break;
		case 'P': k = SHIFT; break;
		case 'Q': k = '?'; break;
		case 'R': k = FIND; break;
		case 'S': k = DELEOL; break;
		default:  k = 0;
		}
		return k;
	}
	return k;
}

void
justify(void) {
	unsigned c, i;
	unsigned char *buf = buffer;

	cursor = findeol(cursor);
	if(cursor >= bufsize) return;
	for(cursor--; cursor > 0 && (buf[cursor] == ' ' || 
				     buf[cursor]=='\t'); cursor--) delete(1);
	cursor++;
	c = findbol(cursor);
	if(c == cursor) {
		if(cursor < bufsize) cursor++;
		return;
	}
	ccol = 0;
	newcol(cursor);
	if(ccol < 72) {
		if(cursor >= bufsize) return;
		if(buf[cursor+1] == '\n') {
			cursor++;
			return;
		}
		buf[cursor++] = ' ';
		setref(REFSCR);
		for(i = 0; buf[cursor+i] == ' ' || buf[cursor+i] == '\t';
		    i++);
		if(i) delete(i);
		ccol = 0;
		cursor = findeol(cursor);
		newcol(cursor);
		modified = 1;
	}

	if(ccol > 72) {
		ccol = 72;
		cursor = newcol(findbol(cursor));
		for(;; cursor--) {
			if(!cursor || buf[cursor] == '\n') {
				down();
				break; 
			}
			if(buf[cursor] != ' ' && buf[cursor] != '\t') 
				continue;
			for(i = 0; cursor && (buf[cursor] == ' ' || 
					 buf[cursor] == '\t'); cursor--) i++;
			cursor++;
			if(i) delete(i);
			insert("\n", 1);
			modified = 1;
			break;
		}
	} else cursor++;

	for(i = 0; buf[cursor+i] == ' ' || buf[cursor+i] == '\t'; i++);
	if(i) delete(i);

	if(cursor >= bufsize || buf[cursor] == '\n') return;

	c = findbol(cursor - 1);
	for(i = 0; buf[c+i] == ' ' || buf[c+i] == '\t'; i++); 
	insert(&buf[c], i);

	cursor = findeol(cursor);
}

int
insertfile(char *filename) {
	int i, k;
	FILE *f;
	static char buf[1024];

	i = 0;
	if(f = fopen(filename, "r")) {
		while((k = getc(f)) != EOF) if(k) {
#ifdef DOS
			if(k == '\r' || k == 26) continue;
#endif
			buf[i++] = k;
			if(i == 1024) {
				insert(buf, 1024);
				i = 0;
			}
		}
		if(i) insert(buf, i);
		fclose(f);
	} else {
		sprintf(scrbuf, "ERROR: could not read %s", filename);
		message(scrbuf,0);
		return 1;
	}
	return 0;
}



int
writefile(char *filename) {
	FILE *f, *fb;
	static char backupfile[255];
	char *dot, *c;
	int notnew, bytes;
	unsigned int i;

	notnew = 1;
	if((f = fopen(filename, "r")) && notnew) {
		fclose(f);

		strncpy(backupfile, filename, 250);
		backupfile[250] = 0;
#ifdef DOS
		dot = 0;
		for(c = backupfile; *c; c++) {
			if(*c == '.') dot = c;
			if(*c == '/' || *c == '\\') dot = 0;
		}
		if(!dot) dot = c;
#else
		dot = strchr(backupfile, 0);
#endif
		strcpy(dot, ".bak");

		unlink(backupfile);
		if(strcmp(filename, backupfile)) {
			if(lstat(filename, &statbuf) ||
			   rename(filename, backupfile)) {
				move(LINES-1, 0);
				addstr("ERROR: could not make backup ");
				addstr(backupfile);
				return 0;
			}
		} else notnew = 0; 
	} else notnew = 0;

	if(!(f = fopen(filename, "w"))) {
		move(LINES-1, 0);
		sprintf(scrbuf, "ERROR: could not write %s", filename);
		message(scrbuf,0);
		return 0;
	}
	bytes = 0;
	for(i = 0; i < bufsize; i++) {
#ifdef DOS
		if(buffer[i] == '\n') bytes++;
#endif
		fputc(buffer[i], f);
		bytes++;
	}
	fclose(f);

	if(notnew) {
		chmod(filename, statbuf.st_mode);
		chown(filename, statbuf.st_uid, statbuf.st_gid);
	}

	move(LINES-1,0);
	clrtoeol();
	sprintf(scrbuf,"%s %d bytes", filename, bytes);
	message(scrbuf, 0);
	modified = 0;
	return 1;
}

void
abortedit(int i) {
	char ynbuf[4];

	strcpy(ynbuf, "YES");
	if(modified) {
		ask("Really quit? ", ynbuf, 3);
		if(*ynbuf == 'n' || *ynbuf == 'N') return;
	}
	refresh();
        noraw();
	nl();
        echo();
        endwin();
	putchar('\n');
	exit(i);
}


main(int argc, char **argv) {

	FILE *f;
	int k, i, j;
	char ch;
	char findbuffer[32];
	char linbuf[16];
	char filenambuf[80];

	if(argc != 2 && argc != 3) {
		puts(VERSION);
		puts("Usage: ned <filename> [<line>]");
		return 1;
	}
	filename = argv[1];

	buffer = 0;
	setbufsize(0);
	if(f = fopen(filename, "r")) {
		while((k = getc(f)) != EOF) { //if(k) {
#ifdef DOS
			if(k == '\r' || k == 26) continue;
#endif
			buffer[bufsize] = k;
			if(!setbufsize(bufsize+1)) {
				puts("File too large");
				return 1;
			}
		}
		fclose(f);
	}

	initscr();
	raw();
	nonl();
	noecho();

	signal(SIGWINCH, (void *)redraw);

	leftmargin = 0;
	ccol = 0;
	cutbuffer = 0;
	cutpoint = 0; selactive = 0;
	scrtop = cursor = 0;
	refstate = REFSCR|REFSTA;
	findbuffer[0] = 0;
	modified = 0;

	if(argc == 3) {
		k = atoi(argv[2]);
		for(i = 1; i < k; i++) down();
	}

	for(;;) {
		k = getkey();
		switch(k) {
		case LEFT:
			left();
			break;
		case UPARR:
			up();
			break;
		case DOWN:
			down();
			break;
		case RIGHT:
			right();
			break;
		case PGUP:
			for(i = 0; i < LINES-1; i++) up();
			break;
		case PGDOWN:
			for(i = 0; i < LINES-1; i++) down();
			break;
		case HOME:
			cursor = findbol(cursor);
			ccol = 0;
			break;
		case END:
			cursor = findeol(cursor);
			ccol = 0;
			break;
		case FIND:
			ask("Find: ", findbuffer, 31);
			cursor = find(findbuffer);
			ccol = 0;
			break;
		case GOTO:
			*linbuf = 0;
			ask("Goto: ", linbuf, 15);
			if(j = atoi(linbuf)) {
				cursor = 0;
				for(i = 1; i < j; i++) down();
			}
			break;
		case SELECT:
			startselect();
			break;
		case CUT:
			cut();
			break;
		case PASTE:
			paste();
			break;
		case DELF:
			delete(1);
			break;
		case DELEOL:
			deleol();
			break;
		case BKSP:
		case DEL:
			if(left()) delete(1);
			break;
		case LFEED:
			justify();
			break;
		case CRET:
			insert("\n", 1);
			break;
		case EXIT:
			if (modified)
				writefile(filename);
			if (!modified) {
				addstr(scrbuf);
				abortedit(0);
			}
			break;
		case ABORT:
			move(LINES-1,0);
			abortedit(0);
			break;
		case REFR:
			redraw(0);
			break;
		case QUOTE:
			k = getkey();
			if(k >= 0 && k < 256) {
				ch = k;
				insert(&ch, 1);
			}
			break;
		case SHIFT:
			switch(getkey()) {
			case PGUP:
				cursor = 0;
				break;
			case PGDOWN:
				cursor = bufsize;
				break;
			case FIND:
				ask("Reverse find: ", findbuffer, 31);
				cursor = findreverse(findbuffer);
				ccol = 0;
				break;
			case 'i':
			case 'I':
				*filenambuf = 0;
				ask("Insert: ", filenambuf, 71);
				insertfile(filenambuf);
				break;
			case 'W':
			case 'w':
				*filenambuf = 0;
				ask("Write to: ", filenambuf, 68);
				if(*filenambuf) writefile(filenambuf);
				break;
			case 'r':
			case 'R':
				delete(strlen(findbuffer));
				paste();
				cursor = find(findbuffer);
				break;
			}
			break;
		default:
			if((k >= ' ' && k < DEL) ||
			   (k >= 160 && k < 255) || k == TAB) {
				ch = k;
				insert(&ch,1);
			}
		}
	}
}
