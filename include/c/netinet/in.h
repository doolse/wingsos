#ifndef _netinet_in_h
#define	_netinet_in_h

typedef unsigned int in_port_t;
typedef unsigned long in_addr_t;

#include <sys/socket.h>
#include <arpa/inet.h>


struct in_addr {
	in_addr_t s_addr;
};

struct sockaddr_in {
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	unsigned char sin_zero[8];
};

#define IPPROTO_IP	0
#define IPPROTO_ICMP	1
#define IPPROTO_TCP	6
#define IPPROTO_UDP	17

#define INADDR_ANY	0
#define INADDR_BROADCAST 0xffffffff

#endif
