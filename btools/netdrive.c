#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>

// typedef unsigned int uint;
typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uchar;

#define INBUFLEN 4096

uchar buf[INBUFLEN];
uchar blkbuf[8192];
uint inbuf;
uint unused;
uchar *cur=buf;
int sock;

uchar *getcommand(int min)
{
	uchar *ret;
	int amount;
	if (INBUFLEN-inbuf < min)
	{
		memmove(buf, cur, unused);
		cur = buf;
		inbuf -= unused;
		unused = 0;
	}
	while (unused < min)
	{
		amount = read(sock, buf+inbuf, INBUFLEN-inbuf);
		printf("Read %d\n", amount);
		if (amount == -1)
		{
			perror("FUCK");
			exit(1);
		}
		if (!amount)
			exit(1);
		inbuf += amount;
		unused += amount;
	}
	ret = cur;
	cur += min;
	unused -= min;
	if (!unused)
	{
		cur = buf;
		inbuf = 0;
	}
	return ret;
}

int main(int argc, char *argv[])
{
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size = sizeof(their_addr), new_fd;
	int diskfd;
	int listsock;
	
	if (argc<2)
	{
		printf("Usage: netdrive disk.d81\n");
		exit(1);
	}
	diskfd = open(argv[1], O_RDWR);
	if (diskfd == -1)
	{
		perror(argv[1]);
		exit(1);
	}
	
	listsock = socket(AF_INET, SOCK_STREAM, 0);		
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(5000);
        my_addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(listsock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
	{
		perror("BIND");
		exit(1);
	}
	if (listen(listsock, 1) == -1)
	{
		perror("LISTEN");
		exit(1);
	}
	if ((sock = accept(listsock, (struct sockaddr *)&their_addr, &sin_size)) == -1)
	{
		perror("ACCEPT");
		exit(1);
	}
	
	while (1)
	{
		uchar *com;
		uint32 amount;
		uint len;
		com = getcommand(1);
		printf("We got a '%c'\n", com[0]);
		switch (com[0])
		{
		case 'r':
			com = getcommand(6);
			len = *(uint16 *)(com+4);
			amount = *(uint32 *)com;
//			printf("Blkoffs %lx, %d, %d\n", amount, amount/256, len);
			lseek(diskfd, amount, SEEK_SET);
			read(diskfd, blkbuf+1, len);
			blkbuf[0] = 0;
			write(sock, blkbuf, len+1);
			break;
		case 'w':
			com = getcommand(6);
			break;
		}
	}
}
