#include <stdio.h>
#include <wgslib.h>
#include <stdlib.h>
#include <termio.h>
#include <sys/types.h>

struct PSInfo APS;
struct termios TIO;
char tokb[9];
char totime[9];

void maketime(uint32 val) {
	val /= 1000;
	sprintf(totime, "%02u:%02u:%02u", (uint) val/3600, (uint) val%3600/60, (uint) val %60);
}

void makekb(uint32 val) {
	if (val < 10240) {
		sprintf(tokb, "%ld", val);
	} else {
		sprintf(tokb, "%u.%uK", (uint) val/1024, (uint) val%1024/103);
	}
}

void main(int argc, char *argv[]) {
	int prevpid;
	int verbose=0;
	
	if (gettio(STDOUT_FILENO, &TIO) != -1) {
		if (TIO.cols > 60)
			verbose = 1;
	}
	
	while ((prevpid = getopt(argc, argv, "v")) != EOF) {
		switch(prevpid) {
			case 'v':
				verbose = 1;
				break;
		}
	}
	prevpid = 1;
	printf(" PID COMMAND               MEM     TIME");
	if (verbose) {
		printf("   SHARED  PRI");
	}
	putchar('\n');
	do {
		prevpid = getPSInfo(prevpid, &APS);
		makekb(APS.Mem);
		maketime(APS.Time);
		printf("%4d %s %8s %8s", APS.PID, APS.Name, tokb, totime);
		if (verbose) {
			makekb(APS.Shared);
			printf(" %8s %4u", tokb, APS.priority);
		}
		putchar('\n');
	} while (prevpid);
}
