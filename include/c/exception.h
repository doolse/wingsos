#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

struct _except {
	int Next;
	int Stack;
	int DP;
	int Type;
	void *RetAdd;
};

extern int try(struct _except *);
extern void throw(int type);
extern void popex();

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

enum {
	EX_OUTOFMEMORY=1,
	EX_NORESOURCE,
	EX_IOEXCEPTION,
	EX_NULLPOINTER,
	EX_USER=0x8000
};

#endif
