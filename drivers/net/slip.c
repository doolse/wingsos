#include <stdio.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <net.h>

#define ESC 0333
#define END 0300

#define MAXPACK 2000

char packet[MAXPACK];
int thisChan;
FILE *outstream;

void outThread() {
	FILE *stream;
	unsigned char *msg;
	unsigned char *pack;
	int rcvid;
	int size;
	int ch;
	
	stream = outstream;
	while(1) {
		rcvid = recvMsg(thisChan,(void *) &msg);
		if ((int) *msg == NET_PacketSend) {
			pack = *(unsigned char **)(msg+2);
			size = *(int *)(msg+6);
			fputc(END,stream);
			while (size--) {
				ch = *pack;
				if (ch == END) {
					fputc(ESC,stream);
					ch = 0334;
				} else
				if (ch == ESC) {
					fputc(ESC,stream);
					ch = 0335;
				}
				fputc(ch,stream);
				pack++;
			}
			fputc(END,stream);
			fflush(stream);
		}
		replyMsg(rcvid,0);
	}
}

void main(int argc, char *argv[]) {
	int ch;
	int count;
	int ipFD;
	FILE *stream;

	ipFD = findNameP("/sys/tcpip");
	if (ipFD == -1) {
		fprintf(stderr,"TCP/IP not loaded!\n");
		exit(1);
	} 
	if (argc<2) {
		fprintf(stderr,"Usage: slip device\n");
		exit(1);
	}
	stream = fopen(argv[1],"rb");
	if (!stream) {
		perror("slip");
		exit(1);
	}
	outstream = fdopen(fileno(stream),"wb");
	thisChan = makeChanP("/sys/slip0");
	newThread(outThread,200,NULL);
	retexit(0);
	count = 0;
	while ((ch = fgetc(stream)) != EOF) {
		if (ch == END) {
			if (count > 19) {
				sendChan(ipFD,NET_PacketRecv,packet);
			}
			count = 0;
		}
		else {
			if (ch == ESC) {
				ch = fgetc(stream);
				if (ch == 0334)
					ch = END;
				else if (ch == 0335)
					ch = ESC;
			}
			if (count<MAXPACK)
				packet[count++]=ch;
		}
	}
}

