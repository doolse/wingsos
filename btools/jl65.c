#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "asm.h"

char magic[6] = { 2,8,'J','o','s',0};
char inmag[6];
char label[64];

typedef struct {
	uint16 flags;
	uint16 version;
	uint16 minstack;
} Header;

typedef struct lseg {
	struct lseg *nextm;
	struct lseg *lastm;
	uint32 startpc;
	uint32 size;
	uint32 relsize;
	uint32 totsize;
	uint flags;
	uint32 reloff;
	uchar *data;
	uchar *rel;
	uint newsegnum;
	struct file65 *file;
} LSegment;

typedef struct imp {
	struct imp *next;
	struct imp *nimp;
	char *name;
	uint number;
	int export;
	int seg;
	uint32 val;
	LSegment *origseg;
	struct file65 *file;
} LImport;

typedef struct file65 {
	char *name;
	LImport **implabs;
	LSegment **trans;
} File65;

Header head;

LLink *links;
LSegment *outseg[17];
uint totsegs;
uint impupto;
uint numexp;
uint impsize;
uint expsize;
uint stacksize;
uint minstack;
uint numlinks;
uint linksize=2;
int pack;
int noglobs;
int norel;
int dynamic;
int formatc64;
int flags;
uint version;
LImport *imphash[32];
LImport *hundef;
File65 *curf65;

uint32 curpc,lastpc;

uint rbfsize, bfsize;
uchar *rbuf;

char *outfile="a.o65";

uchar *addbuf(uint size) {
	uchar *ret;
	if (bfsize+size >= rbfsize) {
		if (rbfsize)
			rbfsize <<= 1;
		else rbfsize = 512;
		rbuf = realloc(rbuf, rbfsize);
	}
	ret = rbuf+bfsize;
	bfsize += size;
	return ret;
}

uint hashcode(uchar *str) {
	uint count=0;
	while (*str) {
		count += *str;
		str++;
	}
	return count&31;
}

FILE *fp;

uint fr16() {
	uint ret=0;
	if (fread(&ret, 2, 1, fp) == 1)
		return ret;
	else {
		fprintf(stderr, "Unexpected EOF\n");
		exit(1);
	}
}

uint32 fr32() {
	uint32 ret;
	if (fread(&ret, 4, 1, fp) == 1)
		return ret;
	else {
		fprintf(stderr, "Unexpected EOF\n");
		exit(1);
	}
}

void f16(uint16 val) {
	fwrite(&val, 2, 1, fp);
}

void f32(uint32 val) {
	fwrite(&val, 4, 1, fp);
}

void mergeseg(LSegment *tseg) {
	uint i;
	LSegment *cseg,**upto;
	
	tseg->file = curf65;
	upto = &outseg[0];
	for (i=0;i<totsegs;i++) {
		cseg = *upto;
		if (tseg->flags == cseg->flags && (!(cseg->flags&S_NOCROSS) || (cseg->totsize+tseg->size < 0x10000)))
			break;
		upto++;
	}
	if (i>=totsegs) {
		*upto = tseg;
		totsegs++;
		tseg->totsize = tseg->size;
		tseg->reloff = 0;
		tseg->nextm = NULL;
		tseg->lastm = NULL;
		tseg->newsegnum = totsegs;
	} else {
		uint32 newpc = cseg->startpc+cseg->totsize;
		tseg->reloff = newpc - tseg->startpc;
		tseg->startpc = newpc;
		cseg->totsize += tseg->size;
		if (!cseg->nextm)
			cseg->nextm = tseg;
		else cseg->lastm->nextm = tseg;
		cseg->lastm = tseg;
		tseg->newsegnum = cseg->newsegnum;
	}
}

void linksegs() {
	uint16 numsegs;
	uint i,ch;
	LSegment *cseg,**segtran;
	uint32 curpc;
	int32 reloff;
	
	numsegs = fr16();
	segtran = malloc(sizeof(LSegment *) * numsegs);
	curf65->trans = segtran;
	for (i=0;i<numsegs;i++) {
		cseg = malloc(sizeof(LSegment));
		*segtran = cseg;
		cseg->startpc = fr32();
		cseg->size = fr32();
		cseg->relsize = fr32();
		cseg->flags = fr16();
		if (cseg->size)
			mergeseg(cseg);
		segtran++;
	}
	segtran = curf65->trans;
	for (i=0;i<numsegs;i++) {
		cseg = *segtran;
		if (!(cseg->flags&S_BLANK)) {
			cseg->data = malloc(cseg->size);
			fread(cseg->data, 1, cseg->size, fp);
			if (cseg->relsize) {
				cseg->rel = malloc(cseg->relsize);
				fread(cseg->rel, 1, cseg->relsize, fp);
			} else cseg->rel = NULL;
		}
		segtran++;
	}
}

LImport *dolab(int export) {
	uint i=0,hash;
	uint ch = 1;
	LImport *lab,*head;
	
	do {
		ch = fgetc(fp);
		if (ch == EOF)
			ch = 0;
		label[i] = ch;
		i++;
	} while (ch);
	hash = hashcode(label);
	head = lab = imphash[hash];
	while (lab) {
		if (!strcmp(label, lab->name))
			break;
		lab = lab->next;
	}
	if (!lab) {
		lab = malloc(sizeof(LImport));
		lab->name = strdup(label);
		lab->next = head;
		lab->file = curf65;
		imphash[hash] = lab;
		lab->nimp = hundef;
		hundef = lab;
		lab->seg = -1;
		lab->export = 0;
	}
	if (export) {
		if (lab->export)
			printf("Warning: Label %s defined in %s and %s\n", lab->name, curf65->name, lab->file->name);
		lab->export = 1;
	}
	return lab;
}

void linkexport() {
	uint16 numexp;
	uint i;
	uint seg;
	uint32 val;
	LImport *lab;
	LSegment **trans = curf65->trans;
	LSegment *cseg;
	
	numexp = fr16();
	for (i=0;i<numexp;i++) {
		lab = dolab(1);
		seg = fgetc(fp);
		val = fr32();
		if (!seg) {
			lab->val = val;
			lab->seg = 0;
		} else {
			seg--;
			cseg = trans[seg];
			lab->val = val;
			lab->origseg = cseg;
			lab->seg = cseg->newsegnum;
		}
	}
}

void getimports() {
	uint numimp;
	uint i;
	LImport **curlabs;
	
	numimp = fr16();
	curlabs = malloc(numimp * sizeof(LImport *));
	curf65->implabs = curlabs;
	for (i=0;i<numimp;i++) {
		*curlabs = dolab(0);
		curlabs++;
	}
}

void addlink(char *name, uint ver) {
	LLink *lib=links;
	
	while (lib) {
		if (!strcmp(name, lib->name))
			break;
		lib = lib->next;
	}
	if (!lib) {
		linksize += strlen(name)+3;
		lib = malloc(sizeof(LLink));
		lib->name = strdup(name);
		lib->version = ver;
		lib->next = links;
		links = lib;
		numlinks++;
	} else {
		if (ver > lib->version)
			lib->version = ver;
	}
}

void linklinks() {
	uint numl,i,j;
	int ch;
	
	numl = fr16();
	for (j=0;j<numl;j++) {
		i=0;
		do {
			ch = fgetc(fp);
			if (ch == EOF)
				ch = 0;
			label[i] = ch;
			i++;
		} while (ch);
		addlink(label, fr16());
	}
}

void linkfile(char *str) {
	uint block;
	uint32 bsize;
	File65 *cfp;
	
	fp = fopen(str, "r");
	if (!fp) {
		perror(str);
		exit(1);
	}
	fread(inmag, 1, 6, fp);
	if (strncmp(inmag, magic, 6)) {
		fprintf(stderr, "Not Jos object file\n");
		exit(1);
	}
	fread(&head, 1, sizeof(Header), fp);
	if (!stacksize) {
		if (head.minstack>minstack)
			minstack = head.minstack;
	}
	
	cfp = malloc(sizeof(File65));
	cfp->name = str;
	curf65 = cfp;
	while ((block = fgetc(fp)) != EOF) {
		if (block == ENDFILE)
			break;
		bsize = fr32();
		switch (block) {
			case IMPORT:
				getimports();
				break;
			case SEGMENTS:
				linksegs();
				break;
			case EXPORT:
				linkexport();
				break;
			case LINKS:
				linklinks();
				break;
			default:
				printf("Unknown Block %d, Size %ld\n", block, bsize);
				fseek(fp, bsize, SEEK_CUR);
		}
	}
	fclose(fp);
}

void prepImpExp() {
	LImport *lab = hundef;
	LSegment *tseg;
	uint len;

	impsize = 2;
	expsize = 2;
	while (lab) {
		len = strlen(lab->name);
		if (lab->seg == -1) {
			lab->number = impupto;
			impupto++;
			impsize += len+1;
		} else if (lab->export) {
			numexp++;
			expsize += len+6;
			if (lab->seg) {
				tseg = lab->origseg;
				lab->val += tseg->reloff;
			}
		}
		lab = lab->nimp;
	}
}

void relseg(LSegment *cseg) {
	LImport **trimp = cseg->file->implabs;
	LSegment **trseg = cseg->file->trans;
	LImport *lab;
	LSegment *tseg;
	uchar *relup=cseg->rel;
	uchar *dataup=cseg->data-1;
	uchar *out;
	uint ch,seg,nseg,dif;
	uint32 reloff;
	uint extra;	
	
	curpc = cseg->startpc-1;
	while(ch = *relup) {
		relup++;
		if (ch == 255) {
			curpc += 254;
			dataup += 254;
			continue;
		}
		curpc += ch;
		dataup += ch;
		ch = *relup;
		relup++;
		seg = ch&0x0f;
		ch >>= 4;
		if (!seg) {
			lab = trimp[*(uint16 *)relup];
			relup += 2;
			if (lab->seg != -1) {
				nseg = lab->seg;
				reloff = lab->val;
			} else {
				reloff = 0;
				nseg = 0;
			}
		} else {
			tseg = trseg[seg-1];
			nseg = tseg->newsegnum;
			reloff = tseg->reloff;
			seg = tseg->flags;
		}
		dif = curpc-lastpc;
		while (dif>254) {
			out = addbuf(1);
			out[0] = 255;
			dif -= 254;
		}
		lastpc = curpc;
		out = addbuf(1);
		out[0] = dif;
		extra = 1;
		if (!nseg)
			extra = 3;
		switch (ch) {
			case RWORD:
				*(uint16 *)dataup += reloff;
				break;
			case RLOW:
				dataup[0] += reloff;
				break;
			case RSEGADR:
				reloff += (*(uint32 *)dataup & 0xffffff);
				*(uint16 *)dataup = reloff&0xffff;
				dataup[2] = reloff>>16;
				break;
			case RLONG:
				*(uint32 *)dataup += reloff;
			case RSOFFL:
			case RSOFFH:
				break;
			case RHIGH:
				if (!(seg&S_PALIGN)) {
					reloff += *relup;
					relup++;
				}
				reloff += dataup[0]<<8;
				dataup[0] = reloff>>8;
				if (!nseg || !(tseg->flags&S_PALIGN)) {
					extra++;
				}
				break;
			case RSEG:
				if (!(seg&S_NOCROSS)) {
					reloff += *(uint16 *)relup;
					relup += 2;
					reloff += dataup[0]<<16;
					dataup[0] = reloff>>16;
				} else dataup[0] = tseg->startpc>>16;
				if (!nseg || !(tseg->flags&S_NOCROSS)) {
					extra += 2;
				}
				break;
		}
		out = addbuf(extra);
		out[0] = (ch<<4)|nseg;
		extra--;
		out++;
		if (!nseg) {
			*(uint16 *)out = lab->number;
			out+=2;
			extra-=2;
		}
		if (extra) {
			memcpy(out, &reloff, extra);
		}
		
	}
	
}

void reloc() {
	LSegment *cseg;
	LSegment **upto = &outseg[0];
	uint i;
	uint32 curpc;
	
	prepImpExp();
	for (i=0;i<totsegs;i++) {
		cseg = *upto;
		if (!(cseg->flags&S_BLANK)) {
			lastpc = cseg->startpc-1;
			while (cseg) {
				if (cseg->rel)
					relseg(cseg);
				cseg = cseg->nextm;
			}
		}
		cseg = *upto;
		cseg->rel = rbuf;
		cseg->relsize = bfsize;
		rbuf = NULL;
		rbfsize = 0;
		bfsize = 0;
		upto++;
	}
	
}

void outputit() {
	LImport *lab = hundef;
	LSegment **upto;
	LSegment *cseg;
	uint32 calc;
	uint i;
	
	fp = fopen(outfile, "w");
	if (!fp)
		exit(1);
	
	if (formatc64) 
		goto just64;
	
	/* Jos Magic */
	fwrite(magic, 1, sizeof(magic), fp);
		
	/* Flags */
	f16(flags);	
		
	/* Version */
	f16(version);
		
	if (!stacksize)
		stacksize = minstack;
	/* Stacksize */
	f16(stacksize);
		
	/* Write fopt's */
/*	if (segbufs[6].size) {
		fputc(INFO, fp);
		f32(segbufs[SFOPT].size, fp);
		fwrite(segbufs[SFOPT].bufptrs, 1, segbufs[SFOPT].size, fp);
	} */
	
	if (numlinks) {
		LLink *lib = links;
		fputc(LINKS, fp);
		f32(linksize);
		f16(numlinks);
		while (lib) {
			fputs(lib->name, fp);
			fputc(0, fp);
			f16(lib->version);
			lib = lib->next;
		}
	}
	
	/* Write imports */
	if (impupto) {
		fputc(IMPORT, fp);
		f32(impsize);
		f16(impupto);
		while (lab) {
			if (lab->seg == -1) {
				fputs(lab->name, fp);
				fputc(0, fp);
			}
			lab = lab->nimp;
		}		
	}
		
	/* Write segments */
	calc = totsegs * 14 + 2;
	upto = &outseg[0];
	for (i=0;i<totsegs;i++) {
		cseg = *upto;
		if (!(cseg->flags&S_BLANK)) {
			if (cseg->relsize && !norel)
				calc += cseg->relsize+1;
			calc += cseg->totsize;
		}
		upto++;
	}
	fputc(SEGMENTS, fp);
	f32(calc);
	f16(totsegs);
	upto = &outseg[0];
	for (i=0;i<totsegs;i++) {
		cseg = *upto;
		f32(cseg->startpc);
		f32(cseg->totsize);
		if (norel)
			calc=0;
		else {
			if (calc = cseg->relsize)
				calc++;
		}
		f32(calc);
		f16(cseg->flags);
		upto++;
	}
	
	just64:
	upto = &outseg[0];
	for (i=0;i<totsegs;i++) {
		cseg = *upto;
		if (!i && formatc64)
			fwrite(&cseg->startpc, 1, 2, fp);
		if (!(cseg->flags&S_BLANK)) {
			while(cseg) {
				fwrite(cseg->data, 1, cseg->size, fp);
				cseg = cseg->nextm;
			}
			if (!formatc64 && !norel) {
				cseg = *upto;
				if (cseg->relsize) {
					fwrite(cseg->rel, 1, cseg->relsize, fp);
					fputc(0, fp);
				}
			}
		}
		upto++;
	} 
		
	/* Write globals */
	
	if (!formatc64 && !noglobs && numexp) {
		fputc(EXPORT, fp);
		f32(expsize);
		f16(numexp);
		lab = hundef;
		while (lab) {
			if (lab->export) {
				fputs(lab->name, fp);
				fputc(0, fp);
				fputc(lab->seg, fp);
				f32(lab->val);
			}
			lab = lab->nimp;
		}		
	}
	if (!formatc64)
		fputc(ENDFILE, fp);
	fclose(fp);

}

void dopack() {
	LSegment **upto;
	LSegment *cseg;
	uint32 pcupto;
	uint32 dif;
	int gotfirpc=0;
	uint i;

	upto = &outseg[0];
	for (i=0;i<totsegs;i++) {
		cseg = *upto;
		if (pack == 2 || cseg->flags&S_DBR) {
			if (!gotfirpc) {
				gotfirpc = 1;
				pcupto = cseg->startpc;
			} else {
				dif = pcupto - cseg->startpc;
				while(cseg) {
					cseg->reloff += dif;
					cseg->startpc += dif;
					cseg = cseg->nextm;
				}
			}
			cseg = *upto;
			pcupto += cseg->totsize;
		}
		upto++;
	} 
	
}

void usage() {
	printf("ld65: link 'o65' files\n"
		"ld65 [options] [filenames...]\n"
		"options:\n"
		"  -h, -?    = print this help\n"
		"  -o file   = uses 'file' as output file. otherwise write to 'a.o65'.\n"
		"  -G        = suppress writing of globals\n"
		"  -R        = suppress writing of reloc entries\n"
		"  -d        = pack data bank (data+bss)\n"
		"  -p        = pack all together\n"
		"  -e        = output C64 format\n"
		"  -l<lib>   = include library\n"
		"  -y        = use dynamic linking\n"
		"  -s<ver>   = make shared library with version ver\n"
		"  -t size   = Stack size\n"
		"  -f number = Set file flags (0x02 STAY)\n"
	);
	exit(1);
}

int main(int argc, char *argv[]) {
	int ch;
	char *ver;
	uint libver;
	
	if (argc<2)
		usage();
	while((ch = getopt(argc, argv, "f:h?depo:RGl:ys:t:")) != EOF) {
		switch(ch) {
			case 'o':
				outfile = optarg;
				break;
			case 'h':
			case '?':
				usage();
			case 'l':
				ver = strrchr(optarg, ':');
				if (ver) {
					*ver = 0;
					libver = strtol(ver+1, NULL, 16);
				} else libver=0x100;
				addlink(optarg, libver);
				break;
			case 'f':
				flags |= strtol(optarg, NULL, 0);
				break;
			case 'd':
				pack=1;
				break;
			case 'p':
				pack=2;
				break;
			case 'G':
				noglobs = 1;
				break;
			case 'R':
				norel = 1;
				break;
			case 'e':
				formatc64 = 1;
				break;
			case 'y':
				dynamic = 1;
				break;
			case 't':
				stacksize = strtol(optarg, NULL, 0);
				break;
			case 's':
				flags |= O_LIBRARY;
				version = strtol(optarg, NULL, 0);
				break;
				
		}
	}
	while (optind<argc) {
		linkfile(argv[optind]);
		optind++;
	}
	if (pack)
		dopack();
	reloc();
	outputit();
}
