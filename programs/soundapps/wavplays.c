#include <stdio.h>
#include <fcntl.h>
#include <wgsipc.h>
#include <wgslib.h>
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

int digiChan,chan,amount;
char * bufptr, * msg;
RChunk Chunk;

void playit() {
  long inread = Chunk.ChSize;

  //delay 3 seconds for buffering
  setTimer(-1,3*1000,0,chan,PMSG_Alarm);
  recvMsg(chan,(void *)&msg);

  while (inread) {
    if (inread > 32766)
      amount = 32766;
    else 
      amount = inread;
    write(digiChan, bufptr, amount);
    bufptr += amount;
    inread -= amount;
  }
  //block until the entire wav is finished playing.
  write(digiChan,bufptr,1);  

  setTimer(-1,1,0,chan,PMSG_Alarm);
}

void main(int argc, char *argv[]) {
	FILE *fp; 
	Riff RiffHdr;
	RifForm Format;
	char *buf;
	int stream = 0,done=0;
	long inread;
	
	if (argc<2 || argc>3) {
		fprintf(stderr,"Usage: wavplay [-s] wavfile\n");
		exit(1);
	} else if(argc == 3) {
		if(argv[1][1] == 's')
			stream++;
		else {
			fprintf(stderr,"Usage: wavplay [-s] wavfile\n");
			exit(1);
		}
        }
       
	digiChan = open("/dev/mixer",O_READ|O_WRITE);
	if (digiChan == -1) {
		fprintf(stderr,"Digi device not loaded\n");
		exit(1);
	}
	fp = fopen(argv[1+stream], "rb");
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
		printf("Channels: %d\n", Format.Chan2);
		sendCon(digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) Format.SampRate, Format.Chan2, 2);
		bufptr = buf = malloc(Chunk.ChSize);
		if (buf) {
			if(stream) {
				chan = makeChan();
				newThread(playit,STACK_DFL,NULL);
			}
			inread = fread(buf, 1, Chunk.ChSize, fp);
			if(!stream) {
				while (inread) {
					if (inread > 32766)
						amount = 32766;
					else 
						amount = inread;
					write(digiChan, bufptr, amount);
					bufptr += amount;
					inread -= amount;
				}
				//block until the entire wav is finished playing.
				write(digiChan,bufptr,1);  
			} else 
				recvMsg(chan,(void *)&msg);
		} else perror("wavplay"); 
	} else perror("wavplay");	
}
