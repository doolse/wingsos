/* net.c
 *
 * This file is part of ftp.
 *
 *
 * 01/25/96 Initial Release	Michael Temari, <temari@ix.netcom.com>
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* 
#include <sys/ioctl.h>
#include <net/netlib.h>
#include <net/hton.h>
#include <net/gen/netdb.h>
#include <net/gen/in.h>
#include <net/gen/inet.h>
#include <net/gen/tcp.h>
#include <net/gen/tcp_io.h> */

#include "ftp.h"
#include "file.h"
#include "net.h"

_PROTOTYPE(void donothing, (int sig));

static int ftpcomm_fd;
static char host[256];

void NETinit()
{
}

int DOopen()
{
   char *ent;
   int s;
   
   if(linkopen) {
	printf("Use \"CLOSE\" to close the connection first.\n");
	return(0);
   }

   if(cmdargc < 2)
	readline("Host: ", host, sizeof(host));
   else
	strncpy(host, cmdargv[1], sizeof(host));
   ent = malloc(strlen(host)+13);
   
   strcpy(ent, "/dev/tcp/");
   strcat(ent, host);
   strcat(ent, ":21");
   s = open(ent, O_RDWR);

   if(s < 0) {
	perror("ftp: connect");
	return(s);
   }

   ftpcomm_fd = s;
   fpcommin  = fdopen(ftpcomm_fd, "r");
   fpcommout = fdopen(ftpcomm_fd, "w");

   s = DOgetreply();

   if(s < 0) {
	fclose(fpcommin);
	fclose(fpcommout);
	close(ftpcomm_fd);
	return(s);
   }

   if(s != 220) {
	fclose(fpcommin);
	fclose(fpcommout);
	close(ftpcomm_fd);
	return(0);
   }

   linkopen = 1;

   return(s);
}

int DOclose()
{
   if(!linkopen) {
	printf("You can't close a connection that isn't open.\n");
	return(0);
   }

   fclose(fpcommin);
   fclose(fpcommout);
   close(ftpcomm_fd);

   linkopen = 0;
   loggedin = 0;

   return(0);
}

int DOquit()
{
int s;

   if(linkopen) {
	s = DOcommand("QUIT", "");
	s = DOclose();
   }

   printf("FTP done.\n");

   exit(0);
}

void donothing(sig)
int sig;
{
}

int DOdata(datacom, file, direction, fd)
char *datacom;
char *file;
int direction;  /* RETR or STOR */
int fd;
{
   int s;
   unsigned int i;
   char port[32];
   unsigned char *pstr,*astr;
   int ftpdata_fd;
   struct sockaddr_in addr;
   socklen_t len = sizeof(addr);

   ftpdata_fd = socket(AF_INET, SOCK_STREAM, 0);
   if (ftpdata_fd == -1)
   	return -1;
   if(passive) {
	s = DOcommand("PASV", "");
	if(s != 227) {
		return(s);
	}
	pstr = strchr(reply, '(');
	if (pstr) {
		pstr++;
		astr = (unsigned char *)&addr.sin_addr.s_addr;
		for (i=0;i<4;i++) {
			astr[i] = atoi(pstr);
			pstr = strchr(pstr, ',');
			if (!pstr)
				goto nopass;
			else pstr++;
		}
		astr = (unsigned char *)&addr.sin_port;
		astr[0] = atoi(pstr);
		pstr = strchr(pstr, ',');
		if (!pstr)
			goto nopass;
		pstr++;
		astr[1] = atoi(pstr);
		addr.sin_family = AF_INET;
		s = connect(ftpdata_fd, (struct sockaddr *)&addr, sizeof(addr));
		if (s == -1)
			goto nopass;
	} else {
		nopass:
		printf("Failed to pass PASV reply\n");
		goto failed;
	}
   } else {
	getsockname(ftpcomm_fd, (struct sockaddr *)&addr, &len);
	addr.sin_port = 0;
	pstr = (unsigned char *)&addr.sin_port;
	bind(ftpdata_fd, (struct sockaddr *)&addr, sizeof(addr));
	listen(ftpdata_fd, 1);
	getsockname(ftpdata_fd, (struct sockaddr *)&addr, &len);
	sprintf(port, "%u,%u,%u,%u,%u,%u", pstr[2], pstr[3], pstr[4], pstr[5], pstr[0], pstr[1]);
	s = DOcommand("PORT", port);
	if(s != 200) {
		close(ftpdata_fd);
		return(s);
	}
   }

   s = DOcommand(datacom, file);
   if(s == 125 || s == 150) {
	if(!passive) {
		s = accept(ftpdata_fd, NULL, NULL);
		if (s != -1) {
			close(ftpdata_fd);
			ftpdata_fd = s;
		} else goto failed;
	}
	switch(direction) {
		case RETR:
			s = recvfile(fd, ftpdata_fd);
			break;
		case STOR:
			s = sendfile(fd, ftpdata_fd);
			break;
	}
	close(ftpdata_fd);
	s = DOgetreply();
   } else {
	close(ftpdata_fd);
   }
   return(s);
   failed:
   close(ftpdata_fd);
   return -1;
}
