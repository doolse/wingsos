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

/* The PPP code 

TODO:

VJ Compression

*/

#define debout(a) if (debug) a

#define FRAME	0x7e
#define CTRL	0x7d

#define MAXPACK 2000

#define PPP_PacketSend	0xe1
#define PPP_LCPUp	0xe3
#define PPP_PacketRecv	0xe4

#define PPPROT_IP 	0x21
#define PPPROT_LCP	0xc021
#define PPPROT_IPCP	0x8021
#define PPPROT_PAP	0xc023

#define PPPINITFCS      0xffff  
#define PPPGOODFCS      0xf0b8  

#define LCP_ConReq	1
#define LCP_ConAck	2
#define LCP_ConNak	3
#define LCP_ConRej	4
#define LCP_TerReq	5
#define LCP_TerAck	6
#define LCP_CodeRej	7
#define LCP_ProtRej	8
#define LCP_EchoReq	9
#define LCP_EchoRep	10
#define LCP_DiscReq	11

#define LCOP_MRU	1
#define LCOP_Async	2
#define LCOP_Aprot	3
#define LCOP_Qprot	4
#define LCOP_Magic	5
#define LCOP_Qual	6
#define LCOP_Pcomp	7
#define LCOP_ACcomp	8
#define LCOP_MRRU	17
#define LCOP_MEnd	19

#define IPOP_Addresses	1
#define IPOP_Comp	2
#define IPOP_Address	3
#define IPOP_PDNS	0x81

uint fcstab[256] = {
      0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
      0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
      0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
      0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
      0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
      0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
      0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
      0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
      0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
      0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
      0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
      0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
      0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
      0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
      0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
      0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
      0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
      0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
      0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
      0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
      0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
      0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
      0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
      0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
      0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
      0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
      0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,

      0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
      0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
      0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
      0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
      0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

#define PSTATE_Closed	0
#define PSTATE_Open	1

uchar packet[MAXPACK];
uchar pack2[512];
uchar *pout;

int maxtrans=10;
int timer=-1;
int thisChan;
int tcpFD;
uint repType;
uint ident;
uint prot;
FILE *outstream;

uint32 localIP=0;
uint32 priDNS=0;
uint32 RX;
uint32 RXErr;
uint32 TX;

int protComp=0;
int fieldComp=0;

int reqPcomp=0;
int reqACcomp=0;

uint32 asyncMap=0xffffffff;
uint32 reqAsync=0xffffffff;
uint32 magicnum;

uint MRU=1500;
uint reqMRU;

int debug;
int noasync;
int nomagic;
int nopcomp;
int noaccomp;
int nomru;
int noqual;
int nopdns;

int IPUp;
int LCPUp;
int AuthUp=1;
int terminate;

int ackedThem;
int ackedUs;

uint fcsglob;
int latestID;
char *username = "guest";
char *password = "login";

void sendPPP(uchar *pack, uint size);

void resetAlarm() {
	timer = setTimer(timer, 0);
}

void setAlarm() {
	timer = setTimer(timer, 3000, 0, thisChan, PMSG_Alarm);
}

uint tonet(uint word) {
	return ((uint)((word & 0xff) << 8) | (uint)((word & (uint)0xff00) >> 8));
}

void putop(uint op, uint length, ...) {
	uchar *upto;
	uchar *outp = pout;
	
	va_start(upto,length);
	outp[0] = op;
	outp[1] = length+2;
	memcpy(outp+2, upto, length);
	pout += length+2;
}

void sendCP() {
	uint size = pout-pack2;
	
	pack2[0] = repType;
	pack2[1] = ident;
	*(uint *)(pack2+2) = tonet(size);
	debout(debPack(pack2+4, size-4, 1, repType));
	sendPPP(pack2, size);
}

void sendAck(uchar *upto, uint len) {
	memcpy(pout, upto, len);
	pout += len;
	sendCP();
}


void copyOp(uchar *upto, uint len, uint field, uint code) {
	uchar *outp = pout;
	
	if (repType != code) {
		pout = pack2+4;
		outp = pout;
		repType = code;
	}
	outp[0] = field;
	outp[1] = len+2;
	memcpy(outp+2, upto, len);
	pout += len+2;
}

void doEchoReq(uchar *upto, uint length) {
	repType = LCP_EchoRep;
	memcpy(pout, &magicnum, 4);
	pout+=4;
	length-=4;
	upto+=4;
	sendAck(upto, length);
}

void quote(uchar *upto, uint thislen) {
	while (thislen) {
		fprintf(stderr, " %02x", *upto);
		thislen--;
		upto++;
	}
}

void debLCPOp(uchar *upto, uint length) {
	int field;
	uint thislen;

	while (length) {
		field = upto[0];
		thislen = upto[1];
		upto += 2;
		length -= thislen;
		thislen -= 2;
		switch(field) {
			case LCOP_Async:
		   		fprintf(stderr, "<ASYNC %lx> ",* (uint32 *)upto);
				break;
			case LCOP_Magic:
				fprintf(stderr, "<MAGIC %lx> ",* (uint32 *)upto);
				break;
			case LCOP_Pcomp:
		   		fprintf(stderr, "<PCOMP> ");
				break;
			case LCOP_ACcomp:
		   		fprintf(stderr, "<ACCOMP> ");
				break;
			case LCOP_Aprot:
		   		fprintf(stderr, "<APROT %x> ",tonet(* (uint *)upto));
				break;
			case LCOP_MRU:
				fprintf(stderr, "<MRU %x> ",tonet(* (uint *)upto));
				break;
			case LCOP_MRRU:
				fprintf(stderr, "<MRRU %x> ",tonet(* (uint *)upto));
				break;				
			default:
				fprintf(stderr, "<Unknown %02x", field);
				quote(upto, thislen);
				fprintf(stderr, "> ");
				break;
		}
		upto += thislen;
	}
}

void debOp(uchar *upto, uint length) {
	int field;
	uint thislen;

	while (length) {
		field = upto[0];
		thislen = upto[1];
		upto += 2;
		length -= thislen;
		thislen -= 2;
		fprintf(stderr, "<Unknown %02x", field);
		quote(upto, thislen);
		fprintf(stderr, "> ");
		upto += thislen;
	}
}

void debLCPCode(int code) {
	switch (code) {
		case LCP_ConReq:
			fprintf(stderr, "ConReq ");
			break;
		case LCP_ConAck:
			fprintf(stderr, "ConAck ");
			break;
		case LCP_EchoReq:
			fprintf(stderr, "EchoReq ");
			break;
		case LCP_ConNak:
			fprintf(stderr, "ConNak ");
			break;
		case LCP_ConRej:
			fprintf(stderr, "ConRej ");
			break;
		case LCP_TerReq:
			fprintf(stderr, "TerReq ");
			break;
		case LCP_TerAck:
			fprintf(stderr, "TerAck ");
			break;
		case LCP_CodeRej:
			fprintf(stderr, "CodeRej ");
			break;
		case LCP_ProtRej:
			fprintf(stderr, "ProtRej ");
			break;
		case LCP_EchoRep:
			fprintf(stderr, "EchoRep ");
			break;
		case LCP_DiscReq:
			fprintf(stderr, "DiscReq ");
			break;
		default:
			fprintf(stderr, "!LCP! %d\n",code);
			break;
		}
	}

void debPack(uchar *upto, uint length, int send, int code) {
	int field;
	uint thislen;
	
	fprintf(stderr, "%s %u [", send ? "sent":"recv", length);
	switch (prot) {
		case PPPROT_LCP:
			fprintf(stderr, "LCP ");
			debLCPCode(code);
			fprintf(stderr, "id=0x%02x ", ident);
			switch(code) {
				case LCP_EchoReq:
				case LCP_TerReq:
				case LCP_TerAck:
				case LCP_CodeRej:
				case LCP_ProtRej:
				case LCP_EchoRep:
				case LCP_DiscReq:
					break;
				default:
					debLCPOp(upto, length);
					break;
			}
			break;
		case PPPROT_IPCP:
			fprintf(stderr, "IPCP ");
			debLCPCode(code);
			fprintf(stderr, "id=0x%02x ", ident);
			debOp(upto, length);
			break;
		case PPPROT_PAP:
			fprintf(stderr, "PAP ");
			fprintf(stderr, "id=0x%02x ", ident);
			break;
		default:
			fprintf(stderr, "Unknown %d ", prot);
			break;			
	}
	fprintf(stderr, "]\n");
}

void doConReq(uchar *upto, uint length) {
	uchar *orig = upto;
	uint origlen = length;
	uint field, thislen;
	uint temp;

	if (LCPUp) {
		IPUp = 0;
		LCPUp = 0;
		AuthUp = 1;
		setAlarm();
		ackedThem = 0;
		ackedUs = 0;
		nopdns = 0;
	}
	repType = LCP_ConAck;
	while (length) {
		field = upto[0];
		thislen = upto[1];
		upto += 2;
		length -= thislen;
		thislen -= 2;
		switch(field) {
			case LCOP_Async:
		   		reqAsync = * (uint32 *)upto;
				break;
			case LCOP_Magic:
				if (magicnum == * (uint32 *)upto) {
					fprintf(stderr, "Looped back link!\n");
					exit(1);
				}
				break;
			case LCOP_Pcomp:
				reqPcomp = 1;
				break;
			case LCOP_ACcomp:
				reqACcomp = 1;
				break;
			case LCOP_Aprot:
				temp = tonet(* (uint *)upto);
				if (temp == PPPROT_PAP) {
					AuthUp = 0;
				} else {
					copyOp(upto, thislen, LCOP_Aprot, LCP_ConRej);
				}
				break;
			case LCOP_MRU:
		   		reqMRU = tonet(* (uint *)upto);
				break;
			default:
				copyOp(upto, thislen, field, LCP_ConRej);
				break;
		}
		upto += thislen;
	}
	if (repType == LCP_ConAck) {
		sendAck(orig, origlen);
		ackedThem = 1;
		if (ackedUs) {
			ackedThem = 0;
			ackedUs = 0;
			doLCPUp();
		}
	} else sendCP();
}

void doConRej(uchar *upto, uint length) {
	uint field, thislen;
	while (length) {
		field = upto[0];
		thislen = upto[1];
		upto += 2;
		length -= thislen;
		thislen -= 2;
		switch(field) {
			case LCOP_Async:
				noasync = 1;
				break;
			case LCOP_Magic:
				nomagic = 1;
				break;
			case LCOP_Pcomp:
				nopcomp = 1;
				break;
			case LCOP_ACcomp:
				noaccomp = 1;
				break;
			case LCOP_MRU:
				nomru = 1;
				break;
			case LCOP_Qual:
				noqual = 1;
				break;
		}
		upto += thislen;
	}
	conReq();
}


void doConReqIP(uchar *upto, uint length) {
	uchar *orig = upto;
	uint origlen = length;
	uint field;
	uint thislen;
	uint32 remoteIP;
	
	repType = LCP_ConAck;
	while (length) {
		field = upto[0];
		thislen = upto[1];
		upto += 2;
		length -= thislen;
		thislen -= 2;
		switch(field) {
			case IPOP_Address:
				remoteIP = * (uint32 *)upto;
				if (remoteIP == 0L) {
					remoteIP = 0x0100a8c0; /* 192.168.0.1 */
					copyOp((uchar *) &remoteIP, thislen, field, LCP_ConNak);
				}
				break;
			default:
				copyOp(upto, thislen, field, LCP_ConRej);
				break;
		}
		upto += thislen;
	}
	if (repType == LCP_ConAck) {
		ackedThem = 1;
		sendAck(orig, origlen);
		if (ackedUs)
			doIPUp();
	} else sendCP();
}

void doConNakIP(uchar *upto, uint length) {
	uint field;
	uint thislen;
	
	while (length) {
		field = upto[0];
		thislen = upto[1];
		upto+=2;
		length -= thislen;
		thislen -= 2;
		switch(field) {
			case IPOP_Address:
				localIP = * (uint32 *)upto;
				break;
			case IPOP_PDNS:
				priDNS = * (uint32 *)upto;
				break;				
			default:
				break;
		}
		upto += thislen;
	}
	doLCPUp();
}

void doAuthAckPAP(uchar *upto, uint length) {
	if (ident == latestID) {
		AuthUp = 1;
		printf("Password authenticated!\n");
		doLCPUp();
	}
}

void doAuthNakPAP(uchar *upto, uint length) {
	static char name[32];
	char *str;
	
	resetAlarm();
	printf("Bad username/password!\nEnter Username (return for %s):", username);
	fflush(stdout);
	str = fgets(name, 31, stdin);
	if (name && strcmp(name, "\n")) {
		name[strlen(name)-1] = 0;
		username = strdup(name);
	}
	printf("Enter Password (return for %s):", password);
	fflush(stdout);
	str = fgets(name, 31, stdin);
	if (name && strcmp(name, "\n")) {
		name[strlen(name)-1] = 0;
		password = strdup(name);
	}
	doLCPUp();
}

void doConAckIP(uchar *upto, uint length) {
	if (ident == latestID) {
		ackedUs = 1;
		if (ackedThem)
			doIPUp();
	}
}

void doConAck(uchar *upto, uint length) {
	if (ident == latestID) {
		ackedUs = 1;
		if (ackedThem) {
			ackedUs = 0;
			ackedThem = 0;
			doLCPUp();
		}
	}
}

void doTerReq(uchar *upto, uint length) {
	repType = LCP_TerAck;
	sendAck(upto, length);
	terminate = 1;
	LCPUp = 0;
	setAlarm();
	ackedThem = 1;
}

void doTerAck(uchar *upto, uint length) {
	if (!terminate)
		return;
	if (ident == latestID) {
		termPPP();
	}
}

void termPPP() {
	printf("PPP session closed\n");
	exit(0);
}

void authReqPAP() {
	uchar *outp;
	uint lenuser = strlen(username);
	uint lenpass = strlen(password);
	char *string;

	pout = pack2+4;
	outp = pout;
	
	latestID++;
	ident = latestID;
	debout(fprintf(stderr, "Requesting Authorisation\n"));
	
	*outp = lenuser;
	outp++;
	strcpy(outp, username);
	outp += lenuser;
	
	*outp = lenpass;
	outp++;
	strcpy(outp, password);
	pout = outp + lenpass;
	repType = LCP_ConReq;
	prot = PPPROT_PAP;
	sendCP();
}


void conReqIP() {
	pout = pack2+4;
	repType = LCP_ConReq;
	prot = PPPROT_IPCP;
	putop(IPOP_Address, 4, localIP);
	if (!nopdns)
		putop(IPOP_PDNS, 4, priDNS);
	latestID++;
	ident = latestID;
	sendCP();
}

void doConRejIP(uchar *upto, uint length) {
	uint field, thislen;
	while (length) {
		field = upto[0];
		thislen = upto[1];
		upto += 2;
		length -= thislen;
		thislen -= 2;
		switch(field) {
			case IPOP_PDNS:
				nopdns = 1;
				break;
		}
		upto += thislen;
	}
	if (!ackedUs)
		conReqIP();
}

void terReq() {
	pout = pack2+4;
	latestID++;
	ident = latestID;
	repType = LCP_TerReq;
	prot = PPPROT_LCP;
	sendCP();
}

void conReq() {
	pout = pack2+4;
	if (!maxtrans) {
		fprintf(stderr, "PPP timed out!\n");
		exit(1);
	} else {
		latestID++;
		ident = latestID;
		if (!noasync)
			putop(LCOP_Async,4,(uint32) 0x00000000);
		if (!nopcomp)
			putop(LCOP_Pcomp,0);
		if (!noaccomp)
			putop(LCOP_ACcomp,0);
		if (!nomagic)
			putop(LCOP_Magic, 4, magicnum);
		if (!nomru)
			putop(LCOP_MRU, 2, tonet(1500));
		repType = LCP_ConReq;
		prot = PPPROT_LCP;
		sendCP();
		maxtrans--;
	}
}



void doIPUp() {
	uchar *anip = (uchar *) &localIP;
	uchar *anip2 = (uchar *) &priDNS;

	resetAlarm();
	IPUp = 1;
	asyncMap = reqAsync;
	fieldComp = reqACcomp;
	protComp = reqPcomp;
	sendCon(tcpFD, NET_AddInt, "/sys/ppp0", localIP, 0L);
	printf("IP is up! IP is %d.%d.%d.%d\n",anip[0],anip[1],anip[2],anip[3]);
	if (priDNS) {
		printf("DNS IP is %d.%d.%d.%d\n",anip2[0],anip2[1],anip2[2],anip2[3]);
		sendCon(tcpFD, NET_DNSAddr, priDNS, 0L);
	}
	retexit(0);
}

void doLCPUp() {
	debout(fprintf(stderr, "LCP Up!\n"));
	LCPUp = 1;
	if (!AuthUp) {
		authReqPAP();
		maxtrans = 10;
	} else {
		conReqIP();
		maxtrans = 10;
	}
	setAlarm();
}


void docp(uchar *upto, uint count) {
	uint code = upto[0];
	uint length;
	ident = upto[1];
	pout = pack2+4;
	
	upto += 2;
	length = tonet(*(uint *)upto);
	upto += 2;
	length -= 4;
	debout(debPack(upto, length, 0, code));

	if (prot == PPPROT_LCP) {
		switch (code) {
			case LCP_ConReq:
				doConReq(upto, length);
				break;
			case LCP_ConAck:
				doConAck(upto, length);
				break; 
			case LCP_EchoReq:
				doEchoReq(upto, length);
				break;
			case LCP_ConNak:
				break;
			case LCP_ConRej:
				doConRej(upto, length);
				break;
			case LCP_TerReq:
				doTerReq(upto, length);
				break;
			case LCP_TerAck:
				doTerAck(upto, length);
				break;
			case LCP_CodeRej:
			case LCP_ProtRej:
			case LCP_EchoRep:
			case LCP_DiscReq:
				break;
		}
	}
	else if (prot == PPPROT_IPCP && LCPUp && AuthUp) {
			switch (code) {
			case LCP_ConReq:
				doConReqIP(upto, length);
				break;
			case LCP_ConAck:
				doConAckIP(upto, length);
				break; 
			case LCP_ConNak:
				doConNakIP(upto, length);
				break;
			case LCP_ConRej:
				doConRejIP(upto, length);
				break;
			case LCP_TerReq:
			case LCP_TerAck:
			case LCP_CodeRej:
			case LCP_ProtRej:
			case LCP_EchoReq:
			case LCP_EchoRep:
			case LCP_DiscReq:
			default:
				debout(fprintf(stderr, "Unrecognised IPCP code! %d\n",code));
				break;
		}
	}
	else if (prot == PPPROT_PAP && LCPUp) {
			switch (code) {
			case LCP_ConAck:
				doAuthAckPAP(upto, length);
				break; 
			case LCP_ConNak:
				doAuthNakPAP(upto, length);
				break;
			case LCP_ConReq:
			default:
				debout(fprintf(stderr, "Unrecognised PAP code! %d\n",code));
				break;
		}
	}
}

uint pppfcs(uint fcs, uchar *cp, uint len) {
	while (len) {
		fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp) & 0xff];
		len--;
		cp++;
	}
	return fcs;
}

uchar *process(uint count) {
	uchar *upto=packet;
	uint fcs;
	
	pout = pack2+4;
	
	if ((fcs = pppfcs(PPPINITFCS, upto, count)) != PPPGOODFCS) {
		RXErr++;
		debout(fprintf(stderr, "Bad FCS: %d\n",fcs));
		while (count) {
			debout(fprintf(stderr, "%02x ", *upto));
			count--;
			upto++;
		}
		debout(fprintf(stderr, "\n"));
		return; 
	} 
	count -= 2;
	prot = tonet(* (uint *)(upto));
	if (prot == 0xff03) {
		prot = tonet(* (uint *)(upto+2));
		upto += 2;
		count -= 2;
	}
	if (!(prot & 0x100)) {
		upto++;
		count--;
	}
	else prot >>= 8;
	upto++;
	count--;
	if (prot == PPPROT_LCP || prot == PPPROT_IPCP || prot == PPPROT_PAP)
		docp(upto, count);
	else if (prot == PPPROT_IP) {
		return upto;
	} else {
	   	uchar *outp = pout;
		ident = upto[1];

		debout(fprintf(stderr, "Unknown Protocol %x\n",prot));
		debout(quote(upto, count));
		debout(fprintf(stderr, "\n"));
		*(uint *)outp = tonet(prot);
		pout += 2;
		repType = LCP_ProtRej;
	     	prot = PPPROT_LCP;
		sendCP();
	}
	return NULL;
}

void asyncput(uint ch) {
	FILE *stream = outstream;
	
	fcsglob = pppfcs(fcsglob, (uchar *)&ch, 1);
	if (ch < 0x20) {
		if (asyncMap && asyncMap & ((uint32) 1 << ch)) {
			fputc(CTRL,stream);
			fputc(ch ^ 0x20,stream);
		} else {
			fputc(ch,stream);
		}
	} else 
	switch (ch) {
		case FRAME:
		case CTRL:
			fputc(CTRL,stream);
			fputc(ch ^ 0x20, stream);
			break;
		default:
			fputc(ch,stream);
			break;
	}
}

void sendPPP(uchar *pack, uint size) {
	FILE *stream = outstream;
	uint fcs;

	fputc(FRAME,stream);
	fcsglob = PPPINITFCS;
	if (!fieldComp || prot == PPPROT_LCP) {
		asyncput(0xff);
		asyncput(0x03);
	}
	if (!protComp || prot >= 0x100)
		asyncput(prot >> 8); 
	asyncput(prot & 0xff); 
	while (size) {
		asyncput(*pack);
		size--;
		pack++;
	}
	fcs = fcsglob ^ (uint) PPPINITFCS;
	asyncput(fcs&0xff);
	asyncput(fcs >> 8);
	fputc(FRAME,stream);
	TX++;
	fflush(stream);
}

int fillIntStat(IntStat *outbuf) {
	outbuf->Type = PPPType;
	*(uint32 *) &(outbuf->IP) = localIP;
	*(uint32 *) &(outbuf->Mask) = 0;
	outbuf->RX = RX;
	outbuf->TX = TX;
	outbuf->RXErr = RXErr;
	outbuf->TXErr = 0;
	outbuf->RXBytes = 0;
	outbuf->TXBytes = 0;
	outbuf->MaxPacket = 1500;
	return 1;
}

void outThread() {
	uchar *msg;
	int rcvid;
	int ch;
	int type;
	uint32 ret;
	
	conReq();
	setAlarm();
	while(1) {
		rcvid = recvMsg(thisChan,(void *) &msg);
		prot = 0;
		ret = 1;
		type = (int) *msg;
		switch (type) {
			case IO_OPEN:
				if (*(int *)(msg+6) & (O_PROC|O_STAT))
					ret = makeCon(rcvid, 1);
				else ret=-1;
				break;
			case PPP_PacketRecv:
				ret = (uint32) process(*(uint *)(msg+2));
				break;
			case NET_StatInt:
				ret = fillIntStat(*(IntStat **)(msg+2));
				break;
			case NET_PacketSend:
				prot = PPPROT_IP;
				sendPPP(*(uchar **)(msg+2), *(uint *)(msg+6));
				break;
			case IO_CONTROL:
				terminate = 1;
				setAlarm();
				terReq();
				LCPUp = 0;
				ackedThem = 0;
				break;
			case IO_FSTAT:
				ret = _fillStat(msg, DT_DEV);
				break;
			case IO_CLOSE:
				break;
			case PMSG_Alarm:
				if (terminate) {
					if (ackedThem)
						termPPP();
					else terReq();
				} else {
					if (!LCPUp)
						conReq();
					else if (!AuthUp)
						authReqPAP();
					else conReqIP();
				}
				if (timer != -1)
					setAlarm();
				break;
			default:
				printf("Unknown! %d\n", type);
				break;
		}
		replyMsg(rcvid,ret);
	}
}

void main(int argc, char *argv[]) {
	int ch;
	int count;
	uchar *upto;
	FILE *stream;
	struct termios T1;

	tcpFD = open("/sys/tcpip",O_PROC);
	if (tcpFD == -1) {
		fprintf(stderr,"TCP/IP not loaded!\n");
		exit(1);
	} 
	if (argc < 2) {
		fprintf(stderr,"Usage: ppp [-d] device [username password]\n");
		exit(1);
	}
	while ((ch = getopt(argc, argv, "d")) != EOF) {
		switch (ch) {
			case 'd':
				debug = 1;
				break;
		}
	}
	if (argc-optind >= 3) {
		username = argv[optind+1];
		password = argv[optind+2];
	}
	printf("Attempting ppp connection on %s\n",argv[optind]);
	stream = fopen(argv[optind],"r+");
	if (!stream) {
		perror("ppp");
		exit(1);
	}
	gettio(fileno(stream),&T1);
	T1.flags=0;
	settio(fileno(stream),&T1);
	outstream = fdopen(fileno(stream),"wb");
	thisChan = makeChanP("/sys/ppp0");
	if (thisChan == -1) {
		fprintf(stderr,"PPP already loaded!\n");
	   	exit(1);
	}
   	newThread(outThread,0x200,NULL);
   	newThread(outThread,0x200,NULL);
	count = 0;
	magicnum = (uint32) rand() * (uint32) rand();
	while ((ch = fgetc(stream)) != EOF) {
		if (ch == FRAME) {
			if (count > 4) {
				RX++;
				upto = (uchar *)sendChan(thisChan, PPP_PacketRecv, count);
				if (upto)
					sendCon(tcpFD, NET_PacketRecv, upto);
			}
			count = 0;
		}
		else {
			if (ch == CTRL)
				ch = fgetc(stream) ^ 0x20;
			if (count<MAXPACK)
				packet[count++]=ch;
		}
	}
}

