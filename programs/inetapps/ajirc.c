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

enum menucommands {
  CMD_ABOUT=0x1000,
  CMD_SERVER,
  CMD_BGCOLOUR,
  CMD_HELP,
  CMD_NICKWINDOW,
  CMD_SAVEAS
};

unsigned char app_icon[] = {
28,34,89,185,144,152,77,38,
0,0,0,128,128,128,0,0,
28,32,36,26,2,3,0,0,
0,28,60,99,96,99,60,28,
0x01,0x01,0x01,0x01
};

MenuData servers[]={
{"melbourne.oz.org", 0, NULL, 0, CMD_SERVER, NULL, NULL},
{"192.168.0.1", 0, NULL, 0, CMD_SERVER, NULL, NULL},
{"irc.stealth.net:6665", 0, NULL, 0, CMD_SERVER, NULL, NULL},
{"efnet.demon.co.uk:6665", 0, NULL, 0, CMD_SERVER, NULL, NULL},
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

MenuData bgcolours[]={
{"White" ,0,NULL,0,CMD_BGCOLOUR,NULL,NULL},
{"Yellow",0,NULL,0,CMD_BGCOLOUR,NULL,NULL},
{"Green", 0,NULL,0,CMD_BGCOLOUR,NULL,NULL},
{"Cyan"  ,0,NULL,0,CMD_BGCOLOUR,NULL,NULL},
{"Pink"  ,0,NULL,0,CMD_BGCOLOUR,NULL,NULL},
{"Purple",0,NULL,0,CMD_BGCOLOUR,NULL,NULL},
{"Blue",  0,NULL,0,CMD_BGCOLOUR,NULL,NULL},
{NULL,    0, NULL, 0, 0, NULL, NULL}
};

MenuData helpmenu[]={
{"About",    0, NULL, 0, CMD_ABOUT, NULL, NULL},
{"Commands", 0, NULL, 0, CMD_HELP,  NULL, NULL},
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

MenuData filemenu[]={
{"Connect to", 0, NULL, 0, 0, NULL, servers},
{"Exit",       0, NULL, 0, CMD_EXIT, NULL, NULL},
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

MenuData themenu[]={
{"Server", 0, NULL, 0, 0, NULL, filemenu}, 
{"Help", 0, NULL, 0, 0, NULL, helpmenu}, 
{"Highlight", 0, NULL, 0, 0, NULL, bgcolours}, 
{"Nick List", 0, NULL, 0, CMD_NICKWINDOW, NULL, NULL},
{"Save As...",0, NULL, 0, CMD_SAVEAS, NULL, NULL},
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

int channel;
char *nick="Ajirc";
int delold=0,delnick=0;

void *App;
void *window1,*text1, *butcon, *txtcard, *nickwindow, *nickcard;
IRCChan *statuswin;
IRCChan *headchan=NULL;
IRCChan *curchan;

char buf[1024];
int sockfd=-1;
int linesz;
char *curserver;
int haveread;
int regis;

void doscrmsg(IRCChan *win, char *fmt, ...);
void getwhat(char *params,char **what);
void opennew(char *name);
void closechan(IRCChan * thechan);
void savechatbuffer(char * filename,int append);

void savechatwithname(void * widget) {
  void * wnd,*txtinput,*chkbox;
  int append;
  char * pathname;

  if(!strcmp(((JBut*)widget)->Label,"Save As..."))
    txtinput = JWGetData(widget);    
  else
    txtinput = widget;

  chkbox = JWGetData(txtinput);
  wnd    = JWGetData(chkbox);

  pathname = strdup(JTxfGetText(txtinput));
  if(strlen(pathname) == 0) {
    free(pathname);
    return;
  }

  append = ((JChk*)chkbox)->Status;
  JWKill(wnd);
  savechatbuffer(pathname,append);
}

void savechatbuffer(char * filename,int append) {
  void * wnd,*savebut,*txtinput,*cnt,*chkbox;
  JMeta * metadata;
  FILE * outfp;
  Piece * pptr;

  if(!filename) {
    metadata = malloc(sizeof(JMeta));
    metadata->showicon = 0;
    metadata->parentreg = ((JW*)window1)->RegID;
    wnd = JWndInit(NULL,"Save the Log?",0,metadata);
    cnt = JCntInit(NULL);
    ((JCnt*)cnt)->Orient = JCntF_LeftRight;
    JCntAdd(wnd,cnt);

    txtinput = JTxfInit(NULL);
    JWSetAll(txtinput,48,16);
    JWinCallback(txtinput,JTxf,Entered,savechatwithname);

    savebut = JButInit(NULL,"Save As...");
    JWSetData(savebut,txtinput);
    JWinCallback(savebut,JBut,Clicked,savechatwithname);
    
    JCntAdd(cnt,savebut);
    JCntAdd(cnt,txtinput);
  
    chkbox = JChkInit(NULL,"Append?");
    JCntAdd(wnd,chkbox);
    JWSetData(txtinput,chkbox);
    JWSetData(chkbox,wnd);

    JWSetBounds(wnd,24,24,104,24);
    JWndSetProp(wnd);
    JWinShow(wnd);
    JWReqFocus(txtinput);
    return;
  }

  if(append)
    outfp = fopen(filename,"a");
  else
    outfp = fopen(filename,"w");

  if(outfp) {
    pptr = ((JTxt*)(curchan->txtarea))->LineTab->PiecePtr;
    fprintf(outfp,"%s",pptr->buffer);
    while(!pptr->Last) {
      pptr = pptr->Next;
      fprintf(outfp,"%s",pptr->buffer);
    }
    fclose(outfp);
  }
}

void expand() {
  return;
}

void * makenicklist(OurModel * RootModel) {
  void *scr;
  JTree *tree;
  JTModel *Model;
  OurModel *cur;

  Model = JTModelInit(NULL, (DefNode *)RootModel, (TreeExpander)expand);
  tree  = JTreeInit(NULL, (TModel *)Model);
	
  JTreeAddColumns(tree, NULL, 
		        "Name", OFFSET(OurModel, name), 64, JColF_STRING, 
			NULL); 

  scr = JScrInit(NULL, tree, 0);
  JWSetData(scr,Model);

  JCntAdd(nickcard,scr);
  JWinLayout(nickcard);
  return(scr);
}

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
		JWSetPen(win->button, COL_Red);
		JWReDraw(win->button);
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
	JTModel *Model;
	OurModel *cur, *RootModel;
	
	switch(type) {
	case RPL_NAMREPLY:
		strsep(&str," ");
		chan = strsep(&str," ");
		text = findwin(chan);
		anick = strsep(&str, "\n");
		getwhat(anick, &nicks);
		if (!text->hasnames) {
		  if(text->nicklist) {
		    RootModel = text->RootModel;
		    Model     = JWGetData(text->nicklist);
		    while (anick = strsep(&nicks," ")) {
		      cur = calloc(1, sizeof(OurModel));
		      cur->name = strdup(anick);
		      JTModelAppend(Model, (DefNode *)RootModel, (DefNode *)cur);
		    }
		    JWinHide(text->nicklist);
		    JWinShow(text->nicklist);
       	            JWReDraw(text->nicklist);
		    JWReDraw(nickcard);
		  }
		} else {
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
		JWinHide(curchan->pane);
		JWinShow(temp->pane);
		JWinLayout(txtcard);

		if(curchan->nicklist)
		  JWinHide(curchan->nicklist);
		if(temp->nicklist) {
		  JWinShow(temp->nicklist);
                  JWReDraw(temp->nicklist);
		  JWinLayout(nickcard);
		  JWReDraw(nickcard);
		}

                JWSetPen(curchan->button, COL_DarkGrey);
                JWReDraw(curchan->button);

		JWSetPen(temp->button, COL_Black);
		JWReDraw(temp->button);

		if (temp->changed) {
			temp->changed = 0;
		} 
		curchan = temp;
	}
}

void chanclick(void *widget) {
	switchto((IRCChan *) JWGetData(widget));
	JWReqFocus(text1);
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
	uint i;
	char *upto,*from, *what, *temp, *command, *params, *who, *host;
	IRCChan *text;
	int servenum;
	OurModel * cur;
	JTModel * Model;
	Vec * chvec;
	/* get first word */
	
	
//	doscrmsg(statuswin, "S-%s", lineptr);
	who = "";
	upto = lineptr;
	if (*lineptr == ':') {
		from = strsep(&upto," ");
		from++;
	} else 
		from = "*";
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
	} else 
		type = docommand(from,command,params,&who,&what);
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
			if(text->nicklist) {
			  if(strcmp(from,nick)) {
			    Model = JWGetData(text->nicklist);
			    cur = calloc(1, sizeof(OurModel));
			    cur->name = strdup(from);
			    JTModelAppend(Model, (DefNode *)(text->RootModel), (DefNode *)cur);
			    JWinHide(text->nicklist);
			    JWinShow(text->nicklist);
        	            JWReDraw(text->nicklist);
			    JWReDraw(nickcard);
		  	  }
			}
			break;
	        case PART:
	     		doscrmsg(text, "%s (%s) has left channel %s", from,host,what);
			if(text->nicklist) {
			  chvec = ((DefNode*)(text->RootModel))->Children;
			  for(i=0;i<VecSize(chvec);i++) {
			    cur = VecGet(chvec,i);
			    if(!strcmp(cur->name,from)) {
			      JTModelRemove(NULL, (DefNode *)cur);
			      JWinHide(text->nicklist);
			      JWinShow(text->nicklist);
        	              JWReDraw(text->nicklist);
			      JWReDraw(nickcard);
			      break;
			    }
			  }
			}
			if(!strcmp(from,nick))
                        	closechan(text);
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

/* open new window */

void opennew(char *name) {
	IRCChan *newchan;
	int fromright;
	void *temp,*scr;

	newchan = findwin(name);
	if (newchan == statuswin) {
		newchan = calloc(sizeof(IRCChan), 1);
		headchan = addQueue(headchan, headchan, newchan);
		newchan->name = strdup(name);
		if (newchan->name[0] != '#') {
			fromright = 0;
			newchan->nicklist = NULL;
		} else {
			newchan->RootModel = calloc(1,sizeof(OurModel));
			newchan->nicklist = makenicklist(newchan->RootModel);
		}
		temp = JTxtInit(NULL);
		scr = JScrInit(NULL, temp, 0);
		JCntAdd(txtcard, scr);
		JWSetBack(temp,COL_White);
		JWSetPen(temp,COL_Black);
		newchan->txtarea = temp;
		newchan->pane = scr;
		
		temp = JButInit(NULL, newchan->name);
		JCntAdd(butcon, temp);
		JWSetData(temp, newchan);
		JWinCallback(temp, JBut, Clicked, chanclick);
		
		JWinShow(temp);
		newchan->button = temp;
		newchan->changed = 0;
		newchan->hasnames = 0;
		JWReqFocus(text1);
		if (newchan->name[0] == '#') {
			switchto(newchan);
		}
		JWinLayout(window1);
		JWinLayout(txtcard);
		JWinLayout(butcon);
	}
}

void closechan(IRCChan * thechan) {
  if(thechan == curchan)
    switchto(thechan->next);

  headchan = remQueue(headchan,thechan);

  JCntRemove(txtcard,thechan->pane);
  JCntRemove(butcon,thechan->button);
  if(thechan->nicklist)
    JCntRemove(nickcard,thechan->nicklist);

  JWKill(thechan->txtarea);
  JWKill(thechan->pane);
  JWKill(thechan->button);
  if(thechan->nicklist)
    JCntRemove(nickcard,thechan->nicklist);
  free(thechan->name);
  free(thechan);

  JWinLayout(window1);
  JWinLayout(txtcard);
  JWinLayout(butcon);
  JWinLayout(nickwindow);
  JWinLayout(nickcard);

  JWReqFocus(text1);
}

void mainmenu(void *Self, MenuData *item) {
int colourid;

	if(item->command == CMD_BGCOLOUR) {
		if(item->name[0] == 'W')
			colourid = COL_White;
		else if(item->name[0] == 'C')
			colourid = COL_Cyan;
		else if(item->name[0] == 'G')
			colourid = COL_LightGreen;
		else if(item->name[0] == 'Y')
			colourid = COL_Yellow;
                else if(item->name[0] == 'B')
			colourid = COL_LightBlue;
                else if(item->name[0] == 'P') {
			if(item->name[1] == 'u')
				colourid = COL_Purple;
			else
				colourid = COL_Pink;
		} 
        }

	switch(item->command) {
	case CMD_ABOUT:
		doscrmsg(curchan, "Ajirc (c) An Greenwood and Jolse Maginnis");
		break;
	case CMD_HELP:
		doscrmsg(curchan, "/join channelname");
		doscrmsg(curchan, "/part channelname");
		doscrmsg(curchan, "/names");
		doscrmsg(curchan, "/nick newnickname");
		doscrmsg(curchan, "/op nickname");
		doscrmsg(curchan, "/deop nickname");
		doscrmsg(curchan, "/whois nickname");
		doscrmsg(curchan, "/msg nickname private message");
		doscrmsg(curchan, "/me message");
		doscrmsg(curchan, "/server domain:port");
		doscrmsg(curchan, "/quit");
		break;
	case CMD_EXIT:
		exit(1);
		break;
	case CMD_SERVER:
		opencon(item->name);
		break;
	case CMD_BGCOLOUR:
		JWSetBack(curchan->txtarea, colourid);
		JWReDraw(curchan->txtarea);
		break;
	case CMD_NICKWINDOW:
		if(((JW*)nickwindow)->HideCnt) {
		  JWinShow(nickwindow);
		} else {
		  JWinHide(nickwindow);
		  JWinShow(nickwindow);
		}
		break;	
	case CMD_SAVEAS:
		savechatbuffer(NULL,0);
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
			else 
				to = curchan->name;
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
	temp = JMnuInit(NULL, 0, themenu, XAbs, YAbs, mainmenu);
	JWinShow(temp);
}	

int main(int argc, char *argv[]) {
	void *temp,*scr;
	int i, j, numofservers;
        FILE * sf;
        char *buf = NULL;
        int size = 0;	
        char * path = NULL;
        JMeta * metadata;
	MenuData *server;

	channel = makeChan();
	App = JAppInit(NULL, channel);

	metadata = malloc(sizeof(JMeta));
   	metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
	metadata->title = "AJirc";
	metadata->icon = app_icon;
	metadata->showicon = 1;
	metadata->parentreg = -1;

	window1 = JWndInit(NULL, "Ajirc V1.0 (c) A.G. & J.M.",JWndF_Resizable,metadata);
	JWinCallback(window1, JWnd, RightClick, RightClick);

	metadata = malloc(sizeof(JMeta));
	metadata->title = "Names List";
	metadata->showicon = 0;
	metadata->parentreg = ((JW*)window1)->RegID;

	nickwindow = JWndInit(NULL,metadata->title,JWndF_NotClosable|JWndF_Resizable,metadata);
	JWSetBounds(nickwindow,264,24,48,80);
	JWSetMin(nickwindow,24,16);
	JWSetMax(nickwindow,64,200);

	JAppSetMain(App, window1);
	
	nickcard = JCardInit(NULL);
	JCntAdd(nickwindow,nickcard);

	txtcard = JCardInit(NULL);
	temp = JTxtInit(NULL);
	scr = JScrInit(NULL, temp, 0);
	
	JWSetBack(temp, COL_Pink);
	JWSetPen(temp, COL_Black); 

	statuswin = malloc(sizeof(IRCChan));
	statuswin->name = "Status";
	statuswin->txtarea = temp;
	statuswin->nicklist = NULL;
	statuswin->changed = 0;
	statuswin->hasnames = 1;
	statuswin->pane = temp;
	curchan = statuswin;
	JCntAdd(txtcard, scr);
	JCntAdd(window1, txtcard);

	text1 = JTxfInit(NULL);
	JCntAdd(window1, text1);

	JWSetBack(text1,COL_Black);
	JWSetPen(text1, COL_White); 

	JWinCallback(text1, JTxf, Entered, fromUser);

	butcon = JCntInit(NULL);
	JCntAdd(window1, butcon);

	temp = JButInit(NULL, "Status");
	JCntAdd(butcon, temp);
	JWSetData(temp, statuswin);
	JWinCallback(temp, JBut, Clicked, chanclick);	
	statuswin->button = temp;

	JWSetMin(window1,104,48);
        JWSetBounds(window1,32,32,200,116);
	JWndSetProp(nickwindow);
	JWndSetProp(window1);

	JWinShow(window1);
	JWReqFocus(text1);
	newThread(outThread,0x400,NULL);
	retexit(0);
	return -1;
}


