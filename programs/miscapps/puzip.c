#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void putdec(FILE *fp, unsigned long a) {
    static const long dec[] = {1000000000,100000000,10000000,1000000,100000,10000,1000,100,10,1};
    int i, s = 0;
    for (i=0; i<10; i++) {
	int j = 0;
	while (a >= dec[i]) {
	    j++;
	    a -= dec[i];
	}
	if (j || s || i==9) {
	    fprintf(fp, "%c", j + '0');
	    s = 1;
	}
    }
}


/* ========================================================================
 * Table of CRC-32's of all single-byte values (made by makecrc.c)
 */
unsigned long crc_32_tab[] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
  0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
  0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL, 0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
  0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
  0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
  0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
  0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L, 0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
  0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
  0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
  0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
  0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L, 0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
  0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
  0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
  0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
  0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L, 0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
  0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
  0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
  0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
  0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L, 0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
  0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
  0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
  0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
  0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL, 0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
  0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
  0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
  0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};


#define updcrc(cp, crc) (crc_32_tab[((int)crc ^ cp) & 255] ^ (crc >> 8))


/****************************************************************
    Tables for deflate from PKZIP's appnote.txt
 ****************************************************************/

/* Order of the bit length code lengths */
static const unsigned border[] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* Copy lengths for literal codes 257..285 */
static const unsigned short cplens[] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 999, 0};

/* Extra bits for literal codes 257..285 */
static const unsigned short cplext[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99}; /* 99==invalid */

/* Copy offsets for distance codes 0..29 */
static const unsigned short cpdist[] = {
    0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d,
    0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1,
    0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01,
    0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001, 0xffff};

/* Extra bits for distance codes */
static const unsigned short cpdext[] = {
     0,  0,  0,  0,  1,  1,  2,  2,
     3,  3,  4,  4,  5,  5,  6,  6,
     7,  7,  8,  8,  9,  9, 10, 10,
    11, 11, 12, 12, 13, 13};


/****************************************************************/


struct datadesc {
    unsigned long crc32;
    unsigned long csize;
    unsigned long usize;
};

struct central {
    struct datadesc dd;
    unsigned long lhdroff;
    char *name;
};

#define MAXFILES 256
struct central CA[MAXFILES];

struct ecentral {
    unsigned char id[4]; /* 50 4b 05 06 */
    unsigned char numdisk[2];
    unsigned char cendisk[2];
    unsigned char numthis[2];
    unsigned char censize[4];
    unsigned char censtart[4];
    unsigned char commentsize[2];
};


static unsigned long outSize = 0;
static int bits = 0, byte = 0;
void FlushBits(FILE *fp) {
    if (bits) {
	while(bits!=8) {
	    byte = (byte>>1);
	    bits++;
	}
	fputc(byte, fp);
	outSize++;
    }
    bits = 0;
}
void PutBits(FILE *fp, int num, int val) {
    if (num > 8) {
	fprintf(stderr, "PutBits(fp, %d, %d)!\n", num, val);
    }
    while (num--) {
	bits++;
	byte = (byte>>1) | ((val & 1)?0x80:0);
	val >>= 1;
	if (bits==8) {
	    fputc(byte, fp);
	    bits = 0;
	    outSize++;
	}
    }
}
void PutBitsR(FILE *fp, int num, int val) {
    static const short maskTab[] = {1,2,4,8,16,32,64,128 ,256,512};
    short mask = maskTab[num];
    if (!num)
	return;
    mask = maskTab[num-1];
    if (num > 8) {
	fprintf(stderr, "PutBitsR(fp, %d, %d)!\n", num, val);
    }
    while (num--) {
	byte = (byte>>1) | ((val & mask)?0x80:0);
	mask >>= 1;
	bits++;
	if (bits==8) {
	    fputc(byte, fp);
	    bits = 0;
	    outSize++;
	}
    }
}


/*
    256	0000000		0
    :   :     :
    279	0010111		23
    0   00110000	48
    :	:      :
    143	10111111	191
    280 11000000	192
    :	:      :
    287 11000111	199
    144	110010000	400
    :	:       :
    255	111111111	511

    Note the bit order!
 */
void PutFixed(FILE *fp, int sym) {
    if (sym >= 256 && sym <= 279) {
	PutBitsR(fp, 7, sym - 256 + 0);
    } else if (sym >= 280 && sym <= 287) {
	PutBitsR(fp, 8, sym -280+192);
    } else if (sym >= 0 && sym <= 143) {
	PutBitsR(fp, 8, sym - 0 + 48);
    } else if (sym >= 144 && sym <= 255) {
	PutBits(fp, 1, 1);
	PutBitsR(fp, 8, sym);
    } else {
	fprintf(stderr, "invalid fixed huffman\n");
    }
}



#define MAX_MATCH 255 /* 258 */

#define BLOCK_SIZE 2048	/* must be a power of 2 */
static unsigned short bSkip[BLOCK_SIZE];

#define LSHIFT  5		/* 4-74s 5-55s 6-45s 7-38s 8-34s */
#define LSIZE   (1<<LSHIFT)
static long lPair[LSIZE * LSIZE];
static unsigned char buffer[BLOCK_SIZE + MAX_MATCH];


/*
		2048x5	4096x5	4096x6	8192x6
	crc32	1k	1k	1k	1k
	lPair	4*1k	4*1k	4*4k	4*4k
	bSkip	2*2k	2*4k	2*4k	2*8k
	buffer	2k	4k	4k	8k
		-----------------------------
		11k	17k	29k	41k
	outBuf	33k	27k	15k	 3k
		-----------------------------
		44k	44k	44k	44k	($2000-$cfff)
*/



int main(int argc, char *argv[]) {
    FILE *fp, *fpout = stdout;
    int i, files = 0, skip, eof, stored = 0;
    unsigned long crc32, cstart, csize, usize;
    long p;
    unsigned short bu;
    unsigned char *wPtr, *rPtr;

    for (i=1; i<argc; i++) {
	if (fp = fopen(argv[i], "rb")) {
	    char temp[4];
	    int len = strlen(argv[i]);

	    CA[files].name = argv[i];
	    CA[files].lhdroff = outSize;

	    fwrite("\x50\x4b\x03\x04", 4, 1, fpout);
	    fwrite("\x14\x01\x08\x00", 4, 1, fpout); /* vers, gpflag */
	    fwrite("\x08\x00", 2, 1, fpout); /* method -- deflate */

	    fwrite("\x9f\x05\x00\x00", 4, 1, fpout); /* modtime/date */
	    fwrite("\x00\x00\x00\x00", 4, 1, fpout); /* CRC32 */
	    fwrite("\x00\x00\x00\x00", 4, 1, fpout); /* compressed size */
	    fwrite("\x00\x00\x00\x00", 4, 1, fpout); /* uncompressed size */
	    temp[0] = len & 0xff;
	    temp[1] = (len >> 8);
	    temp[2] = temp[3] = '\0';
	    fwrite(temp, 4, 1, fpout);		/* filename length */
	    fwrite(argv[i], len, 1, fpout);	/* filename */

	    outSize += 30 + len;

	    crc32 = ~0;
	    usize = 0;
	    cstart = outSize;

	    if (stored) {
		while (1) {
		    int j;

		    len = fread(buffer, 1, BLOCK_SIZE, fp);
		    PutBits(fpout, 1, (len<BLOCK_SIZE)); /* last block? */
		    PutBits(fpout, 2, 0); /* stored */
		    FlushBits(fpout);

		    temp[0] = len;
		    temp[1] = len>>8;
		    temp[2] = ~temp[0];
		    temp[3] = ~temp[1];
		    fwrite(temp, 1, 4, fpout);
		    fwrite(buffer, 1, len, fpout);

		    usize += len;
		    outSize += 4 + len;

		    for (j=0; j<len; j++) {
			crc32 = updcrc(buffer[j], crc32);
		    }
		    if (len < BLOCK_SIZE)
			break;
		}
		goto fileEnd;
	    }

	    PutBits(fpout, 1, 1); /* last block */
	    PutBits(fpout, 2, 1); /* fixed huffman */

	    memset(lPair, 0, sizeof(lPair));
	    memset(bSkip, 0, sizeof(bSkip));

	    eof = 0;
	    skip = 0;
	    p = 0;
	    bu = 0;
	    wPtr = rPtr = &buffer[0];
	    while (1) {
		/* if short of lookahead, fill buffer */
		while (!eof && bu < MAX_MATCH) {
		    int c = fgetc(fp);
		    if (c == EOF) {
			eof = 1;
			break;
		    } else {
			*wPtr = c;
			if (wPtr < &buffer[MAX_MATCH]) {
			    *(wPtr + BLOCK_SIZE) = c;
			}
			wPtr++;
			if (wPtr == &buffer[BLOCK_SIZE]) {
			    wPtr = &buffer[0];
			}
			bu++;
			crc32 = updcrc(c, crc32);
		    }
		}
		if (bu > 1) {
		    long off;
		    unsigned short index =
				((*rPtr & (LSIZE-1)) << LSHIFT) |
				(*(rPtr+1) & (LSIZE-1));
		    long *lPairPtr = &lPair[index];
		    unsigned short *tmp = &bSkip[p & (BLOCK_SIZE-1)];

		    off = p - *lPairPtr;

		    if (off < (BLOCK_SIZE-MAX_MATCH)) {
			*tmp = off;
		    } else {
			*tmp = 0;
		    }
		    *lPairPtr = p;
		}
		if (skip) {
		    skip--;
		} else {
		    short maxlen = 1, maxpos = 0, j;
		    int left = BLOCK_SIZE - MAX_MATCH, loops = 255;
		    unsigned short im = p & (BLOCK_SIZE-1);

		    while ((j = bSkip[im])) {
			char *bp = (char *) rPtr;
			char *ap;

			if (!--loops)
			    break;

			left -= j;
			if (left < 0) {
			    break;
			}
			im = (im - j) & (BLOCK_SIZE-1);

			ap = (char *) &buffer[im];
			j = 0;
			while (*ap++ == *bp++ && ++j < MAX_MATCH)
			    ;
			if (j > maxlen) {
			    maxlen = j;
			    maxpos = (int)(bp - ap) & (BLOCK_SIZE-1);

			    if (maxlen == MAX_MATCH ||
				*rPtr == *(rPtr+1)) {
				break;
			    }
			}
		    }
		    if (maxlen > bu) {
			maxlen = bu;
		    }
		    if (maxlen > 2) {
/*fprintf(stderr, "%d loops\n", loops);*/
			j = 0;
			while (maxlen >= cplens[j])
			    j++;
			j--;
#if 0
fprintf(stderr, "@%d (l%d,d%d) %d +%d ", p, maxlen, maxpos, 257+j, cplext[j]);
#endif
			PutFixed(fpout, 257+j);
			if (cplext[j]) {
			    PutBits(fpout, cplext[j], maxlen-cplens[j]);
			}
			j = 0;
			while (maxpos >= cpdist[j])
			    j++;
			j--;
#if 0
fprintf(stderr, "%d +%d\n", j, cpdext[j]);
#endif
			PutBitsR(fpout, 5, j);
			if (cpdext[j]) {
			    if (cpdext[j] > 8) {
				PutBits(fpout, 8, maxpos-cpdist[j]);
				PutBits(fpout, cpdext[j]-8, (maxpos-cpdist[j])>>8);
			    } else {
				PutBits(fpout, cpdext[j], maxpos-cpdist[j]);
			    }
			}
			skip = maxlen-1;
		    } else {
#if 0
fprintf(stderr, "%02x\n", *rPtr);
#endif
			PutFixed(fpout, *rPtr);
		    }
		}

		p++;
		rPtr++;
		if (rPtr == &buffer[BLOCK_SIZE]) {
		    rPtr = &buffer[0];
		}

		bu--;
		if (!bu && eof) {
		    break;
		}
		if (!(p & 4095)) {
		    fprintf(stderr, "\r%d", p);
		    fflush(stderr);
		}
	    }
	    usize = p;
	    fprintf(stderr, "\r%d\n", p);
	    PutFixed(fpout, 256); /* EOF */
	    FlushBits(fpout);
fileEnd:
	    csize = outSize-cstart;

	    fprintf(stderr, "%d%%\n", 100*csize/usize);
	    if (usize) {
		long a = csize, b = usize, val = 0;
		int i, sh = 0;

		while (a > b) {
		    b <<= 1;
		    sh++;
		}
		for (i=0; i<17; i++) {
		    val <<= 1;
		    if (a >= b) {
			a -= b;
			val |= 1;
		    }
		    a <<= 1;
		}
		val <<= sh;
		putdec(stderr, csize);
		fprintf(stderr, " ");
		putdec(stderr, usize);
		fprintf(stderr, " ");
		putdec(stderr, 100*val/65536);
		fprintf(stderr, "%%\n");
	    }

	    temp[0] = ~crc32;
	    temp[1] = ~crc32>>8;
	    temp[2] = ~crc32>>16;
	    temp[3] = ~crc32>>24;
	    fwrite(temp, 4, 1, fpout);
	    temp[0] = csize;
	    temp[1] = csize>>8;
	    temp[2] = csize>>16;
	    temp[3] = csize>>24;
	    fwrite(temp, 4, 1, fpout); /* compressed size */
	    temp[0] = usize;
	    temp[1] = usize>>8;
	    temp[2] = usize>>16;
	    temp[3] = usize>>24;
	    fwrite(temp, 4, 1, fpout); /* uncompressed size */

	    outSize += 12;

	    CA[files].dd.crc32 = crc32;
	    CA[files].dd.csize = csize;
	    CA[files].dd.usize = usize;
	    fclose(fp);

	    files++;
	}
    }
    cstart = outSize;
    csize = 0;
    for (i=0; i<files; i++) {
	unsigned char temp[4];
	int len = strlen(CA[i].name);

	fwrite("\x50\x4b\x01\x02", 4, 1, fpout);
	fwrite("\x14\x01\x14\x01", 4, 1, fpout); /* vers, vers needed */
	fwrite("\x00\x00\x08\x00", 4, 1, fpout); /* gpflags, method -- deflate */

	fwrite("\x00\x00\x00\x00", 4, 1, fpout); /* modtime/date */
	temp[0] = ~CA[i].dd.crc32;
	temp[1] = ~CA[i].dd.crc32>>8;
	temp[2] = ~CA[i].dd.crc32>>16;
	temp[3] = ~CA[i].dd.crc32>>24;
	fwrite(temp, 4, 1, fpout);
	temp[0] = CA[i].dd.csize;
	temp[1] = CA[i].dd.csize>>8;
	temp[2] = CA[i].dd.csize>>16;
	temp[3] = CA[i].dd.csize>>24;
	fwrite(temp, 4, 1, fpout); /* compressed size */
	temp[0] = CA[i].dd.usize;
	temp[1] = CA[i].dd.usize>>8;
	temp[2] = CA[i].dd.usize>>16;
	temp[3] = CA[i].dd.usize>>24;
	fwrite(temp, 4, 1, fpout); /* uncompressed size */

	temp[0] = len & 0xff;
	temp[1] = (len >> 8);
	fwrite(temp, 2, 1, fpout); /* name len */
	temp[0] = temp[1] = 0;
	fwrite(temp, 2, 1, fpout); /* extra len */
	temp[0] = temp[1] = 0;
	fwrite(temp, 2, 1, fpout); /* comment len */

	temp[0] = temp[1] = 0;
	fwrite(temp, 2, 1, fpout); /* disk number start */
	temp[0] = temp[1] = 0;
	fwrite(temp, 2, 1, fpout); /* int attr */
	temp[0] = 0x01;
	temp[1] = 0x00;
	temp[2] = 0x00;
	temp[3] = 0x00;
	fwrite(temp, 4, 1, fpout); /* ext attr */

	temp[0] = CA[i].lhdroff;
	temp[1] = CA[i].lhdroff>>8;
	temp[2] = CA[i].lhdroff>>16;
	temp[3] = CA[i].lhdroff>>24;
	fwrite(temp, 4, 1, fpout); /* relative offset of local header */

	fwrite(CA[i].name, 1, len, fpout);
	csize += 46 + len;
    }
    if (files) {
	char temp[4];

	fwrite("\x50\x4b\x05\x06", 4, 1, fpout);
	fwrite("\x00\x00\x00\x00", 4, 1, fpout);

	temp[0] = files;
	temp[1] = files>>8;
	fwrite(temp, 2, 1, fpout);
	fwrite(temp, 2, 1, fpout);

	temp[0] = csize;
	temp[1] = csize>>8;
	temp[2] = csize>>16;
	temp[3] = csize>>24;
	fwrite(temp, 4, 1, fpout);

	temp[0] = cstart;
	temp[1] = cstart>>8;
	temp[2] = cstart>>16;
	temp[3] = cstart>>24;
	fwrite(temp, 4, 1, fpout);

	fwrite("\x00\x00", 2, 1, fpout);
    }
}
