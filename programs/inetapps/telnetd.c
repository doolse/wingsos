#include <stdio.h>
#include <wgslib.h>
#include <fcntl.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define BUFLEN 1500

#define debout(a) a

int chan;
int spy=0;

enum {
	NORMAL,
	CTRL,
	CTRL2,
	RET
} States;

#define WILL 0xfb
#define WONT 0xfc
#define DO 0x0fd
#define DONT 0xfe

typedef struct conStruct {
	struct conStruct *next;
	struct conStruct *prev;
	unsigned char inbuf[BUFLEN];
	unsigned char topty[BUFLEN];
	unsigned char toserver[BUFLEN];
	unsigned int plen;
	unsigned int slen;
	int ptys[2];
	int sockfd;
	int pid;
	int state;
	int type;
	int num;
	int sblock;
	int pblock;
	int wsblock;
	int wpblock;
} telCon;

int conup;

void flushserve(telCon *cur) {
	int res;
	if (cur->slen) {
		res = write(cur->sockfd, cur->toserver, cur->slen);
		if (spy)
			write(fileno(stdout), cur->toserver, cur->slen);
		cur->slen = 0;
	}
}

void outserve(telCon *cur, int ch) {
	cur->toserver[cur->slen++] = ch;
	if (cur->slen > BUFLEN-100) {
		flushserve(cur);
	}
}

void flushpty(telCon *cur) {
	int res;
	
	if (cur->plen) {
		res = write(cur->ptys[1], cur->topty, cur->plen);
		cur->plen = 0;
	}
}

void outpty(telCon *cur, int ch) {
	cur->topty[cur->plen++] = ch;
	if (cur->plen > BUFLEN-100) {
		flushpty(cur);
	}
}

void sendOpt(telCon *cur, int type, int code) {	
	outserve(cur, 0xff);
	outserve(cur, type);
	outserve(cur, code);
}

telCon *findCon(telCon *head, int num) {
	telCon *cur = head;
	if (!head) {
		return NULL;
	}
	do {
		if (cur->num == num)
			return cur;
		cur = cur->next;
	} while (cur != head);
}

void chkWillDo(telCon *ourCon, int ch) {
	
	switch(ch) {
		case 3:
			break;
		default:
			sendOpt(ourCon, DONT, ch);
			break;
	}
}

void chkWontDont(telCon *ourCon, int ch) {
	switch(ch) {
		case 1:
			sendOpt(ourCon, WILL, 1);
			break;
		default:
			break;
	}
}

void prepCon(telCon *cur, int sockfd) {

	cur->sockfd = sockfd;
	cur->state = NORMAL;
	cur->plen = 0;
	cur->slen = 0;
	cur->sblock = 0;
	cur->pblock = 0;
	cur->num = ++conup;
	sendOpt(cur, DO, 3);
	sendOpt(cur, DO, 1);
	flushserve(cur);
}

int readser(telCon *cur) {
	int amount,ch;
	unsigned char *upto;
	
	upto = cur->inbuf;
	amount = read(cur->sockfd, upto, 1024);
	if (amount == -1) {
		if (errno == EAGAIN) {
			askNotify(cur->sockfd, chan, IO_NFYREAD, cur->num, 0);
			flushserve(cur);
			flushpty(cur);
			cur->sblock = 1;
			return 1;
		} else return 0;
	} else 
	if (amount) {
		while (amount--) {
			ch = *upto;
			upto++;
			switch (cur->state) {
				case CTRL:
					cur->type = ch;
					cur->state = CTRL2;
					break;
				case CTRL2:
					switch (cur->type) {
						case WILL:
						case DO:
							chkWillDo(cur, ch);
							break;
						case WONT:
						case DONT:
							chkWontDont(cur, ch);
							break;
						default:
							break;
					} 
					cur->state = NORMAL;
					break;
				case RET:
					if (ch == 10 || !ch) 
						break;
					else outpty(cur, ch);
					break;
				default:
					if (ch == 0xff) {
						cur->state = CTRL;
						break;
					} else 
					if (ch == 13) {
						cur->state = RET;
					}
					outpty(cur, ch);
					break;
			}
		}
		cur->sblock = 0;
		return 1;
	} else return 0;
}

telCon *closecon(telCon *cur, telCon *head) {
	
	flushserve(cur);
	flushpty(cur);
	head = remQueue(head, cur);
	close(cur->ptys[1]);
	close(cur->sockfd);
	kill(cur->pid,1);
	free(cur);
	return head;
}

void logcon(int sock) {
	NetStat incoming;

	printf("Got connection!\n");
	status(sock,&incoming);
	printf("Got telnetd connection from %s\n", inet_ntoa(incoming.DIP));
	printf("Segment max %d\n",incoming.SegMax);
}

int readpty(telCon *cur) {
	int amount,ch;
	unsigned char *upto;
	
	upto = cur->inbuf;
	amount = read(cur->ptys[1], upto, 1024);
	if (amount == -1) {
		if (errno == EAGAIN) {
			askNotify(cur->ptys[1], chan, IO_NFYREAD, cur->num, 1);
			flushserve(cur);
			cur->pblock = 1;
			return 1;
		} else return 0;
	} else 
	if (amount) {
		while (amount--) {
			ch = *upto;
			upto++;
			outserve(cur, ch);
		}
		cur->pblock = 0;
		return 1;
	} else return 0;
}

int main(int argc, char *argv[]) {
	telCon *cur;
	int sockfd, newfd;
	char *msg;
	int type,RcvID,done,res;
	telCon *head = NULL;
	
	while (getopt(argc, argv, "s") != EOF) {
		spy=1;
	}
	retexit(1);
	chan = makeChan();
	sockfd = open("/dev/tcpl/23",O_READ|O_WRITE|O_NONBLOCK);
	askNotify(sockfd, chan, IO_NFYWRITE, NULL);
	
	while (1) {
		done = 1;
		cur = head;
		while (head) {
			if (!cur->sblock) {
				if (!readser(cur)) {
					cur = head = closecon(cur, head);
					continue;
				} else if (!cur->sblock)
					done = 0;
			}
			if (!cur->pblock) {
				if (!readpty(cur)) {
					cur = head = closecon(cur, head);
					continue;
				} else if (!cur->pblock)
					done = 0;
			}
			cur = cur->next;
			if (cur == head) {
				if (done)
					break;
				else done = 1;
			}
		}
		RcvID = recvMsg(chan, (void *)&msg);
		type = * (unsigned char *)msg;
		switch (type) {
		case IO_NFYREAD:
			cur = findCon(head, * (int *)(msg+1));
			if (cur) {
				if (* (unsigned char *)(msg+3) == 1)
					cur->pblock = 0;
				else cur->sblock = 0;
			}
			break;
		case IO_NFYWRITE:
			if (!(* (long *)(msg+1) & 0xffffff)) {
	
				cur = malloc(sizeof(telCon));
				newfd = accept(sockfd, NULL, NULL);
				askNotify(sockfd, chan, IO_NFYWRITE, NULL);
				if (newfd == -1) {
					debout(fprintf(stderr, "Couldn't accept connection!\n"));
					goto bad;
				} else
				if (getpty(cur->ptys) != -1) {
					debout(logcon(newfd));
					head = addQueue(head, head, cur);
					noinh(cur->ptys[1]);
					redir(cur->ptys[0],STDOUT_FILENO);
					redir(cur->ptys[0],STDIN_FILENO); 
					redir(cur->ptys[0],STDERR_FILENO);
					cur->pid = spawnlp(S_LEADER,"login",NULL);
					close(cur->ptys[0]);
					setFlags(cur->ptys[1], O_READ|O_WRITE|O_NONBLOCK);
				   	prepCon(cur, newfd);
				} else {
					debout(fprintf(stderr, "Couldn't get pty!\n"));
					close(newfd);
					goto bad;
				}
			}
			break;
			bad:
			free(cur);
		}
		replyMsg(RcvID, 0);
	}
}
