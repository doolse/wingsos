
#ifndef _ERRNO_H
#define	_ERRNO_H

#define errno (*(int *)0x010005)

#define ENOMEM		1
#define ENOENT		2
#define EBADBIN		3
#define ECONREF		4
#define EDNSFAIL	5
#define ETIMEOUT	6
#define ENOSPC		7
#define EEXIST		8
#define ESRCH		9
#define EIO		10
#define EACCESS		11
#define ENOTDIR		12
#define EISDIR		13
#define EAGAIN		14
#define EBADF		15
#define ECHANCLO	16
#define ENOTEMP		17
#define EDIFSYS		18
#define EMFILES		19
#define ECONRES		20
#define ERANGE		21
#define EDOAGAIN	22
#define E2BIG		23
#define EINTR		24
#define EPIPE		25


#endif /* _ERRNO_H */
