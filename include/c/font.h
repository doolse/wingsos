
#ifndef _FONT_H
#define	_FONT_H

/* Routines for the font library */

struct fontret {
	int x;
};

#define FNTS_Bold	1
#define FNTS_Underline	2
#define FNTS_Reverse	4
#define FNTS_Outline	8
#define FNTS_Italic	16
#define FNTS_Plain	0

extern void FL_prepSize(int font, int style);
extern void FL_drawText(char *Outbuf, char *mask, int bufsize, char *string, int font, int style, int x, int y, struct fontret *ret);
extern int FL_extStrX(char *string, int font, int style);
extern int FL_loadFont(char *string, int pntsize);

#endif /* _FONT_H */
