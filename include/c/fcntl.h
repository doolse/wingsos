#ifndef _fnctl_h_
#define _fcntl_h_

extern int open(const char *fname,int flags,...);

enum {
O_APPEND	= 1,
O_NONBLOCK	= 2,
O_ACCMODES	= 15,
O_READ		= 0x10,
O_RDONLY	= 0x10,
O_WRITE		= 0x20,
O_WRONLY	= 0x20,
O_RDWR		= 0x30,
O_EXEC		= 0x40,
O_DIR		= 0x80,
O_PROC		= 0x100,
O_STAT		= 0x200,
O_MOUNT		= 0x400,
O_CREAT		= 0x800,
O_EXCL		= 0x1000,
O_TRUNC		= 0x2000
};

#define fcntl(a, b, c)

#endif
