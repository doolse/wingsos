#ifndef _time_h_
#define _time_h_

#include <sys/types.h>

typedef long time_t;

extern time_t time(time_t *);
extern time_t sysup();
extern void settime(time_t);
extern void setstart(time_t);

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

extern char *tzname[2];

extern struct tm *gmtime(const time_t *timep);
extern struct tm *gmtime_r(const time_t *timep, struct tm *r);
extern struct tm *localtime(const time_t *timep);
extern struct tm *localtime_r(const time_t *timep, struct tm *r);
extern char *asctime_r(const struct tm *tp, char *str);
extern char *asctime(const struct tm *tp);
extern char *ctime(const time_t *timep);
extern ssize_t strftime(char *dst, ssize_t max, const char *fmt, const struct tm *tm);
extern time_t mktime(struct tm *timep);

#define clock() 0

#endif
