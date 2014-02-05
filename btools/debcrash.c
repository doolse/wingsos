#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef _MSC_VER
#include "getopt.h"
#include <string.h>
#else
#include <unistd.h>
#endif
#include "asm.h"

typedef struct {
	uint16 address;
	uint16 prevsize;
	uint16 size;
	uint16 magic;
	uint16 owner;
} KMem;

typedef struct {
	uint32 size __attribute__ ((packed));
	uint16 owner __attribute__ ((packed));
	uchar bytes[8] __attribute__ ((packed));
} Mem;

typedef struct {
	uint16 pid;
	uint16 ppid;
	uint32 memalloc __attribute__ ((packed));
	uint32 sharedmem __attribute__ ((packed));
	uint32 time __attribute__ ((packed));
	uint16 priority;
	char name[18] __attribute__ ((packed));
	uint16 threads;
} Proc;

typedef struct {
	uint16 state;
	uint16 stack;
	uint16 dpmem;
	uint16 ZP;
	uint16 Y;
	uint16 X;
	uint16 A;
	uchar DBR;
	uchar SR;
	uint32 address __attribute__ ((packed));
} Thread;

uint fr16(FILE *fp) {
	int ch;
	int ch2;
	ch = fgetc(fp);
	ch2 = fgetc(fp);
	if (ch == -1 || ch2 == -1)
	{
		fprintf(stderr, "Unexpected EOF\n");
		exit(1);		
	}
	return (ch2<<8)+ch;
}

char *goodbad(uint16 type)
{
    if (type == 0x1980)
	return "OK  ";
    else if (type == 0xdead)
	return "FREE";
    else return "BAD ";
}

char *goodbad2(uint type)
{
    if (type == 0)
	return "FREE";
    else if (type == 1)
	return "OK  ";
    else return "BAD ";
}

char *state(uint state)
{
    switch (state)
    {
	case 0x00:
	    return "Dead";
	case 0x01:
	    return "Ready";
	case 0x02:
	    return "Stopped";
	case 0x03:
	    return "Waiting";
	case 0x04:
	    return "Dieing";
	case 0x10:
	    return "Send";
	case 0x11:
	    return "Receive";
	case 0x12:
	    return "Reply";
    }
    return "???";
}

Proc *showThreads(Proc *memp, int show)
{
    uint left = memp->threads;
    uint count = 1;
    Thread *thr = (Thread *)&memp[1];
    
    while (left)
    {
	if (show)
	{
	    printf("%s %3d %04x ", memp->name, count, thr->dpmem);
	    printf("%04x %04x %04x %04x %04x %02x  %02x  %06x %s\n",
		    thr->stack, thr->ZP, thr->A, thr->X, thr->Y, thr->DBR, thr->SR, thr->address&0xffffff, state(thr->state));
	}
	count++;
	thr++;
	left--;
    }
    return (Proc *)thr;
}

uchar *showProcs(uchar *kmem)
{
    Proc *memp = (Proc *)&kmem[2];
    uint left = *(uint16 *)kmem;
    printf("Procs %d\n", left); 
    while (left)
    {
	printf("%s PID %3d PPID %3d Mem %06x Shared %06x\n", memp->name, memp->pid, memp->ppid, 
		memp->memalloc, memp->sharedmem);
    	memp = showThreads(memp, 0);
	left--;
    }
    memp = (Proc *)&kmem[2];
    left = *(uint16 *)kmem;
    printf("\nName               # Mem  Stk  ZP   A    X    Y    DBR SR  PC     State\n");
    while (left)
    {
    	memp = showThreads(memp, 1);
	left--;
    }
    
    return (uchar *)memp;
}

char *nameForPID(uchar *kmem, uint16 pid)
{
    Proc *memp = (Proc *)&kmem[2];
    uint left = *(uint16 *)kmem;
    if (!pid)
	return "WiNGS           ";
    while (left)
    {
	if (memp->pid == pid)
	    return memp->name;
    	memp = showThreads(memp, 0);
	left--;
    }
    return "Unknown         ";
}

uchar *showKMem(uchar *kmem, uchar *procs)
{
    KMem *memp = (KMem *)&kmem[2];
    uint left = *(uint16 *)kmem;
    printf("Memory blocks %d\n", left); 
    while (left)
    {
	char *type = goodbad(memp->magic);
	printf("%04x %04x %s %04x %s\n", memp->address, memp->size, type, memp->owner, nameForPID(procs, memp->owner));
	left--;
	memp++;
    }
    return (uchar *)memp;
}

uchar *showMem(uchar *mem, uchar *procs)
{
    Mem *memp = (Mem *)&mem[2];
    uint left = *(uint16 *)mem;
    uchar *address = (uchar *)0x20000;
    printf("Memory blocks %d\n", left); 
    while (left)
    {
	uint type = memp->size>>24;
	char *strtype;
	
	if (type == 3)
	{
	    address = *(uchar **)&memp->owner;
	    printf("Resetting address %06x\n", address);
	}
	else
	{
	    uint32 size = memp->size&0xffffff;
	    uchar *dump = memp->bytes;
	    uint i;
	    strtype = goodbad2(type);
	    printf("%06x %06x %s %04x %s ", address, size, strtype, memp->owner, nameForPID(procs, memp->owner));
	    for (i=0; i<8; i++)
	    {
		uint ch = dump[i];
		if (!isprint(ch))
		    ch = '.';
		putchar(ch);
	    }
	    for (i=0; i<8; i++)
	    {
		printf(" %02x", dump[i]);
	    }
	    putchar('\n');
	    address += size;
	}
	left--;
	memp++;
    }
    return (uchar *)memp;
}

char *causeString(uint why)
{
    switch (why)
    {
	case 0x07:
	    return "Corrupt Stack or Kernel mem (During kill)";
	case 0x01:
	    return "Corrupt Stack or Kernel mem";
	case 0x02:
	    return "Corrupt main memory";
	case 0x03:
	    return "Dead or corrupt thread";
	case 0x04:
	    return "Bad Free";
	case 0x0f:
	    return "C= + restore pressed";
    }
    return "Unknown (possible BRK)";
}

uchar *showCause(uchar *cause)
{
    uint why = *(uint16 *)&cause[2];
    uint who = *(uint16 *)cause;
    uchar *procs = &cause[4];
    char *name = nameForPID(procs, who);
    printf("%s in %04x,'%s'\n", causeString(why), who, name);
    return procs;
}

void usage()
{
    fprintf(stderr, "Usage: debcrash crashdump\n");
    exit(1);
}

int main(int argc, char *argv[]) {
	int ch;
	FILE *fp;
	uchar *report,*procs;
	char *fname;
	
	if (argc<2)
		usage();
	while ((ch = getopt(argc, argv, "")) != EOF) 
	{
	}
	fname = argv[optind];
	fp = fopen(fname, "r");
	if (fp) {
	    uint val1,val2,size;
	    
	    val1 = fr16(fp);
	    val2 = fr16(fp);
	    size = val2-val1;
	    report = malloc(size);
	    fread(report, size, 1, fp);
	    procs = showCause(report);
	    report = showProcs(procs);
	    report = showKMem(report, procs);
	    report = showKMem(report, procs);
	    report = showMem(report, procs);
	    fclose(fp);
	} else perror(argv[optind]);
}
