#ifndef _systypes_
#define _systypes_

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;

typedef unsigned int uint16;
typedef int int16;

typedef unsigned long uint32;
typedef long int32;

typedef unsigned char uint8;
typedef char int8;

typedef unsigned char uchar;

typedef int mode_t;
typedef long fpos_t;
typedef long off_t;
typedef long size_t;
typedef int ssize_t;
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define _PROTOTYPE(a,b) a b

#endif
