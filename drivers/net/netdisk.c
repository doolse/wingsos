#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <net.h>
#include <stdarg.h>
#include <termio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

uchar req[7];

/* Write code added by jbevren/dac */

void main(int argc, char *argv[]) {
	FILE *fp;
	char *server = argv[1],*newserve;
	int RcvID, thisChan, type, ret, sock;
	uchar *msg, *blk;
	uint blksize,i;
	
	if (argc < 3)
	{
		printf("Usage: netdisk server /dev/blkdev\n");
		exit(1);
	}
	newserve = malloc(strlen(server)+12);
	sprintf(newserve, "/dev/tcp/%s:5000", server);
	sock = open(newserve, O_RDWR);
	if (sock == -1)
	{
		perror("netdisk");
		exit(1);
	}
	retexit(1);
	thisChan = makeChanP(argv[2]);
	while(1) {
		RcvID = recvMsg(thisChan,(void *) &msg);
		type = (int) *msg;
		ret = -1;
		switch (type) {
			case IO_OPEN:
				ret = makeCon(RcvID, 1);
				break;
			case IO_CLOSE:
				break;
			case IO_READB:
				// This is crap
				req[0] = 'r';
				blksize = *(uint16 *)(msg+10);
				*(uint32 *)(req+1) = *(uint32 *)(msg+6) * blksize;
				*(uint16 *)(req+5) = blksize;
				write(sock, req, 7);
				read(sock, req, 1);
				blk = * (uchar **)(msg+2);
				type = read(sock, blk, blksize);
				ret = blksize;
				break;
			case IO_WRITEB:
				// Same crap
				req[0]='w';
				blksize = *(uint16 *)(msg+10);
				*(uint32 *)(req+1) = *(uint32 *)(msg+6) *blksize;
				*(uint16 *)(req+5) = blksize;
				write(sock, req, 7);
				read(sock, req, 1);
				blk = * (uchar **)(msg+2);
				type = write(sock, blk, blksize);
				ret = blksize;
				break;
		}
		replyMsg(RcvID,ret);
	}
	
}

