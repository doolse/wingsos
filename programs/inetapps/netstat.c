#include <stdio.h>
#include <wgsipc.h>
#include <fcntl.h>
#include <net.h>
#include <string.h>
#include <sys/types.h>

NetStat bufs[128];
IntStat ibufs[128];
char buf[25];

int main(int argc, char *argv[]) {
	int fd,i;
	NetStat *cur=bufs;
	IntStat *icur=ibufs;
	
	fd = open("/sys/tcpip",O_PROC);
	if (fd == -1) {
		fprintf(stderr, "tcpip.drv not loaded!\n");
		exit(1);
	}
	i = statall(fd, bufs, 128*sizeof(NetStat));
	printf("Local                 Foreign               RcvQ  SndQ  Flags\n");
	while (i>0) {
		sprintf(buf, "%s:%u", inet_ntoa(cur->SIP), cur->SPort);
		printf("%-22s", buf);
		sprintf(buf, "%s:%u ", inet_ntoa(cur->DIP), cur->DPort);
		printf("%-22s", buf);
		printf("%-5u %-5u %04x\n", cur->RecvQ, cur->SendQ, cur->State);
		cur++;
		i--;
	}
	i = statint(fd, ibufs, 128*sizeof(IntStat));
	printf("\nInterfaces\n");
	while (i>0) {
		char *tystr = "PPP";
		printf("\nType IP              Mask\n");
		if (icur->Type == ETHType)
			tystr = "ETH";
			
		printf("%s  %-16s", tystr, inet_ntoa(icur->IP));
		printf("%-16s\n", inet_ntoa(icur->Mask));		
		printf("RX packets:%-8ld errors:%-8ld bytes:%-8ld\n", icur->RX, icur->RXErr, icur->RXBytes);
		printf("TX packets:%-8ld errors:%-8ld bytes:%-8ld\n", icur->TX, icur->TXErr, icur->TXBytes);		
		icur++;
		i--;
	}
	
}

