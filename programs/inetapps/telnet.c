#include <stdio.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>
#include <termio.h>
#include <stdlib.h>

int globsock;
int channel;

#define ACHAR	1
#define FLUSH	2
#define SPEC	3

#define CH_WILL 0xfb
#define CH_WONT 0xfc
#define CH_DO	0xfd
#define CH_DONT 0xfe

void fromUser(int *tlc) {
	int ch;
	
	stdin->_flags |= _IOBUFEMP;
	while (1) {
		while ((ch = getchar()) != EOF)
			sendChan(channel,ACHAR,ch);
		sendChan(channel,FLUSH);
	}
}

void toServer(int *tlc) {
	FILE *stream;
	int *msg;
	int RcvId;
	int mtype;
	
	stream = fdopen(*tlc,"wb");
	while (1) {
		RcvId = recvMsg(channel, (void *) &msg);
		mtype = msg[0];
		if (mtype == ACHAR)
			fputc(msg[1],stream);
		else if (mtype == FLUSH)
			fflush(stream);
		else if (mtype == SPEC) {
			fputc(0xff,stream);
			fputc(msg[1], stream);
			fputc(msg[2], stream);
		}
		replyMsg(RcvId,0);
	}
}

void main(int argc, char *argv[]) {
	int ch,last,port=23;
	FILE *stream;
	int control,type;
	struct termios tio;
	char *server,*newserve;

	if (argc>1) {
		server = argv[1];
		if (!strchr(server, '/')) {
			newserve = malloc(strlen(server)+12);
			if (argc>2)
				port = atoi(argv[2]);
			sprintf(newserve, "/tcp/%s:%u", server, port);
			server = newserve;
		}
		stream = fopen(server,"r");
		if (!stream) {
			perror("telnet");
			exit(1);
		}
		printf("Connected\n");
		gettio(STDIN_FILENO,&tio);
		tio.flags &= ~(TF_ICANON|TF_ECHO|TF_ISIG|TF_ICRLF);
		settio(STDIN_FILENO,&tio);
		globsock = dup(fileno(stream));
		channel = makeChan();
		newThread(toServer,256,&globsock);
		newThread(fromUser,256,&globsock);
		stream->_flags |= _IOBUFEMP;
		control = 0;
		last=0;
		while(!feof(stream) && !ferror(stream)) {
			while ((ch = fgetc(stream)) != EOF) {
				fflush(stderr);
				if (!control && ch == 0xff)
					control = 1;
				else if (control == 1) {
					type = ch;
					control = 2;
				} else if (control == 2) {
					control = 0;
					if (type == CH_DO) {
						sendChan(channel,SPEC,CH_WONT,ch);
						// printf("WONT %d\n", ch);
					} else 
					if (type == CH_WILL) {
						sendChan(channel,SPEC,CH_DO,ch);
						// printf("DO %d %x\n", ch, type);
					} 
					sendChan(channel,FLUSH);
				} else if (last != 0x0d || ch)
					putchar(ch);
				last = ch;
			}
			fflush(stdout);
		}
		printf("Connection closed by remote host!\n");
		exit(1);
	}
	
}

