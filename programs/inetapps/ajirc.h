
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

#define SRVMSG	1
#define SCRMSG	2
#define CONMSG	3
#define NEWCHAN	4
#define SRVCOM	5

#define RPL_NONE	300
#define RPL_USERHOST	302
#define RPL_NAMREPLY	353
#define RPL_ENDOFNAMES	366

	
#define VERSIONREPLY "Ajirc V1.0 (c) Jolse Maginnis and An Greenwood"

#define IDENT "ajirc"
#define NAME "http://www.jolz64.cjb.net/"

typedef struct chanstr {
	struct chanstr *next;
	struct chanstr *prev;
	void *txtarea;
	void *nicklist;
	void *pane;
	void *button;
	char *name;
	int changed;
	int hasnames;
} IRCChan;

