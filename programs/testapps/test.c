#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main (int argc, char *argv[])
{
	unsigned int i;
	for (i=0;i<65000;i++)
	{
		char *tmp = strdup(tmpnam(NULL));
		char *tmp2 = strdup(tmpnam(NULL));
		int fd = open(tmp, O_CREAT|O_EXCL);
		int ren = rename(tmp, tmp2);
		printf("%s %s,%d,%d\n", tmp, tmp2, fd, ren);
		close(fd);
		remove(tmp2);
		free(tmp);free(tmp2);
	}
	
}
