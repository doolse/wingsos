#ifndef _SIGNAL_H
#define	_SIGNAL_H

#include <sys/types.h>

extern int kill(pid_t pid, int sig);

#define signal(a,b)

#define SIG_DFL ((void (*)(int)) 0)
#define SIG_ERR ((void (*)(int)) 1)
#define SIG_HOLD ((void (*)(int)) 2)
#define SIG_IGN ((void (*)(int)) 3)

#define SIGABRT 1
#define SIGTERM 2
#define SIGALRM 3
#define SIGCONT 4
#define SIGPIPE 5
#define SIGQUIT 6
#define SIGINT  7

#endif /* _SIGNAL_H */
