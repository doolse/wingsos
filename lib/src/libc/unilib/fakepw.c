#include <pwd.h>

struct passwd fake = {"wings", "nopass", 0,0, "WiNGS User", "/", "sh"};

struct passwd *getpwnam(const char *name){
	return &fake;
};
struct passwd *getpwuid(uid_t uid) {
	return &fake;
}
