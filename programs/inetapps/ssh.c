#include <stdio.h>
#include <fcntl.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include "ssh.h"

char outbuf[1500];
char inbuf[1500];

int chan;
int haveread;
int sockfd;
int readver;
int packbytes;
uint lenbytes=4;
uint inupto;
uint packlen;
uint reallen=5;
uint padding;
int state;

extern void md5(uchar *src, uint len, uchar *out);

enum {
	ST_GETKEYS,
	ST_AUTH,
	ST_PREP,
	ST_INTERACT
};

char version[] = "SSH-1.5-Jolz\n";

void procpack(uchar *pack, uint len) {
	printf("Packet type %2x, len %d\n", pack[0], len);
	switch (state) {
		case ST_GETKEYS:
			break;
		case ST_AUTH:
			break;
		case ST_PREP:
			break;
		case ST_INTERACT:
			break;
	}
}

uint32 tolong(uchar *ptr) {
	return (ptr[0] << 24) + (ptr[1] << 16) + (ptr[2] << 8) + ptr[3];
}

void processread() {
	int len;
	uint ch;
	uchar *upto = inbuf;
	
	goagain:
	len = read(sockfd, inbuf, 1500);
	if (len == -1) {
		if (errno == EAGAIN) {
			askNotify(sockfd, chan, IO_NFYREAD, NULL);
			return;
		} else {
			perror("ssh");
			exit(1);
		}
	}
	if (!readver) {
		while (len) {
			len--;
			ch = *upto;
			upto++;
			if (ch == '\n') {
				readver = 1;
				break;
			}
		}
	}
	while (len) {
		len--;
		inbuf[inupto] = *upto;
		upto++;
		inupto++;
		if (lenbytes) {
			lenbytes--;
			if (!lenbytes) {
				packlen = tolong(inbuf);
				padding = 8-(packlen&7);
				reallen = packlen+padding;
				inupto = 0;
			}
		} else {
			if (inupto >= reallen) {
				procpack(inbuf+padding, packlen);
				lenbytes = 4;
				inupto = 0;
				reallen = 5;
			}
		}
	}
	goto goagain;
}

int main(int argc, char *argv[]) {
	char *msg;
	int type,RcvID,err;
	uchar hashmd[16];
	
	md5(argv[1], strlen(argv[1]), hashmd);
	for (err=0;err<16;err++) {
		printf("%02x", hashmd[err]);
	}
	printf("\n");
	exit(1);
	chan = makeChan();
	sockfd = open("/dev/tcp/192.168.0.1:22",O_READ|O_WRITE|O_NONBLOCK);
	askNotify(sockfd, chan, IO_NFYWRITE, NULL);
	
	while (1) {
		RcvID = recvMsg(chan, (void *)&msg);
		type = * (unsigned char *)msg;
		switch (type) {
		case IO_NFYREAD:
			processread();
			break;
		case IO_NFYWRITE:
			err = write(sockfd, version, strlen(version));
			if (err == -1) {
				perror("ssh");
				exit(1);
			}
			askNotify(sockfd, chan, IO_NFYREAD, NULL);
			break;
		}
		replyMsg(RcvID, 0);
	}
}
