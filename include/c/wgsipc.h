
#ifndef _WGSIPC_H_
#define _WGSIPC_H_


#define FILEMSG	0
#define FSYSMSG	0x20
#define MEMMSG	0x40
#define DEVMSG	0x60
#define WINMSG	0xc0
#define PROCMSG	0x80

#define MEMM_CHAN	0

#define MMSG_Alloc	0+MEMMSG
#define MMSG_AllocBA	1+MEMMSG
#define MMSG_Free	2+MEMMSG
#define MMSG_Info	5+MEMMSG

enum {
IO_OPEN=1,
IO_CLOSE, 
IO_DIED,
IO_READ,
IO_WRITE,
IO_LSEEK,
IO_READB,
IO_WRITEB,
IO_TRUNC,
IO_FSTAT,
IO_CONTROL,
IO_CHDIR,
IO_MKDIR,
IO_REMOVE,
IO_RMDIR,
IO_RENAME,
IO_NFYREAD,
IO_NFYWRITE,
IO_ASKNOTIFY
};

#define FSYS_MOUNT	0+FSYSMSG
#define FSYS_UMOUNT	1+FSYSMSG
#define FSYS_READB256	2+FSYSMSG
#define FSYS_WRITEB256	3+FSYSMSG
#define FSYS_READB512	4+FSYSMSG
#define FSYS_WRITEB512	5+FSYSMSG
#define FSYS_PIPE	6+FSYSMSG
#define FSYS_GETPTY	7+FSYSMSG
#define FSYS_READTERM	8+FSYSMSG
#define FSYS_WRITETERM	9+FSYSMSG
#define FSYS_SYNC	10+FSYSMSG

#define DMSG_Add	DEVMSG+0

#define PMSG_Spawn	PROCMSG+0
#define PMSG_AddName	PROCMSG+1
#define PMSG_ParseFind	PROCMSG+2
#define PMSG_FindName	PROCMSG+3
#define PMSG_QueryName	PROCMSG+4
#define PMSG_Alarm	PROCMSG+5
#define PMSG_KillChan	PROCMSG+6
#define PMSG_WaitPID	PROCMSG+7
#define PMSG_Parse	PROCMSG+8
#define PMSG_Parse2	PROCMSG+9
#define PMSG_GetScr	PROCMSG+10
#define PMSG_LoseScr	PROCMSG+11
#define PMSG_WaitMem	PROCMSG+12
#define PMSG_ShutDown	PROCMSG+13

#define SCRO_This	0
#define SCRO_Next	1
#define SCRO_Prev	2

extern int scrSwitch(int fd, int type);

extern int makeCon(int rcvid, ...);
extern long sendChan(int chan, ...);
extern long sendCon(int fd, ...);
extern int recvMsg(int chan,void **msg);
extern int chkRecv(int chan);
extern void replyMsg(int rcvid,long reply);
extern void sendPulse(int chan, void *msg);
extern int askNotify(int fd, int chan, int type, ...);
extern int makeChanP(char *);
extern int makeChan();
extern int addName(char *name, int chan, ...);
extern long getSCOID(int RcvID);
extern int setErr(int RcvID, int error);
extern int _fillStat(void *msgp, int type);

#endif
