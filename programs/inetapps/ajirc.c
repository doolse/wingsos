#include <fcntl.h>
#include <stdio.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <winlib.h>
#include <errno.h>
#include <stdarg.h>
#include "ajirc.h"
extern char *getappdir();

#define CMD_ABOUT	0x1000
#define CMD_SERVER	0x1001

MenuData helpmenu[]={
{"About", 0, NULL, 0, CMD_ABOUT, NULL, NULL},
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

MenuData servers[11]; //Max of 10, plus the null struct at the end.
                      //Now dynamically created in main();

MenuData filemenu[]={
{"Connect to", 0, NULL, 0, 0, NULL, servers},
{"Exit", 0, NULL, 0, CMD_EXIT, NULL, NULL},
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

MenuData themenu[]={
{"Server", 0, NULL, 0, 0, NULL, filemenu}, 
{"Help", 0, NULL, 0, 0, NULL, helpmenu}, 
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

int channel;
char *nick="Ajirc";
int delold=0,delnick=0;

void *App;
void *window1,*text1, *butcon;
IRCChan *statuswin;
IRCChan *headchan=NULL;
IRCChan *curchan;

char buf[1024];
int sockfd=-1;
int linesz;
char *curserver;
int haveread;
int regis;


IRCChan *findwin(char *str) {
	IRCChan *chanup;
	
	if ((chanup = headchan) != NULL) {
		do {
			if (!strcasecmp(chanup->name,str)) {
				return chanup;
			} else chanup = chanup->next;
		} while (chanup != headchan);
	}
	return statuswin;
}
	
void errmsg() {
	if (haveread) {
		doscrmsg(statuswin, "Disconnected! %s", strerror(errno));
	} else {
		doscrmsg(statuswin, "Unable to connect! %s", strerror(errno));		
	}
	close(sockfd);
	sockfd = -1; 
}

void send2server(char *fmt, ...) {
	va_list args;
	int done;

	va_start(args, fmt);
	if (sockfd != -1) {
		vsprintf(buf, fmt, args);
		done = strlen(buf);
		buf[done] = '\n';
		done = write(sockfd, buf, done+1);
		if (done == -1)
			errmsg();
		else if (!haveread) {
			doscrmsg(statuswin, "Connected!");
			haveread=1;
		}
	}
}

void doscrmsg(IRCChan *win, char *fmt, ...) {
	va_list args;
	void *temp;
	
	va_start(args, fmt);
	vsprintf(buf, fmt, args); 					
	temp = win->txtarea;
	JTxtAppend(temp, buf);  
	JTxtAppend(temp, "\n");
	if (!win->changed && win != curchan) {
		win->changed = 1;
		JWinSetPen(win->button, COL_Red);
		JWinReDraw(win->button);
	}
}

void opencon(char *server) {
	char *servername;
	
	if (sockfd != -1) {
		close(sockfd);
		doscrmsg(statuswin, "Closing connection to %s!", curserver);
	}
	haveread = 0;
	regis = 0;
	curserver = server;
	linesz = 0;
	doscrmsg(statuswin, "Attempting connection at %s",server);
	servername = malloc(strlen(server)+strlen("/dev/tcp/:6667")+1);
	strcpy(servername,"/dev/tcp/");
	strcat(servername,server);
	strcat(servername,":6667");
	sockfd = open(servername, O_READ|O_WRITE|O_NONBLOCK);
	free(servername);
	if (sockfd == -1) {
		doscrmsg(statuswin, "Unable to connect to %s!", server);
		return;
	}
	askNotify(sockfd, channel, IO_NFYWRITE, NULL);
}


void doservecom(int type, char *str) {
	IRCChan *text=statuswin;
	char *chan;
	char *nicks,*anick;
	int msg=0;
	
	switch(type) {
	case RPL_NAMREPLY:
		strsep(&str," ");
		chan = strsep(&str," ");
		text = findwin(chan);
		anick = strsep(&str, "\n");
		getwhat(anick, &nicks);
		if (!text->hasnames)
			while (anick = strsep(&nicks," ")) {
				JLstInsert(text->nicklist, anick, NULL, NULL);
			}
		else {
			str = nicks;
			msg = 1;
		}
		break;
	case RPL_ENDOFNAMES:
		chan = strsep(&str," ");
		str = strsep(&str,"\n");
		text = findwin(chan);
		if (text == statuswin || text->hasnames)
			msg = 1;
		else
			text->hasnames=1;
		break;
	default:
		msg = 1;
		break;
	}
	if (msg)
		doscrmsg(text, "** %s", str);
}

void switchto(IRCChan *temp) {	
	if (temp != curchan) {
		JWinHide(curchan->txtarea);
		if (curchan->nicklist)
			JWinHide(curchan->nicklist);
		JWinShow(temp->txtarea);
		if (temp->nicklist)
			JWinShow(temp->nicklist); 
		if (temp->changed) {
			JWinSetPen(temp->button, COL_Black);
			JWinReDraw(temp->button);
			temp->changed = 0;
		} 
		curchan = temp;
	}
}

void chanclick(void *widget) {
	switchto((IRCChan *) JWinGetData(widget));
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

void strip(char *upto) {
	char *out = upto;
	int ch;
	int blank = 0;
	
	while (ch = *upto) {
		if (blank)
			*out = ' ';
		else *out = ch;
		upto++;
		if (ch == 3) {
			if (*upto == '0' && !isdigit(*(upto+1))) {
				blank = 1;
			} else blank = 0;
			while (*upto == ',' || isdigit(*upto))
				upto++;
		} else out++;
	}
	*out = 0;
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
		*who = *what;
	   	return PART;
	} else
	if (!strcmp(command,"JOIN")) {
		getwhat(params, what);
		*who = *what;
		if (!strcmp(from,nick)) {
			opennew(*what);
		}
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
		strip(*what);
		ctcp = *what;
		if (!strcasecmp(*who,nick)) {
			*who = from;
			if (findwin(from) == statuswin)
				opennew(from);
		}
		if (*ctcp == 1) {
			ctcp++;
			if (!strncmp(ctcp,"VERSION",7)) {
				send2server("NOTICE %s :\1VERSION " VERSIONREPLY "\1", from);
			   	doscrmsg(curchan, "Someone versioned us :%s", from);
				return IGNORE;
			} else
		   	if (!strncmp(ctcp,"ACTION",6)) {
			   	*what = ctcp + 7;
			   	return ME;
			} else 
			if (!strncmp(ctcp,"PING",4)) {
				send2server("NOTICE %s :\1PING %s", from, ctcp+5);
			   	return IGNORE; 
			} else { 
				*what = ctcp;
				return UNKCTCP;
			}
		}
		return NORMAL;
	} else
	if (!strcmp(command,"PING")) {
		send2server("PONG %s",params);
		regis = 1;
		return IGNORE;
	}
	*what = params;
	return NORMAL;
}

void doline(char *lineptr) {
	int done,type;
	char *upto,*from, *what, *temp, *command, *params, *who, *host;
	IRCChan *text;
	int servenum;
	
	/* get first word */
	
	
//	doscrmsg(statuswin, "S-%s", lineptr);
	who = "";
	upto = lineptr;
	if (*lineptr == ':') {
		from = strsep(&upto," ");
		from++;
	} else from = "*";
	command = strsep(&upto," ");
	params = strsep(&upto,"");
	if (temp = strchr(from,'!')) {
		host = temp+1;
		*temp='\0';
	} 
	servenum = 0;
	if (isdigit(*command)) {
		from = "*";
		type = SERVER;
		servenum = atoi(command);
		getwhowhat(params,&who,&what);
		doservecom(servenum, what);
		return;
	} else type = docommand(from,command,params,&who,&what);
	text = findwin(who);
	
	switch (type) {
		case NORMAL:
			doscrmsg(text, "<%s> %s", from, what);
			break;
		case ME:
			doscrmsg(text, "* %s %s", from, what);
			break;
		case JOIN:
			doscrmsg(text, "%s (%s) has joined channel %s",from,host,what);
			break;
	        case PART:
	     		doscrmsg(text, "%s (%s) has left channel %s", from,host,what);
	   		break;
		case QUIT:
	   		doscrmsg(text, "%s has quit IRC (%s)", from,what);
	   		break;
	 	case NICK:
	   		if (!strcmp(from,nick)) {
				if (delnick)
					free(nick);
				nick = strdup(what);
				delnick=1;
			}
			doscrmsg(text, "** %s <-> %s", from,what);
	   		break;
		case MODE:
			doscrmsg(text, "%s sets mode %s",from,what);
			break;
		case SERVER:
			doscrmsg(text, "** %s", what);
			break;
		case NOTICE:
			doscrmsg(text, "-%s- %s",from,what);
			break;
		case UNKCTCP:
			doscrmsg(text, "Unknown CTCP from %s : %s",from,what);
			break;
		default:
			break;
	}
}

void process() {
	int done,ch;
	static char line[1024];
	static char inbuf[512];
	char *upto;
	
	if (sockfd == -1)
		return;
	while (1) {
		done = read(sockfd, inbuf, 512);
		if (done == -1) {
			if (errno == EAGAIN) {
				askNotify(sockfd, channel, IO_NFYREAD, NULL);
				return;
			}
			errmsg();
			return;
		}
		if (!done) {
			doscrmsg(statuswin, "Disconnected from %s", curserver);
			close(sockfd);
			sockfd = -1;
			return;
		}
		upto = inbuf;
		haveread = 1;
		while (done--) {
			ch = *upto;
			++upto;
			switch(ch) {
			case 10:
				line[linesz] = 0;
				doline(line);
				linesz = 0;
				break;
			case 13:
				break;
			default:
				if (linesz<1023) {
					line[linesz] = ch;
					linesz++;
				}
				break;
			}
		}
	}
}

void outThread(int *tlc) {
	int RcvId;
	char *msg;
	int type;
	
	while (1) {
		RcvId = recvMsg(channel, (void *)&msg);
		type = * (int *)msg;
		switch (type) {
		case WIN_EventRecv:
			JAppDrain(App);
			break;
		case IO_NFYWRITE: 
			send2server("USER " IDENT " some thing :" NAME "\nNICK %s", nick);
		case IO_NFYREAD:
			process();
			break;
		}
		replyMsg(RcvId,0);
	}
}

void opennew(char *name) {
	IRCChan *newchan;
	int fromright;
	void *temp;

	newchan = findwin(name);
	if (newchan == statuswin) {
		newchan = malloc(sizeof(IRCChan));
		headchan = addQueue(headchan, headchan, newchan);
		newchan->name = strdup(name);
		if (newchan->name[0] != '#') {
			fromright = 0;
			newchan->nicklist = NULL;
		} else {
			fromright = 60;
			temp = JLstInit(NULL, window1, 0);
			JWinGeom(temp, 60, 0, 0, 32, GEOM_TopRight | GEOM_BotRight2);
			JWinSetBack(temp,COL_Black);
			JWinSetPen(temp,COL_MedGrey);
			newchan->nicklist = temp;
		}
		temp = JTxtInit(NULL, window1, 0, "");
		JWinGeom(temp, 0, 0, fromright, 32, GEOM_TopLeft | GEOM_BotRight2);
		JWinSetBack(temp,COL_White);
		JWinSetPen(temp,COL_Black);
		newchan->txtarea = temp;		
		temp = JButInit(NULL, butcon, 0, newchan->name);
		JWinSetData(temp, newchan);
		JWinCallback(temp, JBut, Clicked, chanclick);
		JWinShow(temp);
		newchan->button = temp;
		newchan->changed = 0;
		newchan->hasnames = 0;
		JWinSelect(text1);
		if (newchan->name[0] == '#') {
			switchto(newchan);
		}
	}
}

void mainmenu(void *Self, MenuData *item) {
	switch(item->command) {
	case CMD_ABOUT:
		doscrmsg(curchan, "Ajirc (c) An Greenwood and Jolse Maginnis");
		break;
	case CMD_EXIT:
		exit(1);
		break;
	case CMD_SERVER:
		opencon(item->name);
		break;
	}
	
}

void fromUser(void *widget, int type) {
	char *command,*upto,*params,*to,*line;
		
	line = strdup(JTxfGetText(widget));
	JTxfSetText(widget,"");
	command = line;
	if (*command == '/') {
		upto = ++command;
		command = strsep(&upto," ");
		params = strsep(&upto,"\n\r");
		
	     	if (!strcasecmp(command,"nick")) {
			if (!regis)
				nick = strdup(params);
			send2server("NICK %s", params);
		} else
	     	if (!strcasecmp(command,"server")) {
			opencon(strdup(params));
                } else
                if (!strcasecmp(command,"topic")) {
                  if(params == NULL || params == "")
                    send2server("TOPIC %s :", curchan->name);
                  send2server("TOPIC %s :%s", curchan->name, params);
                } else
                if (!strcasecmp(command,"quote")) {
                  send2server("%s", params);
                } else
                if (!strcasecmp(command, "op")) {
                  send2server("MODE %s +o %s", curchan->name, params);
                } else
                if(!strcasecmp(command, "deop")) {
                  send2server("MODE %s -o %s", curchan->name, params);
		} else
	     	if (!strcasecmp(command,"me")) {
		   	send2server("PRIVMSG %s :\1ACTION %s\1", curchan->name, params);
			doscrmsg(curchan, "* %s %s", nick, params);
		} else
	     	if (!strcasecmp(command,"msg")) {
			to = strsep(&params," ");
			params = strsep(&params, "");
		   	send2server("PRIVMSG %s :%s", to, params);
		} else
		if (!strcasecmp(command,"notice")) {
			to = strsep(&params," ");
			params = strsep(&params, "");
		   	send2server("NOTICE %s :%s", to, params);
		} else
		if (!strcasecmp(command,"part") || !strcasecmp(command,"names")) {
			if (params != NULL)
				to = params;
			else to = curchan->name;
		   	send2server("%s %s", command, to);
		} else {
			if (params == NULL)
				params = "";
			if (strchr(params, ' '))
				send2server("%s :%s", command, params); 
			else send2server("%s %s", command, params); 
		}
	}
	else {
	   	send2server("PRIVMSG %s :%s", curchan->name, command);
		doscrmsg(curchan, "<%s> %s", nick, command);
	}
	free(line);
}


void RightClick(void *Self, int Type, int X, int Y, int XAbs, int YAbs) {
	void *temp;
	temp = JMnuInit(NULL, NULL, themenu, XAbs, YAbs, mainmenu);
	JWinShow(temp);
}	

int main(int argc, char *argv[]) {
	void *temp;
	int i;
        FILE * sf;
        int j;
        char *buf = NULL;
        int size = 0;	
        uint numofservers;
        char * path = NULL;
	MenuData *server;
/*
MenuData servers[]={
{"southern.oz.org", 0, NULL, 0, CMD_SERVER, NULL, NULL},
{"192.168.0.1", 0, NULL, 0, CMD_SERVER, NULL, NULL},
{"irc.stealth.net:6665", 0, NULL, 0, CMD_SERVER, NULL, NULL},
{NULL, 0, NULL, 0, 0, NULL, NULL}
};
*/
        path = fpathname("resources/serverlist.rc", getappdir(), 1);

        sf = fopen(path, "r");

        if(sf) {
          getline(&buf, &size, sf);
          numofservers = atoi(buf);

          if(numofservers > 10)
            numofservers = 10;

	  server = servers;
          for(j=0;j<numofservers;j++) {

            getline(&buf, &size, sf);
            buf[strlen(buf)-1] = 0;
            server->name    = buf;
            buf  = NULL;
            size = 0;

            server->shortcut = 0;
            server->icon     = NULL;
            server->flags    = 0;
            if(server->name[0] == '-')
              server->command  = 0;
            else
              server->command  = CMD_SERVER;
            server->data     = NULL;
            server->submenu  = NULL;
	    server++;
          }
          server->name     = NULL;
          fclose(sf);
        }

	channel = makeChan();
	App = JAppInit(NULL, channel);
	window1 = JWndInit(NULL, NULL, 0, "Ajirc V1.0 (c) A.G. & J.M.", JWndF_Resizable);
	JWinCallback(window1, JWnd, RightClick, RightClick);

   	JWinGeom(window1, 16, 16, 16, 16, GEOM_TopLeft | GEOM_BotRight2);
	JAppSetMain(App, window1);
	
	temp = JTxtInit(NULL, window1, 0, "");
	JWinGeom(temp, 0, 0, 0, 32, GEOM_TopLeft | GEOM_BotRight2);
	JWinSetBack(temp, COL_White);
	JWinSetPen(temp, COL_Black); 

	statuswin = malloc(sizeof(IRCChan));
	statuswin->name = "Status";
	statuswin->txtarea = temp;
	statuswin->nicklist = NULL;
	statuswin->changed = 0;
	statuswin->hasnames = 1;
	curchan = statuswin;
	
	butcon = JCntInit(NULL, window1, 0, 0);
	JWinGeom(butcon, 0, 16, 0, 16, GEOM_BotLeft | GEOM_TopRight2);

	temp = JButInit(NULL, butcon, 0, "Status");
	JWinSetData(temp, statuswin);
	JWinCallback(temp, JBut, Clicked, chanclick);	
	statuswin->button = temp;
	
	text1 = JTxfInit(NULL, window1, 0, "");
	JWinGeom(text1, 0, 32, 0, 16, GEOM_BotLeft | GEOM_TopRight2);
	JWinCallback(text1, JTxf, Entered, fromUser);
	JWinShow(window1);
	newThread(outThread,0x400,NULL);
	retexit(0);
	return -1;
}

