#include <stdio.h>
#include <time.h>

extern int readDS1202(struct tm *tm, int port);
char formdate[64];

int main(int argc, char *argv[]) {
    struct tm thetime;
    time_t rtime;
    uint port;
        
    for (port=0; port<2; port++)
    {
	if (readDS1202(&thetime, port))
	{
            strftime(formdate, sizeof(formdate), "%c", &thetime);
	    rtime = mktime(&thetime);
    	    printf("Read RTC to be '%s'\n", formdate);
    	    settime(rtime);
	    setstart(rtime);
	    break;
	}
    }
    return 0;
}
