
#ifndef _NET_H_
#define _NET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define NETMSG		0xa0

#define NET_AddInt	(NETMSG+0)
#define NET_RemInt	(NETMSG+1)
#define NET_EOF		(NETMSG+2)
#define NET_Status	(NETMSG+3)
#define NET_PacketSend	(NETMSG+4)
#define NET_PacketRecv	(NETMSG+5)
#define NET_Accept	(NETMSG+6)
#define NET_StatAll	(NETMSG+7)
#define NET_IntAll	(NETMSG+8)
#define NET_StatInt	(NETMSG+9)
#define NET_DNSAddr     (NETMSG+10)

typedef struct {
	struct in_addr SIP;
	struct in_addr DIP;
	uint SegMax;
	uint SPort;
	uint DPort;
	uint State;
	uint SendQ;
	uint RecvQ;
} NetStat;

typedef struct {
	struct in_addr PIP;
} PPPIStat;

typedef struct {
	struct in_addr PIP;
} ETHIStat;

enum {
	PPPType,
	ETHType
};

typedef struct {
	int Type;
	struct in_addr IP;
	struct in_addr Mask;
	uint MaxPacket;
	uint32 RX;
	uint32 TX;
	uint32 RXErr;
	uint32 TXErr;
	uint32 RXBytes;
	uint32 TXBytes;
	union {
	PPPIStat Pint;
	ETHIStat Eint;
	} Int;
} IntStat;

extern int sendeof(int fd);
extern int status(int fd, NetStat *);
extern int statall(int fd, NetStat *, int size);
extern int statint(int fd, IntStat *, int size);

#endif
