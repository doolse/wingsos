#include <stdio.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>
#include <ctype.h>
#include <console.h>
#include <stdlib.h>

#define IGNORE	0
#define NORMAL	1
#define ME	2
#define SERVER	3
#define NOTICE	4
#define JOIN	5
#define MODE	6
#define PART	7
#define QUIT	8
#define NICK	9
#define UNKCTCP	10
#define DCC	11

#define SRVMSG	1
#define SCRMSG	2
#define SCRMSG2	3
#define KEYINP	4
#define CLRINP	5

#define VERSIONREPLY "Ajirc V1.0 (c) Jolse Maginnis and An Greenwood"

#define IDENT "ajirc"
#define NAME "http://www.jolz64.cjb.net/"

int globsock;
char *nick="SilentB0B";
int channel;
char *chan="#hole";
int delnick=0;
int inpx=0,inpy=24;

void receivedcc(char * thestr) {
	unsigned long ipaddy,r,inbytes, t_inbytes,filesize;
	unsigned int a,b,c,d;
        int ch;
	char * filename, * tempstr, *port, *ipstr, *dccstr;
	FILE * infp, * outfp;

	dccstr = thestr;

	con_gotoxy(5,4);
	printf("%s",dccstr);

	//Point to "DCC"
	filename = strsep(&dccstr," ");
	filename = dccstr;
	//Point to "SEND"
	filename = strsep(&dccstr," ");
	filename = dccstr;

	//Point to filename
	filename = strsep(&dccstr," ");

	//Grab IP as String
	ipstr = dccstr;
	ipstr = strsep(&dccstr," ");
	
	//Grab PORT as string
	port = dccstr;
	port = strsep(&dccstr," ");

        filesize = strtoul(dccstr,NULL,10);

	ipaddy = strtoul(ipstr,NULL,10);

	a = (unsigned int)(ipaddy/16777216);
	r = ipaddy - (a * 16777216);

	b = (unsigned int)(r/65536);
	r = r - (b * 65536);

	c = (unsigned int)(r/256);
	r = r - (c * 256);

	d = r;

	tempstr = malloc(strlen("/dev/tcp/:  ") + 25);
	sprintf(tempstr,"/dev/tcp/%u.%u.%u.%u:%s",a,b,c,d,port);
        sendChan(channel, SCRMSG, "\x1b[0m%s", tempstr);

	outfp = fopen(filename,"w");
	infp = fopen(tempstr,"r+");
        inbytes = 0;
	t_inbytes = 0;

	while(!feof(infp)) {
		fputc(fgetc(infp),outfp);
		inbytes++;
                t_inbytes++;
		if(inbytes == 2048) {
			fflush(infp);
			fwrite(&t_inbytes,1,4,infp);
			fflush(infp);
			inbytes = 0;
		} else if(t_inbytes >= filesize) {
			inbytes = 0;
			fflush(infp);
			fwrite(&t_inbytes,1,4,infp);
			fflush(infp);
			if(feof(infp))
        sendChan(channel, SCRMSG, "\x1b[0mEOF ... Why aren't you finished.");
			else
        sendChan(channel, SCRMSG, "\x1b[0mThe File is still not ended.");
		}
	}
	fclose(infp);
	fclose(outfp);

        sendChan(channel, SCRMSG, "\x1b[0mDCC Received.");
}

void outThread(int *tlc) {
	FILE *stream;
	int RcvId;
	char *msg;
	char **upto;
	int count,type;
	int lastone=0;
	int ch;
	stream = fdopen(*tlc,"wb");
	
	while (1) {
		RcvId = recvMsg(channel, (void *)&msg);
		type = * (int *)msg;
		if (type == SRVMSG) {
			count = * (int *)(msg+2);
			upto = (char **)(msg+4);
			while (count--)
				fputs(*upto++,stream);
			fprintf(stream,"\n");
			fflush(stream);
		} else 
		if (type == KEYINP) {
			ch = * (int *)(msg+2);
			if (ch == '\b' || ch == 0x7f) {
				putchar('\b');
				putchar(' ');
				putchar('\b');
				--inpx;
			} else {
				putchar(ch);
				++inpx;
			}
			con_update();
		} else
		if (type == CLRINP) {
			inpx=0;
			con_gotoxy(inpx,inpy);
			con_clrline(LC_End);
			con_update();
		} else {
		
			con_gotoxy(0,22);
			if (lastone == SCRMSG)
				putchar('\n');
			vprintf(* (char **)(msg+2), (void *)(msg+6));
			lastone = type;
			con_gotoxy(inpx,inpy);
			con_update(); 
		}
		replyMsg(RcvId,0);
	}
	
}

void fromUser(void *tlc) {
	int done,i;
	int delold=0;
	char *freecommand,*command,*upto,*params,*to;
	sendChan(channel, SRVMSG, 2,"USER " IDENT " some thing :" NAME "\nNICK ",nick);

        freecommand = NULL;
	
	while (1) {
		if(freecommand)
			free(freecommand);
    
                do {
                  command = getmylinerestrict(NULL,(long)256,con_xsize-1,0,con_ysize-1,"",0);
                  if(!strlen(command)) {
                    free(command);
                    command = NULL;
                  }
                } while(!command);

                freecommand = command;

		if (*command == '/') {
			upto = ++command;
			command = strsep(&upto," ");
			params = upto;
			if (!strcasecmp(command,"join")) {
				sendChan(channel, SRVMSG, 2, "JOIN ",params);
				to = strsep(&params," ");
				if (delold)
					free(chan);
				chan = strdup(to);
				delold=1;
			} else
		     	if (!strcasecmp(command,"me")) {
			   	sendChan(channel, SRVMSG, 5,  "PRIVMSG ", chan, " :\1ACTION ", params, "\1");
				sendChan(channel, SCRMSG, "\x1b[1;33m~\x1b[0m %s %s", nick, params);
			} else
		     	if (!strcasecmp(command,"msg")) {
				to = strsep(&params," ");
				params = strsep(&params, "");
			   	sendChan(channel, SRVMSG, 4,"PRIVMSG ", to, " :", params);
			} else
		     	if (!strcasecmp(command,"notice")) {
				to = strsep(&params," ");
				params = strsep(&params, "");
			   	sendChan(channel, SRVMSG, 4, "NOTICE ", to, " :", params);
			} else
			sendChan(channel, SRVMSG, 3, command, " :", params);
		}
		else {
		   	sendChan(channel, SRVMSG, 4, "PRIVMSG ", chan, " :", command);
			sendChan(channel, SCRMSG, "\x1b[1;33m<%s>\x1b[0m %s", nick, command);
		}
	}
}

void getwhat(char *params,char **what) {
	*what = params;
	if (**what == ':')
		(*what)++;
}

void getwhowhat(char *params, char **who, char **what) {
	int i;
	
	*who = strsep(&params," ");
	i = strspn(params," :");
	*what = params+i;
}

int docommand(char *from, char *command, char *params, char **who, char **what) {
	char *ctcp;
   	
   	if (!strcmp(command,"QUIT")){
     	       	getwhat(params, what);
	   	return QUIT;
	} else
     	if (!strcmp(command,"NICK")) {
	   	getwhat(params, what);
	  	return NICK;
	} else
	if (!strcmp(command,"PART")) {
	   	getwhat(params, what);
	   	return PART;
	} else
	if (!strcmp(command,"JOIN")) {
		getwhat(params, what);
		return JOIN;
	} else
	if (!strcmp(command,"NOTICE")) {
		getwhowhat(params, who, what);
		return NOTICE;
	} else
	if (!strcmp(command,"MODE")) {
		getwhowhat(params, who, what);
		return MODE;
	} else
	if (!strcmp(command,"PRIVMSG")) {
		getwhowhat(params, who, what);
		ctcp = *what;
		if (*ctcp == 1) {
			ctcp++;
			if (!strncmp(ctcp,"VERSION",7)) {
				sendChan(channel, SRVMSG, 3,"NOTICE ", from, " :\1VERSION " VERSIONREPLY "\1");
			   	sendChan(channel, SCRMSG, "Someone versioned us :%s", from);
				return IGNORE;
			} else
		   	if (!strncmp(ctcp,"ACTION",6)) {
			   	*what = ctcp + 7;
			   	return ME;
			} else 
			if (!strncmp(ctcp,"PING",4)) {
				sendChan(channel, SRVMSG, 4,"NOTICE ", from, " :\1PING ", ctcp+5);
			   	return IGNORE; 
			} else
			if (!strncmp(ctcp,"DCC",3)) {
				//badness... auto receive the DCC. 
				newThread(receivedcc,STACK_DFL,strdup(ctcp));
				return DCC;	
			} else { 
				*what = ctcp;
				return UNKCTCP;
			}
		}
		return NORMAL;
	} else
	if (!strcmp(command,"PING")) {
		sendChan(channel, SRVMSG, 2,"PONG ",params);
		return IGNORE;
	}
	*what = params;
	return NORMAL;
}

int process(FILE *stream) {
	int done,type;
	char *upto,*from, *what, *temp, *command, *params, *who, *host;
        char *filename,*filesize;
	static char *lineptr=NULL;
	static int linesz=0;
	
	if (getline(&lineptr,&linesz,stream) == -1)
		return 0;
	
	/* get first word */
	
	upto = lineptr;
	if (*lineptr == ':') {
		from = strsep(&upto," ");
		from++;
	} else 
		from = "*";
	command = strsep(&upto," ");
	params = strsep(&upto,"\n\r");
	if (temp = strchr(from,'!')) {
		host = temp+1;
		*temp='\0';
	} 
	if (isdigit(*command)) {
		from = "*";
		type = SERVER;
		getwhowhat(params,&who,&what);
	} else 
		type = docommand(from,command,params,&who,&what);

	switch (type) {
		case ME:
		case NORMAL:
		case NOTICE:
			if (strcasecmp(who,chan)) {
				sendChan(channel, SCRMSG2, "\x1b[0m- ");
			}
			break;
		default:
			break;
	};
	switch (type) {
		case NORMAL:
			sendChan(channel, SCRMSG, "\x1b[1;33m<%s>\x1b[0m %s", from, what);
			break;
		case ME:
			sendChan(channel, SCRMSG, "\x1b[35m* %s %s", from, what);
			break;
		case JOIN:
			sendChan(channel, SCRMSG, "\x1b[34m%s (%s) has joined channel %s",from,host,what);
			break;
	        case PART:
	     		sendChan(channel, SCRMSG, "\x1b[34m%s (%s) has left channel %s", from,host,what);
	   		break;
		case QUIT:
	   		sendChan(channel, SCRMSG, "\x1b[1;35m%s has quit IRC (%s)", from,what);
	   		break;
	 	case NICK:
	   		if (!strcmp(from,nick)) {
				if (delnick)
					free(nick);
				nick = strdup(what);
				delnick=1;
			}
			sendChan(channel, SCRMSG, "\x1b[35m** %s \x1b[32m<->\x1b[35m %s", from,what);
	   		break;
		case MODE:
			sendChan(channel, SCRMSG, "\x1b[35m%s sets mode %s",from,what);
			break;
		case SERVER:
			sendChan(channel, SCRMSG, "\x1b[1;33m** %s",what);
			break;
		case NOTICE:
			sendChan(channel, SCRMSG, "\x1b[37m-%s- \x1b[1;33m%s",from,what);
			break;
		case UNKCTCP:
			sendChan(channel, SCRMSG, "\x1b[1;31mUnknown CTCP from %s : %s",from,what);
			break;
                case DCC:
                        //point filename to "DCC"
			filename = strsep(&what," ");
                        filename = what;

                        //point filename to "SEND"
			filename = strsep(&what," ");
                        filename = what;

                        //point filename to "filename"
			filename = strsep(&what," ");

                        filesize = what;
			//point filesize to "32bit IP number"
			filesize = strsep(&what," ");
			filesize = what;

			//point filesize to "IP PORT"
			filesize = strsep(&what," ");
			filesize = what;

			//point filesize to "filesize"
			filesize = strsep(&what," ");

			sendChan(channel, SCRMSG, "\x1b[37mReceiving DCC from %s : filename: %s  size: %s", from,filename,filesize);
			break;
		default:
			break;
	}
	return 1;
	
}

void main(int argc, char *argv[]) {
	FILE *stream;
	char *server = argv[1];
	int i;
	
	if (argc>1) {
		printf("Welcome to A-Jirc V1.0\n");
		if (!strchr(server, '/')) {
			char *newserve = malloc(strlen(server)+16);
			sprintf(newserve, "/dev/tcp/%s:6667", server);
			server = newserve;
		}
		stream = fopen(server, "r");
		if (!stream) {
			perror("AJirc");
			exit(1);
		}
		if (argc>2)
			nick = argv[2];
		con_init();
		printf("Connected to server\n");
		con_update();
		con_setscroll(0,con_ysize-2);
		con_gotoxy(0,con_ysize-2);
		printf("Ajirc V1.0 (c) J Maginnis & A Greenwood");
		for (i=39;i<con_xsize;i++) {
			putchar('-');
		}
		channel = makeChan();
		globsock = dup(fileno(stream));
		newThread(fromUser,STACK_DFL,NULL);
		newThread(outThread,256,&globsock);
		while (process(stream));
		printf("Connection closed by remote host!\n");
		con_end();
                printf("\x1b[0m\x1b[H\x1b[2J");
		exit(1);
	}
	printf("\x1b[0m\x1b[H\x1b[2J");
}

