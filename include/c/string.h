
#ifndef _STRING_H
#define	_STRING_H

#include <sys/types.h>

#define memmove(a,b,c) memcpy(a,b,c)

extern ssize_t strlen(const char*);
extern void *memset(void *to,int c, size_t size);
extern void *memcpy(void *to,const void *from, ssize_t size);

extern char *strcpy(char *to,const char *from);
extern char *strncpy(char *to,const char *from, ssize_t size);
extern char *strdup(const char *s);
extern char *strndup(const char *s, ssize_t size);
extern char *strcat(char *to, const char *from);
extern char *strncat(char *to, const char *from, ssize_t size);

extern int strcmp(const char *s1, const char *s2);
extern int strcasecmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, ssize_t size);
extern int strncasecmp(const char *s1, const char *s2, ssize_t size);
extern int memcmp(const void *s1, const void *s2, ssize_t size);

extern char *strchr(const char *string, int c);
extern char *strrchr(const char *string, int c);
extern char *strstr(const char *haystack, const char *needle);
extern char *strcasestr(char *haystack, char *needle);
extern char *strpbrk(const char *string, const char *stopset);
extern ssize_t strspn(const char *string, const char *skipset);
extern ssize_t strcspn(const char *string, const char *stopset);
extern char *strsep(char **string_ptr, const char *delimiter);
extern char *strerror(int errnum);

#define strnicmp(a,b,c) strncasecmp(a,b,c)

/* Unixlib functions */

extern char *strtok(char *s, const char *delim);

#endif /* _STRING_H */
