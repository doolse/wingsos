#include <fcntl.h>
#include <wgsipc.h>

int main (int argc, char *argv[])
{      	
	int fd;
	fd = open("/sys/blk.iec",O_READ);
	sendCon(fd,PMSG_ShutDown);
}
