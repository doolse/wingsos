#include <fcntl.h>
#include <wgsipc.h>

int main (int argc, char *argv[])
{	
	int fd,i;
	
	fd = open("/sys/ppp0",O_READ|O_WRITE|O_PROC);
	if (fd != -1) {
		sendCon(fd, IO_CONTROL);
		close(fd);
	} else printf("Couldn't hang up!\n");
}
