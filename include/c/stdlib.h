
#ifndef _STDLIB_H
#define	_STDLIB_H

#include <sys/types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

extern char *getenv(const char *key);
extern int setenv(const char *key, const char *val, int overwrite);
extern int getopt(int argc, char **argv, char *opts);
extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;

extern void *malloc(long);
extern void *xmalloc(long);
extern void *balloc(long);
extern void *calloc(size_t s1, size_t s2);
extern void *realloc(void *,long);
extern void free(void *);

#define RAND_MAX 0x7fff

extern int rand();
extern void srand(unsigned int);

#define usleep(a)

extern long strtol(const char *str, char **end, int radix);
extern unsigned long strtoul(const char *str, char **end, int radix);

extern void exit(int code);

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#endif
