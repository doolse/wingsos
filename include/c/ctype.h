#ifndef _CTYPE_H_
#define _CTYPE_H_

extern unsigned char __ctype[];

#define _C	1
#define _U	2
#define _L	4
#define _X	8
#define _P	16
#define _S	32
#define _N	64

extern int tolower(int);
extern int toupper(int);

#define isspace(a) ((__ctype[(unsigned char)a])&_S)
#define isdigit(a) ((__ctype[(unsigned char)a])&(_N))
#define isxdigit(a) ((__ctype[(unsigned char)a]&(_N|_X))
#define isalpha(a) ((__ctype[(unsigned char)a])&(_U|_L))
#define isalnum(a) ((__ctype[(unsigned char)a])&(_U|_L|_N))
#define iscntrl(a) ((__ctype[(unsigned char)a])&_C)
#define isupper(a) ((__ctype[(unsigned char)a])&_U)
#define islower(a) ((__ctype[(unsigned char)a])&_L)
#define isgraph(a) ((__ctype[(unsigned char)a])&(_P|_U|_L|_N))
#define ispunct(a) ((__ctype[(unsigned char)a])&_P)
#define isprint(a) !((__ctype[(unsigned char)a])&_C)
#define isascii(a) ((unsigned)(a) < 128)

#endif
