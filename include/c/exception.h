#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <stdio.h>

struct _except {
	int Next;
	int Stack;
	int DP;
	int Type;
	void *Data;
	void *RetAdd;
};

extern int try(struct _except *);
extern void throw(int type, void *data);
extern void popex();

// Need to include wgsutil lib

extern void errexc(int type, void *data);
extern void printexc(int type, void *data, FILE *out);

#define Try \
{ \
	struct _except _ex_; \
	if (!try(&_ex_)) \
	{

#define Catch(a) \
		popex(); \
		a = 0; \
	} \
	else { \
		a = _ex_.Type; \
	} \
} \
if (a)

#define Catch2(a, b) \
		popex(); \
		a = 0; \
	} \
	else { \
		a = _ex_.Type; \
		b = _ex_.Data; \
	} \
} \
if (a)

enum {
	EX_OUTOFMEMORY=1,
	EX_NORESOURCE,
	EX_IOEXCEPTION,
	EX_FILENOTFOUND,
	EX_NULLPOINTER,
	EX_USER=0x1000
};

#endif
