
#ifndef _TERMIO_H
#define	_TERMIO_H

enum { 
IOCTL_Font=0x10,
IOCTL_ChBG,
IOCTL_ChFG,
IOCTL_ChBord,
IOCTL_ChCurs
};

#define FONTF_8x8Char 0

/* non-standard Terminal IO stuff */

struct termios {
	int flags;
	int MIN;
	int TIMEOUT;
	int cols;
	int rows;
	int x;
	int y;
	int Speed;
};

extern int gettio(int fd, struct termios *tio);
extern int settio(int fd, struct termios *tio);
extern int setfg(int fd, int pid);

#define TF_ICANON	1
#define TF_IGNCR	2
#define TF_ECHO		4
#define TF_ECHONL	8
#define TF_OPOST	16
#define TF_ISIG		32
#define TF_ICRLF	64

#endif /* _TERMIO_H */
