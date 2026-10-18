#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <signal.h>
#include <poll.h>

// typedef unsigned int uint;
/* Fixed sizes: these overlay the little endian request fields. */
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef unsigned char uchar;

#define INBUFLEN 4096

uchar buf[INBUFLEN];
uchar blkbuf[8192];
uint inbuf;
uint unused;
uchar *cur=buf;
int sock;
int listsock;

/* Write code added by jbevren/dac */

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
		/* WiNGs can't close the connection when VICE quits or resets,
		 * so a new client replaces the current one. */
		struct pollfd fds[2] = {{sock, POLLIN}, {listsock, POLLIN}};
		if (poll(fds, 2, -1) == -1)
		{
			perror("poll");
			return NULL;
		}
		if ((fds[1].revents & POLLIN) && !(fds[0].revents & POLLIN))
			return NULL;
		amount = read(sock, buf+inbuf, INBUFLEN-inbuf);
		if (amount == -1)
		{
			/* e.g. a reset from a rebooted WiNGs: drop the client */
			perror("read");
			return NULL;
		}
		if (!amount)
			return NULL;
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
	int yes=1;
	
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
	
	/* A write to a reset connection must not kill us */
	signal(SIGPIPE, SIG_IGN);
	listsock = socket(AF_INET, SOCK_STREAM, 0);		
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(5000);
        my_addr.sin_addr.s_addr = INADDR_ANY;
	if (setsockopt(listsock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
	        perror("setsockopt");
        	exit(1);
	}
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
	while (1)
	{
	if ((sock = accept(listsock, (struct sockaddr *)&their_addr, &sin_size)) == -1)
	{
		perror("ACCEPT");
		exit(1);
	}
	printf("Client connected\n");
	inbuf = unused = 0;
	cur = buf;
	
	while (1)
	{
		uchar *com;
		uint32 amount;
		uint len;
		if (!(com = getcommand(1)))
			break;
		switch (com[0])
		{
		case 'r':
			if (!(com = getcommand(6)))
				break;
			len = *(uint16 *)(com+4);
			amount = *(uint32 *)com;
//			printf("Read blkoffs %lx, %d, %d\n", amount, amount/256, len);
			lseek(diskfd, amount, SEEK_SET);
			read(diskfd, blkbuf+1, len);
			blkbuf[0] = 0;
			write(sock, blkbuf, len+1);
			break;
		case 'w':
			if (!(com = getcommand(6)))
				break;
			len = *(uint16 *)(com+4);
			amount = *(uint32 *)com;
//			printf("Write blkoffs %lx, %d, %d\n", amount, amount/256, len);
			lseek(diskfd, amount, SEEK_SET);
			blkbuf[0] = 0;
			write(sock, blkbuf, 1);
			if (!(com = getcommand(len)))
				break;
			write(diskfd, com, len);
			break;
		}
		if (!com)
			break;
	}
	printf("Client disconnected\n");
	close(sock);
	}
}
