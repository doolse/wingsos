#include <stdio.h>
#include <fcntl.h>
#include <wgsipc.h>
#include <stdlib.h>

#define RCHNK_FORMAT	1
#define RCHNK_DATA	2

typedef struct riff {
	char RiffIdent[4];
	long TotalSize;
	char RiffType[4];
} Riff;

typedef struct rchunk {
	char Ident[4];
	long ChSize;
} RChunk;

typedef struct rform {
	int Chan1;
	int Chan2;
	long SampRate;
	long ByteSec;
	int ByteSamp;
	int BitSamp;
	int Unused;
} RifForm;

void main(int argc, char *argv[]) {
	
	int digiChan;
	FILE *fp; 
	Riff RiffHdr;
	RChunk Chunk;
	RifForm Format;
	char *buf;
	int done=0;
	int amount;
	long inread;
	
	if (argc<2) {
		fprintf(stderr,"Usage: wavplay wavfile\n");
		exit(1);
	}
	digiChan = open("/dev/mixer",O_READ|O_WRITE);
	if (digiChan == -1) {
		fprintf(stderr,"Digi device not loaded\n");
		exit(1);
	}
	fp = fopen(argv[1], "rb");
	if (fp) {
		fread(&RiffHdr,1,sizeof(Riff),fp);
		if (RiffHdr.RiffIdent[0]!='R' || RiffHdr.RiffIdent[1]!='I') {
			fprintf(stderr,"Not a wav file!\n");
			exit(1);
		}
		while (!done) {
			fread(&Chunk,1,sizeof(RChunk),fp);
			if (Chunk.Ident[0]=='f' && Chunk.Ident[1]=='m')
			{
				fread(&Format,1,Chunk.ChSize,fp);
			}
			else if (Chunk.Ident[0]=='d')
				done=1;
			else {
				fseek(fp,Chunk.ChSize,SEEK_CUR);
			}
		}
		printf("Sample rate: %ld\n", Format.SampRate);
		printf("Sample size: %ld\n", Chunk.ChSize);
		sendCon(digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) Format.SampRate, 1, 2);
		buf = malloc(Chunk.ChSize);
		if (buf) {
			inread = fread(buf, 1, Chunk.ChSize, fp);
			while (inread) {
				if (inread > 32767)
					amount = 32767;
				else amount = inread;
				write(digiChan, buf, amount);
				buf += amount;
				inread -= amount;
			}
		} else perror("wavplay"); 
	} else perror("wavplay");	
}
