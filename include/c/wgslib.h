#ifndef _SYSLIB_H_
#define _SYSLIB_H_

#include <sys/types.h>

struct wmutex {
   int a;
   int b;
};

struct PSInfo {
	char Name[18];
	pid_t PID;
	pid_t PPID;
	int priority;
	long Time;
	long Mem;
	long Shared;
};

extern char *wgswd();
extern int setFlags(int fd, int flags);

extern char *fullpath(const char *fname);
extern char *fpathname(const char *fname, const char *dir, int type);

extern int mount(char *,char *,char *);
extern int umount(const char *);

/* Memory allocation */

#define S_WAIT		1
#define S_LEADER	4

extern pid_t getPPID();
extern pid_t spawnv(int, char **argv);
extern pid_t spawnvp(int, char **argv);
extern pid_t spawnl(int, ...);
extern pid_t spawnlp(int, ...);
extern pid_t jspawn(int, char **argv, char **envp, char *redir);
extern void waitPID(int);
extern int redir(int from ,int to);
extern void resetredir();

extern void setvpgid(int);

extern long _debug();

extern pid_t getPSInfo(pid_t, struct PSInfo *);

extern char *getappdir();

extern char *queryname(char *name, int flag);
extern int getpty(int fds[2]);
extern int noinh(int);
extern char __redirtab[32];

#define STACK_DFL 512

extern int newThread(void (),int,void *);

extern void getMutex(struct wmutex *);
extern void relMutex(struct wmutex *);

#define T_Release	0
#define T_Alarm		1

extern unsigned long getTimer();
extern int setTimer(int timer, unsigned long len, ...);

extern void retexit(int);
extern int syncfs(char *);

extern void *addQueue(void *head, void *insp, void *item);
extern void *remQueue(void *head, void *item);
extern void *addQueueB(void *head, void *insp, void *item);

#endif
