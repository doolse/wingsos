#include <stdio.h>
#include <stdlib.h>
#include "getopt.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

typedef unsigned char uchar;

// typedef unsigned int uint;
typedef unsigned long uint32;
typedef unsigned short uint16;

enum {
IM_D64 = 0,
IM_D71, IM_D81,
IM_CMD, IM_IDE64
};

int disktype;
int cmdtracks;
int idecyls;
uchar *disk;
uint32 disklen;
uchar *g_dirent;
char *outname = NULL;
char *outdir = "";
char *remdir = "";
char *inname = NULL;
int verbose;
int strip;
uint rmdlen;
struct tm *g_tmbuf;

uchar *getBlock(uint track, uint sector)
{
	switch(disktype)
	{
	case IM_D81:
		return ((track-1) * 40 + sector)*256 + disk;
	case IM_CMD:
	    	return ((track-1) * 256 + sector)*256 + disk;
	}
}

uchar *get1581t(uint track)
{
	uchar *trk = disk + (256*40*39 + 256);
	track--;
	if (track >= 40)
	{
		track -= 40;
		trk += 256;
	}
	return trk + (track*6) + 16;
}

int allocD81(uint *ptrack, uint *psector)
{
    uint dir = 0;
    uchar *trk;
    uint track, sector;

    track = *ptrack;
    sector = *psector;

    while (dir < 2) {
	trk = get1581t(track);
	if (!*trk) {
	    if (track < 40) {
		if (!--track)
		{
		    track = 41;
		    ++dir;
		}
	    } else {
		if (++track > 80) {
		    track = 39;
		    ++dir;
		}
	    }
	} else {
	    uint bit = 1 << (sector&7) ;
	    uint byte = sector/8 + 1;
	    
    	    //printf("TrkBAM[0]=%d,[1]=%x,[2]=%x,[3]=%x,[4]=%x,[5]=%x\n", 
	    //        trk[0], trk[1], trk[2], trk[3], trk[4], trk[5]);
    	    //printf("Bit %d, byte %d\n", bit, byte);
	    
	    while (1) {
		if (trk[byte]&bit)
		{
		    --trk[0];
		    trk[byte] &= ~bit;
		    *ptrack = track;
		    *psector = sector;
    	    	    //printf("Alloced %d,%d\n", track, sector);
		    return 1;
		} else {
		    sector++;
		    bit <<= 1;
		    if (bit > 128) {
			bit = 1;
			if (++byte > 5) {
			    byte = 1;
			    sector = 0;
			}
		    }
		}
	    }
	}
	sector = 0;
    }
    return 0;
    
}

int allocCMD(uint *ptrack, uint *psector)
{
    uint i, len;
    uchar *trk;

    len = cmdtracks*32;
    trk = getBlock(1, 2)+0x20;
    i=0;
    while (i<len)
    {
	uint ch = trk[i];
	if (ch)
	{
	    uint bit = 0x80;
	    uint offs = i*8;
	    while (!(ch&bit))
	    {
		offs++;
		bit >>= 1;
	    }
	    trk[i] &= ~bit;
	    *ptrack = (offs/256)+1;
	    *psector = offs&255;
//	    printf("Alloced %d,%d\n", *ptrack, *psector);
	    return 1;
	}
	i++;
    }
    return 0;
}

int allocBlock(uint *ptrack, uint *psector)
{
    switch (disktype) 
    {
	case IM_D81:
	    return allocD81(ptrack, psector);
	case IM_CMD:
	    return allocCMD(ptrack, psector);
    }
}


/*
 * IDE64 images, in the CHS filesystem read by drivers/fsys/idefsys.a65
 * (the pre-CFS IDEDOS layout). 512 byte blocks, addressed linearly:
 *
 *   block 0       geometry, root dir and BAM pointers
 *   block 1..     BAM: one bit per block, LSB first, 1 = used
 *   cylinder 1    root directory, then files and dirs
 *
 * Every data/dir block starts with a 4 byte link: a non-last block holds
 * $80|cyl_hi, cyl_lo, head, sector of the next block; the last block holds
 * $C0|count_hi, count_lo, 0, 0 where count is the bytes used of the 508
 * byte payload. Dir blocks hold 15 32-byte entries from offset 8.
 */
#define IDE_HEADS	16
#define IDE_SECS	63
#define IDE_CYLSECS	(IDE_HEADS*IDE_SECS)
#define IDE_BLK		512
#define IDE_DATA	508
#define IDE_ENTS	15

uint32 idebams;

uchar *ideBlock(uint32 lba)
{
    return disk + lba*IDE_BLK;
}

void ideCHS(uint32 lba, uchar *out)
{
    uint cyl = lba / IDE_CYLSECS;
    out[0] = cyl >> 8;
    out[1] = cyl;
    out[2] = (lba % IDE_CYLSECS) / IDE_SECS;
    out[3] = lba % IDE_SECS + 1;
}

uint32 ideLBA(uchar *chs)
{
    uint cyl = ((chs[0] & 0x7f) << 8) | chs[1];
    return (uint32) cyl*IDE_CYLSECS + chs[2]*IDE_SECS + chs[3] - 1;
}

void ideUse(uint32 lba)
{
    ideBlock(1 + (lba>>12))[(lba>>3) & 511] |= 1 << (lba&7);
}

void ideFree(uint32 lba)
{
    ideBlock(1 + (lba>>12))[(lba>>3) & 511] &= ~(1 << (lba&7));
}

/* Entries written by this run, to tell a collision from a replacement */
uchar **idemade;
int idenmade;

/* First free block at or after the root directory */
uint32 ideAlloc()
{
    uint32 lba, total = (uint32) idecyls*IDE_CYLSECS;

    for (lba = IDE_CYLSECS; lba < total; lba++)
    {
	if (!(ideBlock(1 + (lba>>12))[(lba>>3) & 511] & (1 << (lba&7))))
	{
	    ideUse(lba);
	    memset(ideBlock(lba), 0, IDE_BLK);
	    return lba;
	}
    }
    fprintf(stderr, "IDE64 image full\n");
    exit(1);
}

void ideLast(uchar *blk, uint count)
{
    blk[0] = 0xc0 | (count >> 8);
    blk[1] = count;
    blk[2] = blk[3] = 0;
}

void ideLink(uchar *blk, uint32 next)
{
    ideCHS(next, blk);
    blk[0] |= 0x80;
}

void ideFormat()
{
    uchar *blk0 = ideBlock(0);
    uint32 i, total = (uint32) idecyls*IDE_CYLSECS;

    idebams = (total + 4095) / 4096;
    memcpy(blk0, "WINGS IMAGE     ", 16);
    blk0[0x10] = (idecyls-1) >> 8;
    blk0[0x11] = idecyls-1;
    blk0[0x12] = IDE_HEADS-1;
    blk0[0x13] = IDE_SECS;
    ideCHS(IDE_CYLSECS, &blk0[0x14]);	/* root */
    ideCHS(1, &blk0[0x18]);		/* BAM */
    for (i = 0; i <= idebams; i++)
	ideUse(i);
    ideUse(IDE_CYLSECS);
    ideLast(ideBlock(IDE_CYLSECS), IDE_DATA);
}

void ideTime(uchar *ent)
{
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);	/* idefsys stores UTC */

    ent[22] = tm->tm_sec;
    ent[23] = tm->tm_min;
    ent[24] = tm->tm_hour;
    ent[25] = tm->tm_mday;
    ent[26] = tm->tm_mon;
    ent[27] = tm->tm_year;
}

void ideName(uchar *ent, char *name, uint len)
{
    memset(ent+2, 0xa0, 16);
    memcpy(ent+2, name, len > 16 ? 16 : len);
}

int ideLive(uchar *ent)
{
    return (ent[0] & 0x80) && (ent[0] & 0x17);
}

/* A new directory's first block, with the "." and ".." entries */
uint32 ideNewDir(uint32 parent)
{
    uint32 lba = ideAlloc();
    uchar *blk = ideBlock(lba);

    ideLast(blk, IDE_DATA);
    blk[8] = 0xf0;
    ideName(blk+8, ".", 1);
    ideCHS(lba, blk+8+28);
    blk[40] = 0xd0;
    ideName(blk+40, "..", 2);
    ideCHS(parent, blk+40+28);
    return lba;
}

/*
 * Find "name" in the directory starting at "dir", or create it (as a
 * directory if isdir). Returns the entry.
 */
uchar *ideEntry(uint32 dir, char *name, uint len, int isdir)
{
    uchar *blk, *ent, *blank = NULL;
    uchar pad[16];
    uint32 first = dir;
    uint i;

    memset(pad, 0xa0, 16);
    memcpy(pad, name, len > 16 ? 16 : len);
    while (1)
    {
	blk = ideBlock(dir);
	for (i = 0; i < IDE_ENTS; i++)
	{
	    ent = blk + 8 + i*32;
	    if (!ideLive(ent)) {
		if (!blank)
		    blank = ent;
	    } else if (!memcmp(ent+2, pad, 16))
		return ent;
	}
	if (blk[0] & 0x40)
	    break;
	dir = ideLBA(blk);
    }
    if (!blank)
    {
	uint32 next = ideAlloc();
	ideLink(blk, next);
	ideLast(ideBlock(next), IDE_DATA);
	blank = ideBlock(next) + 8;
    }
    memset(blank, 0, 32);
    memcpy(blank+2, pad, 16);
    ideTime(blank);
    if (isdir) {
	blank[0] = 0x90;
	ideCHS(ideNewDir(first), blank+28);
    } else
	blank[0] = 0x82;
    return blank;
}

void ideSave(FILE *fp, uchar *ent)
{
    uchar *blk;
    uint32 lba, len = 0;
    uint amount;
    int ch;

    lba = ideAlloc();
    ideCHS(lba, ent+28);
    while (1)
    {
	blk = ideBlock(lba);
	amount = fread(blk+4, 1, IDE_DATA, fp);
	len += amount;
	/* Only non-last blocks may be full: peek for more data */
	if (amount < IDE_DATA || (ch = getc(fp)) == EOF) {
	    ideLast(blk, amount);
	    break;
	}
	ungetc(ch, fp);
	lba = ideAlloc();
	ideLink(blk, lba);
    }
    ent[18] = len;
    ent[19] = len >> 8;
    ent[20] = len >> 16;
    ent[21] = len >> 24;
    ideTime(ent);
}

/*
 * petname is a '/' separated path, already in disk (PETSCII) encoding.
 * Names are cut to 16 characters, which can make two names collide.
 */
int ideCreate(char *petname, FILE *fp, char *hostname)
{
    uint32 dir = IDE_CYLSECS;
    uchar *ent;
    char *sep;

    while (1)
    {
	while (*petname == '/')
	    petname++;
	sep = strchr(petname, '/');
	if (!sep)
	    break;
	if (sep-petname > 16)
	    printf("Warning: %s: name cut to 16 characters\n", hostname);
	ent = ideEntry(dir, petname, sep-petname, 1);
	if ((ent[0] & 0x17) != 0x10)
	    return 0;
	dir = ideLBA(ent+28);
	petname = sep+1;
    }
    if (!*petname)
	return 0;
    if (strlen(petname) > 16)
	printf("Warning: %s: name cut to 16 characters\n", hostname);
    ent = ideEntry(dir, petname, strlen(petname), 0);
    if ((ent[0] & 0x17) == 0x10)
	return 0;
    if (ent[31]) {
	/* Already has data: from the -i image, replace it; from this run,
	 * two names collided once cut to 16 characters. */
	uchar *blk;
	uint32 lba = ideLBA(ent+28);
	int i;

	for (i = 0; i < idenmade; i++)
	    if (idemade[i] == ent) {
		printf("%s: a file with the same (16 character) name exists\n", hostname);
		return 0;
	    }
	do {
	    blk = ideBlock(lba);
	    ideFree(lba);
	    lba = ideLBA(blk);
	} while (!(blk[0] & 0x40));
    }
    ideSave(fp, ent);
    idemade = realloc(idemade, (idenmade+1) * sizeof(uchar *));
    idemade[idenmade++] = ent;
    return 1;
}

void formatdisk()
{
	uint32 blocks;
	uchar *BAM;
	uint track;
	
	switch(disktype)
	{
	case IM_D81:
		blocks = 256*80*40;
		break;
	case IM_D64:
		blocks = 256*683;
		break;
	case IM_D71:
		blocks = 256*683*2;
		break;
	case IM_CMD:
	    	blocks = 256*cmdtracks*256;
		break;
	case IM_IDE64:
		blocks = (uint32) idecyls*IDE_CYLSECS*IDE_BLK;
		break;
	}
	disk = calloc(blocks, 1);
	disklen = blocks;
	if (inname)
	{
		FILE *fp = fopen(inname, "r");
		if (fp)
		{
			fread(disk, 1, disklen, fp);
			fclose(fp);
			return;
		}
	}
	switch(disktype)
	{
	case IM_D81:
	    BAM = getBlock(40,0);
	    BAM[0] = 40;
	    BAM[1] = 3;
	    BAM[2] = 'D';
	    BAM[0x100] = 40;
	    BAM[0x101] = 2;
	    BAM[0x201] = 0xff;
	    BAM[0x301] = 0xff;
	    memcpy(&BAM[4], "WINGS IMAGE     ", 16);
	    memset(&BAM[0x14], 0xA0, 11);
	    BAM[0x19] = '3';
	    BAM[0x1a] = 'D';
	    BAM[0x16] = '0';
	    BAM[0x17] = '2';

	    BAM = getBlock(40, 1);
	    BAM[2] = 'D';
	    BAM[3] = ~'D';
	    BAM[4] = '0';
	    BAM[5] = '2';
	    BAM[6] = 192;
	    for (track = 1; track <= 40; track++) {
		    uchar *tmp = BAM + 16 + (track - 1) * 6;
		    tmp[0] = (track == 40) ? 36 : 40;
		    tmp[1] = (track == 40) ? 0xf0 : 0xff;
		    tmp[2] = tmp[3] = tmp[4] = tmp[5] = 0xff;
	    }
	    BAM = getBlock(40, 2);
	    BAM[2] = 'D';
	    BAM[3] = ~'D';
	    BAM[4] = '0';
	    BAM[5] = '2';
	    BAM[6] = 192;
	    for (track = 41; track <= 80; track++) {
		    uchar *tmp = BAM + 16 + (track - 41) * 6;
		    tmp[0] = 40;
		    tmp[1] = tmp[2] = tmp[3] = tmp[4] = tmp[5] = 0xff;
	    }
	    break;
	case IM_CMD:
	    BAM = getBlock(1, 1);
	    // Free the bam area
	    memset(&BAM[0x120], 0xff, cmdtracks*32);
	    
	    // Allocate root+bam blocks
	    memset(&BAM[0x120], 0x00, 36/8);
	    BAM[0x120+36/8] &= ~0xf0;
	    BAM[0] = 0x01;
	    BAM[1] = 0x22;
	    BAM[0x108] = cmdtracks-1;
	    break;
	case IM_IDE64:
	    ideFormat();
	    break;
	case IM_D64:
		break;
	case IM_D71:
		break;
	}
	
}

void setTimestamp(uchar *dirent, struct tm *tmbuf)
{
    dirent[0x17] = tmbuf->tm_year;
    dirent[0x18] = tmbuf->tm_mon;
    dirent[0x19] = tmbuf->tm_mday;
    dirent[0x1a] = tmbuf->tm_hour;
    dirent[0x1b] = tmbuf->tm_min;
}


int saveFile(FILE *fp, uchar *dirent)
{
    uchar *blk = NULL;
    uint amount,track=41,sector=0;
    uint blks=0;

    do {
	if (!allocBlock(&track, &sector))
	    return 0;
	if (blk) {
	    blk[0] = track;
	    blk[1] = sector;
	} else {
	    dirent[1] = track;
	    dirent[2] = sector;
	}
	blk = getBlock(track, sector);
	amount = fread(blk+2, 1, 254, fp);
	blk[0] = 0;
	blk[1] = amount+1;
	++blks;
    }
    while (amount == 254);
    setTimestamp(dirent, g_tmbuf);
    *(uint16 *)(dirent+28) = blks;
    return 1;
}

int createFile(uint track, uint sector, char *petname)
{
    uchar *dirblk = getBlock(track, sector);
    uchar *dirent = dirblk+2;
    uchar *blank = NULL;
    char *sep;
    uint len,i,extras;

    again:
    sep = strchr(petname, '/');
    if (sep)
    {
	char *sep2;
	extras = 1;
	len = sep-petname + 1;
	*sep = 0xa0;
	sep2 = sep+1;
	while (*sep2 == '/') {
	    ++sep2;
	    ++extras;
	}
	if (len == 1) {
	    petname = sep2;
	    goto again;
	}
    } else {
	len = strlen(petname);
	if (!len)
	    return 0;
	petname[len] = 0xa0;
	len++;
    }
//	printf("Path %s,%d,%d\n", petname, track, sector);
    if (len > 16)
	    len = 16;
    while (dirblk[0]) {
	track = dirblk[0];
	sector = dirblk[1];
	dirblk = getBlock(track, sector);
	dirent = dirblk+2;
	for (i=0; i<8; i++) {
	    if (!dirent[0]) {
		    if (!blank)
			blank = dirent;
	    } else {
		if (!strncmp(&dirent[3], petname, len)) {
		    if (!sep)
			return 0;
		    return createFile(dirent[1], dirent[2], sep+extras);
		}
	    }
	    dirent += 32;
	}
    } 
//	printf("Blank %lx\n", blank);
    if (!blank)
    {
	if (allocBlock(&track, &sector)) {
	    dirblk[0] = track;
	    dirblk[1] = sector;
	    blank = getBlock(track, sector)+2;
	} else return 0;
    }
    memcpy(&blank[3], petname, len);
    memset(&blank[3+len], 0xa0, 16-len);
    if (!sep)
    {
	blank[0] = 0x82;
	g_dirent = blank;
	return 1;
    } else {
//		printf("Creating dir %s\n", petname);
	blank[0] = 0x86;
        setTimestamp(blank, g_tmbuf);
	track = 41;
	sector = 0;
	allocBlock(&track, &sector);
	dirblk = getBlock(track, sector);
	blank[1] = track;
	blank[2] = sector;
	allocBlock(&track, &sector);
	dirblk[0] = track;
	dirblk[1] = sector;
	return createFile(blank[1], blank[2], sep+1);
    }
}

void doFile(char *this)
{
    FILE *fp;
    uint track, sector, ch;

    struct stat buf;
    if (stat(this, &buf))
	    return;
    if (S_ISDIR(buf.st_mode)) {
	    struct dirent *dirent;
	    DIR *dir = opendir(this);
	    if (!dir)
		    return;
	    while (dirent = readdir(dir))
	    {
		char *new;
		if (dirent->d_name[0] != '.') {
		    new = malloc(strlen(this) + strlen(dirent->d_name) + 2);
		    strcpy(new, this);
		    strcat(new, "/");
		    strcat(new, dirent->d_name);
		    doFile(new);
		    free(new);
		}
	    }
	    closedir(dir);
	    return;
    }
    fp = fopen(this, "r");
    if (fp) {
	char *petname;
	char *str;

	petname = malloc(strlen(outdir)+strlen(this)+2);
	if (!strip) {
	    strcpy(petname, outdir);
	    strcat(petname, "/");
	    if (!strncmp(this, remdir, rmdlen))
		this += rmdlen;
	    strcat(petname, this);
	} else {
	    str = strrchr(this, '/');
	    if (!str)
		str = this;
	    else str++;
	    strcpy(petname, str);
	}
	str = petname;
	if (verbose)
		printf("Adding %s\n", petname);
	while (ch = *str)
	{
	    if (ch >= 'a' && ch <= 'z')
		    ch ^= 0x20;
	    else if (ch >= 'A' && ch <= 'Z')
	    {
		    ch ^= 0x80;
	    }
	    *str = ch;
	    str++;
	}
	if (disktype == IM_IDE64)
	{
	    if (!ideCreate(petname, fp, this))
	    {
		printf("Error writing %s\n", this);
		exit(1);
	    }
	    fclose(fp);
	    return;
	}
	switch (disktype)
	{
	    case IM_D81:
		track = 40;
		sector = 0;
		break;
	    case IM_CMD:
		track = 1;
		sector = 1;
		break;
	}
	if (!(createFile(track, sector, petname) && saveFile(fp, g_dirent))) 
	{
	    printf("Error writing %s\n", this);
	    exit(1);
	}
	fclose(fp);
    } else {
	    perror(this);
	    exit(1);
    }
}

void showhelp()
{
    printf("USAGE: mkimage [options] -o outname [files]\n"
	   "-v           be verbose\n"
	   "-t type      one of c=CMD, 8=d81\n"
	   "-s           strip path from input files\n"
	   "-r remdir    remove 'remdir' from the front of\n"
	   "             filenames\n"
	   "-i diskfile  name of an image file to read\n"
	   "-d dirname   name of a dir to save the files in\n"
	  );
    exit(1);
}

int main(int argc, char *argv[])
{
	FILE *fp;
	uint ch;
	time_t curtime;
	
	disktype = IM_D81;
	time(&curtime);
	g_tmbuf = localtime(&curtime);
	while ((ch = getopt(argc, argv, "ho:t:d:r:vsi:")) != EOF)
	{
		switch(ch)
		{
		case 'o':
			outname = optarg;
			break;
		case 't':
		    	if (optarg[0] == 'c')
			{
			    disktype = IM_CMD;
			    cmdtracks = atoi(&optarg[1]);
			}
			else if (optarg[0] == 'i')
			{
			    disktype = IM_IDE64;
			    idecyls = atoi(&optarg[1]);
			    if (idecyls < 2 || idecyls > 32767) {
				fprintf(stderr, "IDE64 cylinders must be 2-32767\n");
				exit(1);
			    }
			}
			break;
		case 'd':
			outdir = optarg;
			break;
		case 'r':
			remdir = optarg;
			rmdlen = strlen(remdir);
			break;
		case 'v':
			verbose = 1;
			break;
		case 's':
			strip = 1;
			break;
		case 'i':
			inname = optarg;
			break;
		case 'h':
		    	showhelp();
		}
			
	}
	if (!outname)
     	    showhelp();
	formatdisk();
	while (optind < argc) {
		doFile(argv[optind]);
		optind++;
	}
	fp = fopen(outname, "w");
	if (fp) {
		fwrite(disk, 1, disklen, fp);
		fclose(fp);
	} else {
		perror(outname);
		exit(1);
	}
	return 0;
}
