#include <stdio.h>
#include <fcntl.h>
#include <wgsipc.h>
#include <stdlib.h>
#include <wgslib.h>
#include <termio.h>

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

struct termios tio;

void playthread();
void interfacethread();
void visualizethread();

void showmenu() {
  //printf("(Q)uit\n");
  printf("(p)ause, (r)esume, (Q)uit\n");
  con_update();
}

RifForm Format;

struct wmutex playmutex  = {-1, -1};
struct wmutex loadmutex  = {-1, -1};
struct wmutex pausemutex = {-1, -1};

char * playbuf, *buf;
long inread2, inread;
int  digiChan;
int  pauseflag = 0;
int  listdone = 0;

//ToBe implemented... I was thinking a 4th Thread that reads a Globalvar for a number, and 
//puts an asterisk on screen, then linefeeds. Next loop, it grabs the next number, puts the next
//asterisk on the screen, and line feeds again. The result would create a waveform rotated 90 degrees
//that flows from top to bottom.

int  visual    = 0;

void main(int argc, char *argv[]) {
  int Channel;
  char * MsgP;
  FILE    *fp; 
  Riff    RiffHdr;
  RChunk  Chunk;
  int     done=0;
  int     song;
	
  if (argc < 2) {
    fprintf(stderr,"Usage: wpth file1.wav file2.wav file3.wav ...\n");
    fprintf(stderr,"In player: Q quits, p pauses, r resumes, v visual\n");
    exit(1);
  }
 
  digiChan = open("/dev/mixer",O_READ|O_WRITE);

  if (digiChan == -1) {
    fprintf(stderr,"Digi device not loaded\n");
    exit(1);
  }

  getMutex(&pausemutex);
  getMutex(&playmutex);
  newThread(interfacethread, STACK_DFL, NULL);  
  newThread(playthread, STACK_DFL, NULL);

  for(song=1;song<(argc);song++) {
    getMutex(&loadmutex);
    if(song > 1) {
      printf("Playing '%s', loading '%s'.\n", argv[song-1], argv[song]);
      con_update();
    }

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
      done = 0;

      if(!visual) {
        printf("Sample rate: %ld\n", Format.SampRate);
        printf("Sample size: %ld\n", Chunk.ChSize);
        con_update();
      }

      buf = malloc(Chunk.ChSize);

      inread = fread(buf, 1, Chunk.ChSize, fp);
      relMutex(&playmutex);
    } 	
  }

  getMutex(&loadmutex);
  listdone = 1;

  Channel = makeChan();
  recvMsg(Channel,(void *)&MsgP);
}

void playthread() {
  int amount;
  char * bufstart;  
  
  while(1) {
    getMutex(&playmutex);

    inread2 = inread;
    playbuf = buf;

    relMutex(&loadmutex);

    bufstart = playbuf;

    sendCon(digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) Format.SampRate, 1, 2);

    while (inread2) {
      if(pauseflag == 1)
        getMutex(&pausemutex);

      if (inread2 > 32767)
        amount = 32767;
      else amount = inread2;
        write(digiChan, playbuf, amount);

      playbuf += amount;
      inread2 -= amount;
    }
    free(bufstart);
    if(listdone)
      exit(-1);
  }
}

void interfacethread() {
  int inputchar;

  con_init();

  con_modeonoff(TF_ECHO | TF_ICRLF, TF_ICANON);

  showmenu();

  inputchar = 0; 
  while(inputchar != 'Q'){
    inputchar = con_getkey();
    
    switch(inputchar) {
      case 'p':
        pauseflag = 1;
      break;
      case 'r':
        if(pauseflag == 1) {
          relMutex(&pausemutex);
          pauseflag = 0;
        }
      break;
    }

  }
  con_end();
  con_clrscr();
  exit(-1);
}

