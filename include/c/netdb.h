#ifndef _netdb_h
#define	_netdb_h

#include <sys/types.h>
#include <netinet/in.h>

struct hostent {
	char *h_name;
	char **h_aliases;
	int h_addrtype;
	int h_length;
	char **h_addr_list;
};
#define h_addr h_addr_list[0]

struct hostent *gethostbyname(const char *name);
struct hostent *gethostbyaddr(const void *addr, ssize_t len, int type);
struct hostent *gethostent(void);

#endif
