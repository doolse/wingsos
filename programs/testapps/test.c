#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <fcntl.h>
#include <iec.h>

int main (int argc, char *argv[])
{	unsigned int i;
	int fd = open(argv[1], O_READ);
	i = sendCon(fd, IO_CONTROL, IOCTL_Change)&0xffff;
	printf("Changed %d\n", i);
	close(fd);
}
