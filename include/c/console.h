
#ifndef _CONSOLE_H
#define	_CONSOLE_H

/*  cursor based console IO stuff */

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

#define LC_End		0
#define LC_Start	1
#define LC_Full		2

extern int con_init();
extern int con_nosig();
extern int con_end();
extern int con_gotoxy(int x,int y);
extern int con_setscroll(int top,int bottom);
extern int con_setfg(int fgcol);
extern int con_setbg(int bgcol);
extern int con_setfgbg(int fgcol,int bgcol);
extern int con_setattr(int attr);
#define con_update() fflush(stdout)
extern int con_clrline(int which);
extern int con_clrscr();
extern int con_getkey();

extern int con_xsize;
extern int con_ysize;


#endif /* _CONSOLE_H */
