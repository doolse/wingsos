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
  printf("(p)ause, (r)esume, (v)isualize, (Q)uit\n");
  con_update();
}

RifForm Format;

struct wmutex playmutex  = {-1, -1};
struct wmutex pausemutex = {-1, -1};

char * playbuf;
long inread2;
int  digiChan;
int  pauseflag = 0;

//ToBe implemented... I was thinking a 4th Thread that reads a Globalvar for a number, and 
//puts an asterisk on screen, then linefeeds. Next loop, it grabs the next number, puts the next
//asterisk on the screen, and line feeds again. The result would create a waveform rotated 90 degrees
//that flows from top to bottom.

int  visual    = 0;

void main(int argc, char *argv[]) {
	
  FILE    *fp; 
  Riff    RiffHdr;
  RChunk  Chunk;
  char    *buf;
  long    inread;
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
  newThread(interfacethread, STACK_DFL, NULL);  

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
      done = 0;

      if(!visual) {
        printf("Sample rate: %ld\n", Format.SampRate);
        printf("Sample size: %ld\n", Chunk.ChSize);
      }

      buf = malloc(Chunk.ChSize);

      inread = fread(buf, 1, Chunk.ChSize, fp);
      getMutex(&playmutex);
      inread2 = inread;
      playbuf = buf;
      newThread(playthread, STACK_DFL, NULL);

      if(!visual)
        printf("Playing '%s', loading '%s'.\n", argv[song], argv[song+1]);
    } 	
  }
  //This getmutex is to prevent the program from quiting when the last
  //song is playing and there are no more to pre-load.

  getMutex(&playmutex);
}

void playthread() {
  int amount;
  char * bufstart;  
  
  bufstart = playbuf;

  sendCon(digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) Format.SampRate, 1, 2);

  while (inread2) {
    if(pauseflag == 1) {
      getMutex(&pausemutex);
      pauseflag = 0;
    }

    if (inread2 > 32767)
      amount = 32767;
    else amount = inread2;
      write(digiChan, playbuf, amount);

    playbuf += amount;
    inread2 -= amount;
  }
  free(bufstart);
  relMutex(&playmutex);
}

void interfacethread() {
  int inputchar;

  con_init();

  con_modeon(TF_ECHO);
  con_modeon(TF_ICRLF);
  con_modeoff(TF_ICANON);

  showmenu();

  inputchar = 0; 
  while(inputchar != 'Q'){
    inputchar = con_getkey();
    
    switch(inputchar) {
      case 'p':
        pauseflag = 1;
      break;
      case 'r':
        if(pauseflag == 1)
          relMutex(&pausemutex);
      break;
      case 'v':
        if(visual == 1)
          visual = 0;
        else {
          visual = 1;
          newThread(visualizethread, STACK_DFL, NULL);
        }
      break;
    }
  }
  con_end();
  con_clrscr();
  exit(1);
}

void visualizethread() {
  int i = 0;
  int j;
  unsigned int value, row;

  con_modeon(TF_ICANON);

  while(visual) {
    if(i > 30000)
      i = 0;
    value = (unsigned char)playbuf[i];
    row = ((value*24)/255);
    //printf("%d", value);
    
    for(j=0;j<25;j++) {
      con_gotoxy(0,j);
      if(j == row)
        putchar('*');
      else
        putchar(' ');
    }
    con_update();
    i += 3;
  }

  con_modeoff(TF_ICANON);
  showmenu();

}
