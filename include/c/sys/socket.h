#ifndef _sys_socket_h
#define	_sys_socket_h

#include <sys/types.h>

typedef unsigned int socklen_t;
typedef unsigned int sa_family_t;

struct sockaddr {
	sa_family_t sa_family;
	char sa_data[14];
};

#define SOCK_DGRAM	1
#define SOCK_STREAM	2
#define SOCK_SEQPACKET	3

#define SOL_SOCKET	1

#define SO_ACCEPTCONN	1
#define SO_BROADCAST	2
#define SO_DEBUG	3
#define SO_DONTROUTE	4
#define SO_ERROR	5
#define SO_KEEPALIVE	6
#define SO_LINGER	7
#define SO_OOBINLINE	8
#define SO_RCVBUF	9
#define SO_RCVLOWAT	10
#define SO_RCVTIMEO	11
#define SO_REUSEADDR	12
#define SO_SNDBUF	13
#define SO_SNDLOWAT	14
#define SO_SNDTIMEO	15
#define SO_TYPE		16

#define AF_UNIX		1
#define AF_UNSPEC	2
#define AF_INET		3

#define SHUT_RD		1
#define SHUT_WR		2
#define SHUT_RDWR	3

#define MSG_CTRUNC	1
#define MSG_DONTROUTE	2
#define MSG_EOR		3
#define MSG_OOB		4
#define MSG_PEEK	5
#define MSG_TRUNC	6
#define MSG_WAITALL	7

int accept(int socket, struct sockaddr *address, socklen_t *address_len);
int bind(int socket, const struct sockaddr *address, socklen_t address_len);
int connect(int socket, const struct sockaddr *address, socklen_t address_len);
int getpeername(int socket, struct sockaddr *address, socklen_t *address_len);
int getsockname(int socket, struct sockaddr *address, socklen_t *address_len);
int listen(int socket, int backlog);
ssize_t recv(int socket, void *buffer, ssize_t length, int flags);
ssize_t recvfrom(int socket, void *buffer, ssize_t length, int flags, struct sockaddr *address, socklen_t *address_len);

ssize_t send(int socket, const void *message, ssize_t length, int flags);
ssize_t sendto(int socket, const void *message, size_t length, int flags,
             const struct sockaddr *dest_addr, socklen_t dest_len);


ssize_t recvmsg(int socket, struct msghdr *message, int flags);
ssize_t sendmsg(int socket, const struct msghdr *message, int flags);

int getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len);
int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
int shutdown(int socket, int how);
int socket(int domain, int type, int protocol);
int socketpair(int domain, int type, int protocol, int socket_vector[2]);

#endif
