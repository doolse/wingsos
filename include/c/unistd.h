#ifndef _UNISTD_H_
#define _UNISTD_H_

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#include <sys/types.h>

#define sleep(a) 

extern int dup(int fd);
extern int read(int fd,void *buf, int size);
extern int write(int fd,const void *buf, int size);
extern off_t lseek(int fd, off_t offset, int whence);
extern void close(int);
extern int chdir(const char *);
extern int rmdir(const char *);

extern void _exit(int code);

extern int isatty(int fd);
extern int pipe(int fds[2]);

extern char *getcwd(char *buf, ssize_t size);
extern char *get_current_dir_name();

/* unilib routines */

char *getpass(char *);

#endif
