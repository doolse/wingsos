#ifndef _sys_time_h_
#define _sys_time_h_

#include <time.h>

typedef struct {
	long fds_bits[1];
} fd_set;

struct timeval {
	long tv_sec;
	long tv_usec;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

int gettimeofday(struct timeval *tv, struct timezone *tz);
int settimeofday(const struct timeval *tv, const struct timezone *tz);

void FD_CLR(int fd, fd_set *fdset);
int  FD_ISSET(int fd, fd_set *fdset);
void FD_SET(int fd, fd_set *fdset);
void FD_ZERO(fd_set *fdset);
int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

#define FD_SETSIZE 32

#endif
