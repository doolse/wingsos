#include <stdio.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include "getopt.h"
#include <string.h>
#else
#include <unistd.h>
#endif
#include "asm.h"

char magic[6] = { 2,8,'J','o','s',0};
char inmag[6];
char label[64];

typedef struct {
	uint16 flags;
	uint16 version;
	uint16 minstack;
} Header;

typedef struct {
	uint32 startpc;
	uint32 size;
	uint32 relsize;
	uint flags;
} LSegment;

Header head;

LSegment *segs;

int doimps;
int doexps;
int dorel;
int dolinks;

FILE *fp;

uint fr16() {
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

uint32 fr32() {
	int l=fr16();
	int h=fr16();
	return (h<<16)+l;
}


void showsegs() {
	uint16 numsegs,un;
	uint i,seg;
        int ch;
	LSegment *cseg;
	uint32 curpc;
	
	numsegs = fr16();
	printf("Segment Block with %d segments\n", numsegs);
	segs = malloc(sizeof(LSegment) * numsegs);
	cseg = &segs[0];
	for (i=0;i<numsegs;i++) {
		cseg->startpc = fr32();
		cseg->size = fr32();
		cseg->relsize = fr32();
		cseg->flags = fr16();
		printf("Startpc %06lx, size %06lx, relsize %06lx, flags %04x\n", cseg->startpc, cseg->size, cseg->relsize, cseg->flags);
		cseg++;
	}
	cseg = &segs[0];
	for (i=0;i<numsegs;i++) {
		if (!(cseg->flags&S_BLANK)) {
			fseek(fp, cseg->size, SEEK_CUR);
			if (cseg->relsize) {
				curpc = cseg->startpc-1;
				while (1) {
					ch = fgetc(fp);
					if (!ch || ch == EOF)
						break;
					if (ch == 255) {
						curpc += 254;
						continue;
					}
					curpc += ch;
					ch = fgetc(fp);
					seg = ch&0x0f;
					ch >>= 4;
					if (!seg)
						fr16();
					if (ch == RSEG) {
						if (!seg)
							fr16();
					} else if (ch == RHIGH) {
						fgetc(fp);
					}
					if (dorel) {
						switch (ch) {
						case RWORD:
							printf("Word    ");
							break;
						case RLOW:
							printf("Low     ");
							break;
						case RSEG:
							printf("Seg     ");
							break;
						case RSEGADR:
							printf("Long    ");
							break;
						case RHIGH:
							printf("High    ");
							break;
						case RLONG:
							printf("Long32  ");
							break;
						default:
							printf("Err %2x  ", ch);
							break;
						}
						printf("%1d %06lx", seg, curpc);
						putchar('\n');
					}
				}
			}
		}
		cseg++;
	}
	
}

void getlab() {
	uint i=0;
	int ch;
	
	ch = fgetc(fp);
	while (ch && ch != EOF) {		
		label[i++] = ch;
		ch = fgetc(fp);
	}
	label[i] = 0;
}

void showexport() {
	uint16 numexp;
	uint i;
	uint seg;
	uint32 val;
	
	numexp = fr16();
	printf("Exported Labels Block with %d symbols\n", numexp);
	for (i=0;i<numexp;i++) {
		getlab(fp);
		seg = fgetc(fp);
		val = fr32();
		if (doexps)
			printf("%s - %d %06lx\n", label, seg, val);
	}
}

void showlinks() {
	uint16 numlinks;
	uint i,ver;
	
	numlinks = fr16();
	printf("Link Block with %d libraries\n", numlinks);
	for (i=0;i<numlinks;i++) {
		getlab(fp);
		ver = fr16();
		if (dolinks)
			printf("%s - %x\n", label, ver);
	}
}

void showimp() {
	uint16 numimp;
	uint i,ver;
	
	numimp = fr16();
	printf("Import Block with %d labels\n", numimp);
	for (i=0;i<numimp;i++) {
		getlab(fp);
		if (doimps)
			printf("%s\n", label);
	}
}

void showfile() {
	int block;
	uint32 bsize;
	
	fread(inmag, 1, 6, fp);
	if (strncmp(inmag, magic, 6)) {
		fprintf(stderr, "Not Jos object file\n");
		return;
	}
	head.flags = fr16();
	head.version = fr16();
	head.minstack = fr16();
	printf("Flags %x\n", head.flags);
	printf("Version %x\n", head.version);
	printf("Minimum stack %x\n", head.minstack);
	while ((block = fgetc(fp)) != EOF) {
		bsize = fr32();
		switch (block) {
			case IMPORT:
				showimp();
				break;
			case LINKS:
				showlinks();
				break;
			case SEGMENTS:
				showsegs();
				break;
			case EXPORT:
				showexport();
				break;
			default:
				printf("Block %d, Size %ld\n", block, bsize);
				fseek(fp, bsize, SEEK_CUR);
		}
	}
}

void usage() {
	printf("file65: examine an 'o65' file\n"
		"file65 [-ixrlv] filename\n"
		"options:\n"
		"-i    = show imports\n"
		"-x    = show exports\n"
		"-r    = show relocation entries\n"
		"-l    = show links\n"
		"-v    = show all\n"
	);
	exit(1);
}

int main(int argc, char *argv[]) {
	int ch;
	
	if (argc<2)
		usage();
	while ((ch = getopt(argc, argv, "vixrl")) != EOF) {
		switch(ch) {
		case 'v':
			doimps = 1;
			doexps = 1;
			dorel = 1;
			dolinks = 1;
			break;
		case 'i':
			doimps = 1;
			break;
		case 'x':
			doexps = 1;
			break;
		case 'r':
			dorel = 1;
			break;
		case 'l':
			dolinks = 1;
			break;
		}
	}
	fp = fopen(argv[optind], "r");
	if (fp) {
		showfile(fp);
		fclose(fp);
	}
}
