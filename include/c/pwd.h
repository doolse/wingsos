#ifndef _pwd_h_
#define _pwd_h_

#include <sys/types.h>

struct passwd {
	char    *pw_name;       /* user name */
	char    *pw_passwd;     /* user password */
	uid_t   pw_uid;         /* user id */
	gid_t   pw_gid;         /* group id */
	char    *pw_gecos;      /* real name */
	char    *pw_dir;        /* home directory */
	char    *pw_shell;      /* shell program */
};

struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);

#endif             
