#include <stdio.h>
#include <wgslib.h>
#include <fcntl.h>
#include <net.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>
#include <wgsipc.h>
#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define BUFLEN 1460

enum {
	NONE,
	GET,
	POST
} Methods;

enum {
	NORMAL,
	POSTING,
	SENDING,
	DONE,
	CGI
} States;

typedef struct webStruct {
	struct webStruct *next;
	struct webStruct *prev;
	char buf[BUFLEN];
	char inbuf[BUFLEN];
	char line[512];
	unsigned int blen;
	unsigned int bdone;
	unsigned int llen;
	int num;
	int method;
	int sockfd;
	int filefd;
	int state;
	int rsblock;
	int wsblock;
	int deldir;
	char *req;
	char *referrer;
	char *browser;
	char *filestr;
	char *args;
	unsigned int arglen;
	unsigned int conlen;
} webCon;

int chan;
int conup;

char *servedir="./";

int flushout(webCon *cur) {
	int amount;
	
	amount = write(cur->sockfd, cur->buf+cur->bdone, cur->blen - cur->bdone);
	if (amount == -1) {
		if (errno == EAGAIN) {
			askNotify(cur->sockfd, chan, IO_NFYWRITE, cur->num, 1);
			cur->wsblock = 1;
			return 1;
		} else return 0;
	} else {
		cur->bdone += amount;
		if (cur->bdone >= cur->blen) {
			cur->blen = 0;
			cur->bdone = 0;
		}
		cur->wsblock = 0;
	}
	return 1;
}

char *getmime(char *filestr, int *exec) {
	char *dotstr;
	*exec = 0;
	
	if ((dotstr = strrchr(filestr,'.'))!=NULL) {
		if (!strncasecmp(dotstr,".htm",4))
			return "text/html";
		else if (!strcasecmp(dotstr,".an")) {
			*exec = 1;
			return "app/an-script";
		}
		else if (!strcasecmp(dotstr,".gif"))
			return "image/gif";
		else if (!strcasecmp(dotstr,".jpg") || !strcasecmp(dotstr,".jpeg"))
			return "image/jpeg";
	}
	return "text/plain";
}

void logout(webCon *cur, NetStat *in) {	
	printf("%s:%s:%s:%s\n", inet_ntoa(in->DIP), cur->req, cur->referrer, cur->browser);
}

unsigned int enclen(char *str) {
	unsigned int upto = 0;
	
	while (1) {
		switch (*str) {
			case '+':
			case '&':
			case '%':
				upto+=3;
				break;
			case 0:
				return upto;
			default:
				upto++;
				break;
		}
		str++;
	}
}

void encode(char *str, char *val) {
	while (1) {
		switch (*val) {
			case '+':
			case '&':
			case '%':
				sprintf(str, "%%%02x", *val);
				str += 2;
				break;
			case ' ':
				*str = '+';
				break;
			case 0:
				*str = '&';
				str[1] = 0;
				return;
			default:
				*str = *val;
				break;
		}
		val++;
		str++;
	}
}

void outf(webCon *cur, char *str, ...) {
	va_list args;
	
	va_start(args, str);
	vsprintf(cur->buf+cur->blen, str, args);
	cur->blen = strlen(cur->buf);
}

void addarg(webCon *cur, char *name, char *val) {
	char *arg = cur->args;
	unsigned int newlen = strlen(arg) + strlen(name) + enclen(val) + 2;
	
	if (newlen > cur->arglen) {
		newlen += 64;
		if (!arg) {
			arg = xmalloc(newlen);
			*arg = 0;
		} else arg = realloc(arg, newlen);
		cur->arglen = newlen;
		cur->args = arg;
	}
	strcat(arg, name);
	encode(&arg[strlen(arg)], val);
}


void showEnt(char *name, char *file, struct stat *buf, FILE *stream2) {
	char *content="Directory";
	int ch='/';
	
	if (S_ISDEV(buf->st_mode)) 
		content = "Special";
	else if (!S_ISDIR(buf->st_mode)) {
		content = getmime(name, &ch);
		ch = ' ';
	}
	fprintf(stream2, "<tr>\n"
	"<td width=\"40%%\"><a href=\"%s\">%s%c</a></td>\n"
	"<td>%s</td>\n"
	"<td>%10ld</td>\n</tr>\n", file, name, ch, content, buf->st_size);
}

void doDir(char *actfile, char *filestr, FILE *stream2) {
	DIR *dir;
	struct dirent *entry;
	char *file,*fullname;
	struct stat buf;
	int err;

	dir = opendir(actfile);
	if (dir) {
		file = fpathname("..", filestr, 0);
		fprintf(stream2, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n<html>\n<head>\n<title>Listing dir</title></head><body bgcolor=\"white\">\n"
  		"<h2>Listing dir %s</h2>\n"
		"<a href=\"%s\">Back to parent dir</a><p>\n"
		"<table>\n"
		"<tr><td>Name</td><td>Type</td><td>Length</td></tr>", filestr, file);
		free(file);
		while (entry = readdir(dir)) {
			file = fpathname(entry->d_name, filestr, 0);
			fullname = fpathname(entry->d_name, actfile, 0);
			err = stat(fullname, &buf);
			if (err != -1)
				showEnt(entry->d_name, file, &buf, stream2);
			free(file);
			free(fullname);
		}
		fprintf(stream2, "</table>\n</body>\n</html>\n");
		closedir(dir);
	}
}

void decatr(char *str) {
	char *d=str;
	int temp;
	
	while (1) {
		switch (*str) {
		case '%':
			temp = str[3];
			str[3] = 0;
			*d = strtol(str+1, &str, 16);
			*str = temp;
			break;
		case '+':
			*d = ' ';
			str++;
			break;
		case 0:
			*d = 0;
			return;
		default:
			*d = *str;
			str++;
		}
		d++;
	}
}

webCon *findCon(webCon *head, int num) {
	webCon *cur = head;
	if (!head) {
		return NULL;
	}
	do {
		if (cur->num == num)
			return cur;
		cur = cur->next;
	} while (cur != head);
}

webCon *closecon(webCon *cur, webCon *head) {
	
	head = remQueue(head, cur);
	close(cur->sockfd);
	if (cur->filefd != -1) {
		close(cur->filefd);
	}
	if (cur->deldir)
		remove(cur->filestr);
	if (cur->filestr)
		free(cur->filestr);
	if (cur->args)
		free(cur->args); 
	if (cur->req)
		free(cur->req); 
	if (cur->referrer)
		free(cur->referrer); 
	if (cur->browser)
		free(cur->browser); 
	free(cur);
	return head;
}

void prepCon(webCon *cur, int sockfd) {

	memset(cur, 0, sizeof(webCon));
	cur->sockfd = sockfd;
	cur->state = NORMAL;
	conup++;
	cur->num = conup;
	cur->filefd = -1;
}

void do404(webCon *cur, char *str) {
	sprintf(cur->buf, 
	"HTTP/1.1 404 Not Found\n"
	"Content-Type: text/html\n\n"
	"<html><head><title>Error 404 not found</title></head>\n"
	"<body><h1>Error 404</h1>\n"
	"<p>The file you requested \"%s\" could not be found.\n"
	"<p><i>Generated by JWEB Serve V1 by Jolse Maginnis</i>\n"
	"</body></html>\n", str);
	cur->blen = strlen(cur->buf);
	cur->state = DONE;
}

void do301dir(webCon *cur, char *file, char *query) {
	char *str = malloc(enclen(file)+2);
	encode(str, file);
	str[strlen(str)-1] = 0;
	if (query) {
		sprintf(cur->buf, 
		"HTTP/1.1 301 Moved Permanently\n"
		"Location: %s/?%s\n\n", str, query);
	} else {
		sprintf(cur->buf, 
		"HTTP/1.1 301 Moved Permanently\n"
		"Location: %s/\n\n", str);
	}
	cur->blen = strlen(cur->buf);
	cur->state = DONE;
}

void startSend(webCon *cur) {
	char *str;
	char *file, *temp, *actfile, *upto;
	struct stat buf;
	NetStat incoming;
	int res;
	char *mime;
	FILE *stream;

	upto = str = fpathname(cur->filestr,"/", 0);
	file = strsep(&upto, "?");
	temp = strsep(&upto, "");
	decatr(file);
	addarg(cur, "FILE=", file);
	if (temp)
		addarg(cur, "QUERY=", temp);
	actfile = fpathname(file+1, servedir, 0);
	res = stat(actfile, &buf);
	if (res != -1) {
		if (S_ISDIR(buf.st_mode)) {
			if (file[strlen(file)-1] != '/') {
				do301dir(cur, file, temp);
				goto end;
			}
			temp = fpathname("index.html", actfile, 0);
			res = stat(temp, &buf);
			if (res == -1) {
				free(temp);
				temp = tmpnam(NULL);
				stream = fopen(temp, "wb");
				if (stream) {
					cur->deldir = 1;
					doDir(actfile, file, stream);
					fclose(stream);
					actfile = strdup(temp);
					res = stat(temp, &buf);
				} 
			} else {
				free(actfile);
				actfile = temp;
			}
		}
	}
	free(cur->filestr);
	cur->filestr = actfile;
	if (res == -1) {
		do404(cur, file);
		goto end;
	} else {
		if (cur->deldir) {
			mime = "text/html";
			res = 0;
		} else mime = getmime(actfile, &res);
		status(cur->sockfd,&incoming);
		logout(cur, &incoming);
		if (res) {
			addarg(cur, "IP=", inet_ntoa(incoming.DIP));
			outf(cur, "HTTP/1.1 200 OK\r\n");
			cur->state = CGI;
			return;
		} else {
			cur->filefd = open(actfile, O_READ);
			if (cur->filefd == -1) {
				do404(cur, file);
				goto end;
			}
			outf(cur, "HTTP/1.1 200 OK\r\nConnection: close\r\n");
			if (buf.sizeexact)
				outf(cur, "Content-Length: %ld\r\n", buf.st_size);
			outf(cur, "Content-Type: %s\r\n\r\n",mime);
			cur->state = SENDING;
		}
	}
	end:
	free(str);
}

void gotline(webCon *cur) {
	char *str = cur->line;
	char *file;
	int method = 0;

	if (*str == 0) {
		if (!cur->filestr) {
			cur->state = DONE;
			return;
		}
		if (cur->method == GET)
			startSend(cur);
		else {
			if (cur->conlen)
				cur->state = POSTING;
			else startSend(cur);
		}
		return;
	}
	if (!strncasecmp("get ", str, 4))
		method = GET;
	else if (!strncasecmp("post ", str, 5))
		method = POST;
	if (method) {
		cur->method = method;
		cur->req = strdup(str);
		strsep(&str, " ");
		file = strsep(&str, " ");
		if (file)
			cur->filestr = strdup(file);
	} else if (!strncasecmp("User-Agent: ", str, 12)) {
		addarg(cur, "BROWSER=", str+12);
		cur->browser = strdup(str+12);
	} else if (!strncasecmp("Referer: ", str, 9)) {
		addarg(cur, "REFERRER=", str+9);
		cur->referrer = strdup(str+9);
	} else if (!strncasecmp("Content-length: ", str, 16))
		cur->conlen = strtol(str+16, NULL, 0);
}

int readser(webCon *cur) {
	int amount,ch;
	char *upto,*str;
	
	upto = cur->inbuf;
	str = cur->line;
	amount = read(cur->sockfd, upto, BUFLEN);
	if (amount == -1) {
		if (errno == EAGAIN) {
			askNotify(cur->sockfd, chan, IO_NFYREAD, cur->num, 0);
			cur->rsblock = 1;
			return 1;
		} else return 0;
	} else 
	if (amount) {
		while (amount--) {
			ch = *upto;
			upto++;
			switch (cur->state) {
			case POSTING:
				--cur->conlen;
				if (cur->llen < 511)
					str[cur->llen++] = ch;
				if (!cur->conlen) {
					str[cur->llen] = 0;
					addarg(cur, "POSTED=", str);
					startSend(cur);
				}
				break;
			default:
				if (ch == 10) {
					str[cur->llen] = 0;
					gotline(cur);
					cur->llen = 0;
				} else 
				if (cur->llen < 511 && ch != 13)
					str[cur->llen++] = ch;
				break;
			}
		}
		cur->rsblock = 0;
		return 1;
	} else return 0;
}

int tryFill(webCon *cur) {
	switch(cur->state) {
		case CGI:
			if (cur->blen)
				return 1;
			else {
				redir(cur->sockfd, STDOUT_FILENO);
				spawnl(0, cur->filestr, "-i", cur->args, NULL);
				return 0;
			}
		case SENDING:
			if (cur->blen < BUFLEN) {
				int amount;
				amount = read(cur->filefd, cur->buf+cur->blen, BUFLEN - cur->blen);
				if (amount == -1 || !amount)
					cur->state = DONE;
				else cur->blen += amount;
				return 1;
			}
		default:
			return 1;
	}
}

int main(int argc, char *argv[]) {
	webCon *cur;
	int sockfd, newfd;
	char *msg;
	int type,RcvID,done,res;
	webCon *head = NULL;
	
	if (optind<argc)
		servedir = argv[optind];
	servedir = fpathname(servedir, get_current_dir_name(), 0);
   	printf("JWeb Server V1.0\nServing on port 80 from %s\n",servedir);
	chan = makeChan();
	retexit(1);
	sockfd = open("/dev/tcpl/80",O_READ|O_WRITE|O_NONBLOCK);
	askNotify(sockfd, chan, IO_NFYWRITE, NULL);
	
	while (1) {
		done = 1;
		cur = head;
		while (head) {
			if (!cur->rsblock) {
				if (!readser(cur)) goto bad;
				else if (!cur->rsblock)
					done = 0;
			}
			if (!tryFill(cur)) goto bad;
			if (!cur->wsblock && cur->blen) {
				if (!flushout(cur)) goto bad;
				else if (!cur->wsblock)
					done = 0;
			}
			if (cur->state == DONE && !cur->blen)
				goto bad;
			cur = cur->next;
			if (cur == head) {
				if (done)
					break;
				else {
					done = 1;
					if (chkRecv(chan))
						break;
				}
			}
			continue;
			bad:
			cur = head = closecon(cur, head);				
		}
		RcvID = recvMsg(chan, (void *)&msg);
		type = * (unsigned char *)msg;
		switch (type) {
		case IO_NFYREAD:
			cur = findCon(head, * (int *)(msg+1));
			if (cur) {
				cur->rsblock = 0;
			}
			break;
		case IO_NFYWRITE:
			if (!(* (long *)(msg+1) & 0xffffff)) {
				newfd = accept(sockfd, NULL, NULL);
				askNotify(sockfd, chan, IO_NFYWRITE, NULL);
				if (newfd == -1) 
					break;
				cur = xmalloc(sizeof(webCon));
				prepCon(cur, newfd);
				head = addQueue(head, head, cur);
				break;
			} else {
				cur = findCon(head, * (int *)(msg+1));
				if (cur) {
					cur->wsblock = 0;
				}
			}
		default:
			break;
		}
		replyMsg(RcvID, 0);
	}
}
