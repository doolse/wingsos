#include <stdio.h>
#include <fcntl.h>
#include <wgsipc.h>
#include <stdlib.h>
#include <wgslib.h>
#include <termio.h>
#include <winlib.h>

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

typedef struct data_s {
  int pauseflag;
} pausestruct;

struct termios tio;

void playthread();
void loadthread();

RifForm Format;

struct wmutex pausemutex = {-1, -1};
struct wmutex buf0mutex  = {-1, -1};
struct wmutex buf1mutex  = {-1, -1};

char * playbuf, *buf;
char * songname;
void * textbar;
long inread;
int  numofsongs;
int  lastbuffer = 3;

char **allsongs;

void mysleep(int seconds) {
  int channel;
  char * MsgP;

  channel = makeChan();
  setTimer(-1, seconds*1000, 0, channel, PMSG_Alarm);
  recvMsg(channel,(void *)&MsgP);
}

void pauseplay(void * button) {
  pausestruct * pauses;

  pauses = JWGetData(button);

  if(!pauses->pauseflag) {
    pauses->pauseflag = 1;
    getMutex(&pausemutex);    
  } else {
    pauses->pauseflag = 0;
    relMutex(&pausemutex);
  }
}

void main(int argc, char *argv[]) {
  void * app,*wnd, *pausebut;
  pausestruct * pauses;
	
  if (argc < 2) {
    fprintf(stderr,"Usage: wpth file1.wav [file2.wav file3.wav ...]\n");
    exit(1);
  }

  pauses = (pausestruct *)malloc(sizeof(pausestruct));
  pauses->pauseflag = 0;

  numofsongs = argc;
  allsongs   = argv; 

  app = JAppInit(NULL, 0);
  wnd = JWndInit(NULL, "Wave Player", JWndF_Resizable);

  JWSetBounds(wnd, 8,8, 96, 24);
  JAppSetMain(app,wnd);

  ((JCnt *)wnd)->Orient = JCntF_TopBottom;

  textbar = JTxfInit(NULL);
  pausebut = JButInit(NULL, "play/pause");

  JCntAdd(wnd, textbar);
  JCntAdd(wnd, pausebut);

  JWSetData(pausebut, pauses);

  JWinCallback(pausebut, JBut, Clicked, pauseplay);

  retexit(1);

  newThread(loadthread, STACK_DFL, NULL);
  newThread(playthread, STACK_DFL, NULL);

  JWinShow(wnd);
  JAppLoop(app);
}

void loadthread() {
  int Channel;
  char * MsgP;
  FILE    *fp; 
  Riff    RiffHdr;
  RChunk  Chunk;
  int     done=0;
  int     song;
  int buffer = 0;

  for(song=1;song<numofsongs;song++) {
    if(buffer)
      getMutex(&buf1mutex);
    else
      getMutex(&buf0mutex);

    //printf("%s\n", allsongs[song]);

    fp = fopen(allsongs[song], "rb");
    if(fp) {
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

      buf = malloc(Chunk.ChSize);

      inread = fread(buf, 1, Chunk.ChSize, fp);

      songname = allsongs[song];

      if(song+1 == numofsongs)
        lastbuffer = buffer;

      if(buffer) {
        relMutex(&buf1mutex);
        buffer = 0;
      } else {
        relMutex(&buf0mutex);
        buffer = 1;
      }
    }
  }

  //TRY REMOVING THIS ... 
  Channel = makeChan();
  recvMsg(Channel,(void *)&MsgP);
}

void playthread() {
  long lengthleftplaying, minutes, seconds;
  int amount;
  int digiChan;
  RifForm playformat;
  char * bufstart;  
  char * string;
  char * playingsongname;
  int buffer = 0;

  //Assuming 8bit mono for the remaining minutes:seconds counter

  string = (char *)malloc(30);  

  digiChan = open("/dev/mixer",O_READ|O_WRITE);

  if (digiChan == -1) {
    fprintf(stderr,"Digi device not loaded\n");
    exit(1);
  }

  while(1) {

    JTxfSetText(textbar, "Loading...");

    if(buffer)
      getMutex(&buf1mutex);
    else
      getMutex(&buf0mutex);

    playingsongname = songname;

    memcpy(&playformat, &Format, sizeof(RifForm));

    lengthleftplaying = inread;
    bufstart = playbuf = buf;

    sendCon(digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) playformat.SampRate, 1, 2);
    //printf("Changing samplerate to %d\n",(unsigned int)playformat.SampRate);

    mysleep(1);

    while (lengthleftplaying > 0) {
      getMutex(&pausemutex);

      if (lengthleftplaying > playformat.SampRate)
        amount = playformat.SampRate;
      else 
        amount = lengthleftplaying;

      write(digiChan, playbuf, amount);

      minutes = (lengthleftplaying/playformat.SampRate)/60;
      seconds = (lengthleftplaying/playformat.SampRate) - (minutes * 60);

      if(seconds < 10)
        sprintf(string, "%s %ld:0%ld",playingsongname, minutes, seconds);
      else
        sprintf(string, "%s %ld:%ld",playingsongname, minutes, seconds);

      JTxfSetText(textbar, string);

      playbuf += amount;
      lengthleftplaying -= amount;

      relMutex(&pausemutex);
    }
    free(bufstart);

    if(lastbuffer == buffer)
      break;

    if(buffer) {
      relMutex(&buf1mutex);
      buffer = 0;
    } else {
      relMutex(&buf0mutex);
      buffer = 1;
    }
  }

  close(digiChan);
  JTxfSetText(textbar, "Playlist Finished.");
}
