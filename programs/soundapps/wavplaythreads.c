#include <stdio.h>
#include <fcntl.h>
#include <wgsipc.h>
#include <stdlib.h>
#include <wgslib.h>

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

void playthread();

RifForm Format;

char * playbuf;
int  playing = 0;
long inread2;
int  digiChan;

void main(int argc, char *argv[]) {
	
  FILE    *fp; 
  Riff    RiffHdr;
  RChunk  Chunk;
  char    *buf;
  long    inread;
  int     done=0;
  int     song;
	
  if (argc<2) {
    fprintf(stderr,"Usage: wavplay wavfile\n");
    exit(1);
  }
 
  digiChan = open("/dev/mixer",O_READ|O_WRITE);

  if (digiChan == -1) {
    fprintf(stderr,"Digi device not loaded\n");
    exit(1);
  }

  for(song=1;song<(argc);song++) {
    fp = fopen(argv[song], "rb");
    if (fp) {
      fread(&RiffHdr,1,sizeof(Riff),fp);

      if (RiffHdr.RiffIdent[0]!='R' || RiffHdr.RiffIdent[1]!='I') {
        fprintf(stderr,"Not a wav file!\n");
        break;
      }

      while (!done) {
        fread(&Chunk,1,sizeof(RChunk),fp);

        if (Chunk.Ident[0]=='f' && Chunk.Ident[1]=='m') {
          fread(&Format,1,Chunk.ChSize,fp);
        } else if (Chunk.Ident[0]=='d')
          done=1;
	else {
          fseek(fp,Chunk.ChSize,SEEK_CUR);
        }
      }

      printf("Sample rate: %ld\n", Format.SampRate);
      printf("Sample size: %ld\n", Chunk.ChSize);

      buf = malloc(Chunk.ChSize);

      if(buf == NULL) {
        printf("Sorry, you ran out of ram.\n");
        exit(1);
      } else {
        inread = fread(buf, 1, Chunk.ChSize, fp);
        while(playing); //HIDEOUS fix this asap!!
        inread2 = inread;
        playbuf = buf;
        newThread(playthread, STACK_DFL, NULL);
        playing = 1;
        done    = 0;
        buf     = NULL;
        printf("Playing song %d, loading song %d.\n", song, song+1);
      }  
    } 	
  }
  while(playing);
}

void playthread() {
  int amount;
  char * bufstart;  
  
  bufstart = playbuf;

  sendCon(digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) Format.SampRate, 1, 2);

  while (inread2) {
    if (inread2 > 32767)
      amount = 32767;
    else amount = inread2;
      write(digiChan, playbuf, amount);

    playbuf += amount;
    inread2 -= amount;
  }
  free(bufstart);
  playing = 0;
}
