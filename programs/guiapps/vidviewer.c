#include <winlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define RCHNK_FORMAT    1
#define RCHNK_DATA      2

typedef struct framestr_s {
  void * frame;
} framestruct;

typedef struct mheader_s {
  //Media ID Characters; Should be WMOV
  char id[4];

  //Video Details
  ulong framecount;
  uint xsize;
  uint ysize;
  int framerate;
  int numofpreviewframes;
  int previewfps;
  
  //Audio Details
  long wavbytes;
  long sidbytes;
} mheader;

//WAVE Headers... taken from Jolz' wavplay

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

typedef struct movie_s {
  char * filename;

  //controls
  int reverse;
  int pause;
  int endofpreview;
  int playratepercent;
  int cframerate;
  int csamplerate;
  int cframe;

  //video info
  mheader wmovheader;
  long imgsize;
 
  //audio info
  int digiChan;
  RifForm Format;

} movie;

void getframes();
void showframes();
void dowavaudio();
void donoaudio();

//Global Variables

void *bmp, *bmpdata;
FILE * fp;
movie * themovie;

int replayflag;
int playbackchannel;
int audiochannel;

framestruct * preview;
framestruct * frames;
int  endofmovie;
long nextframe;

struct wmutex pausemutex = {-1, -1};

void getpreview();
void playpreview();

void mysleep(int seconds) {
  char *MsgP;
  int Channel, RcvID;

  Channel = makeChan();
  setTimer(-1, seconds*1000, 0, Channel, PMSG_Alarm);
  RcvID = recvMsg(Channel, (void *)&MsgP);
  replyMsg(RcvID,-1);
}

void changeframerate(void * frameadjust) {
  
  themovie->playratepercent = atoi(JTxfGetText(frameadjust));
  themovie->cframerate      = themovie->wmovheader.framerate * themovie->playratepercent / 100;

  if(themovie->wmovheader.wavbytes) {
    themovie->csamplerate = themovie->Format.SampRate * themovie->playratepercent / 100;  
    sendCon(themovie->digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) themovie->csamplerate, 1, 2);

    if(!themovie->pause) {
      getMutex(&pausemutex);
      mysleep(1);
      relMutex(&pausemutex);
    }
  }
}

void resumeplay() {
  if(replayflag) {
    setTimer(-1, 1, 0, playbackchannel, PMSG_Alarm);
    setTimer(-1, 1, 0, audiochannel,    PMSG_Alarm);
  }
}

void reverseplay(void * button) {
  movie * movieptr;

  movieptr = JWGetData(button);

  if(movieptr->reverse)
    movieptr->reverse = 0;
  else
    movieptr->reverse = 1;
}

void pauseplay(void * button) {
  movie * movieptr;

  movieptr = JWGetData(button);

  if(movieptr->pause) {
    movieptr->pause = 0;
    relMutex(&pausemutex);
  } else {
    movieptr->pause = 1;
    getMutex(&pausemutex);
  }
}

// *** THE MAIN FUNCTION ***

void main(int argc, char * argv[]) {
  void *app, *window, *scr, *view;
  void *controlcontainer, *reversebut, *pausebut, *playbut, *frameadjust;
  int xsize, ysize;

  if(argc < 2) {
    fprintf(stderr, "USAGE: vidviewer moviefile\n");
    exit(1);
  }

  fp = fopen(argv[1], "rb");
  if(!fp) {
    printf("the file '%s' could not be found.\n", argv[1]);
    exit(1);
  }

  // INITIALIZE THE MOVIE 
  themovie = (movie *)malloc(sizeof(movie));

  themovie->filename        = strdup(argv[1]);
  themovie->reverse         = 0;
  themovie->pause           = 0;
  themovie->endofpreview    = 0;
  themovie->playratepercent = 100;
  themovie->cframe          = 0;

  fread(&(themovie->wmovheader),sizeof(mheader),1,fp);
  // END OF MOVIE INIT


  //CHECK TO SEE IF THIS IS A VALID MOVIE

  if(!(themovie->wmovheader.id[0] == 'W' && 
       themovie->wmovheader.id[1] == 'M' && 
       themovie->wmovheader.id[2] == 'O' &&
       themovie->wmovheader.id[3] == 'V')) {
    printf("The file %s is not a valid WiNGs movie file.\n", argv[1]);
    exit(1);
  } else {
    printf("Valid movie file.\n");
  }

  app    = JAppInit(NULL,0);
  window = JWndInit(NULL, "Video Viewer", JWndF_Resizable);

  ((JCnt *)window)->Orient = JCntF_TopBottom;

  //CALCULATE MEMORY SIZE OF AN UNCOMPRESSED FRAME
  themovie->imgsize = (themovie->wmovheader.ysize*(themovie->wmovheader.xsize/8)) + ((themovie->wmovheader.xsize/8)*(themovie->wmovheader.ysize/8));

  //CALCULATE STARTING SIZE OF GUI WINDOW
  if(themovie->wmovheader.xsize > 296)
    xsize = 296;
  else
    xsize = themovie->wmovheader.xsize;

  if(themovie->wmovheader.ysize > 160)
    ysize = 160;
  else
    ysize = themovie->wmovheader.ysize;

  JWSetBounds(window, 0,0, xsize+8, ysize+24);
  JAppSetMain(app, window);

  frames = (framestruct *)malloc((themovie->wmovheader.framecount+1) * sizeof(framestruct));

  bmpdata = calloc(themovie->imgsize,1);
  bmp     = JBmpInit(NULL,(int)themovie->wmovheader.xsize,(int)themovie->wmovheader.ysize,bmpdata);
  view    = JViewWinInit(NULL, bmp);
  scr     = JScrInit(NULL, view, 0);
  controlcontainer = JCntInit(NULL);

  playbut     = JButInit(NULL, " Play ");
  reversebut  = JButInit(NULL, " <<->> ");
  pausebut    = JButInit(NULL, " Pause ");
  frameadjust = JTxfInit(NULL);

  JCntAdd(window, scr);
  JCntAdd(window, controlcontainer);

  ((JCnt *)controlcontainer)->Orient = JCntF_LeftRight;

  JCntAdd(controlcontainer, playbut);
  JCntAdd(controlcontainer, reversebut);
  JCntAdd(controlcontainer, pausebut);
  JCntAdd(controlcontainer, frameadjust);  

  JWSetData(pausebut,   themovie);
  JWSetData(reversebut, themovie);

  JWinCallback(playbut,     JBut, Clicked, resumeplay);
  JWinCallback(reversebut,  JBut, Clicked, reverseplay);
  JWinCallback(pausebut,    JBut, Clicked, pauseplay);
  JWinCallback(frameadjust, JTxf, Entered, changeframerate);

  JWinShow(window);

  printf("wave bytes: %ld\n", themovie->wmovheader.wavbytes);
  printf("sid bytes:  %ld\n", themovie->wmovheader.sidbytes);
  printf("framecount: %ld\n", themovie->wmovheader.framecount);
  printf("xsize:      %d\n",  themovie->wmovheader.xsize);
  printf("ysize:      %d\n",  themovie->wmovheader.ysize);
  printf("framerate:  %d\n",  themovie->wmovheader.framerate);

  retexit(1);

  getpreview();  

  if(themovie->wmovheader.wavbytes > 0) 
    newThread(dowavaudio, STACK_DFL, NULL); 
  else
    newThread(donoaudio, STACK_DFL, NULL);

  JAppLoop(app);
}

void donoaudio() {
  ulong totalsize;
  int fps;

  newThread(getframes, STACK_DFL, NULL);

/*
  totalsize = themovie->imgsize * themovie->wmovheader.framecount;

  fps = 1000 / themovie.cframerate;
  mysleep(themovie.cframerate * imgsize / )
*/

  //buncha bullshit I'll figure out later... sleep for 20 seconds.

  mysleep(20);
  themovie->endofpreview = 1;

  newThread(showframes, STACK_DFL, NULL);
}

void dowavaudio() {
  char *MsgP;
  int RcvID;

  Riff RiffHdr;
  RChunk Chunk;

  char *wavbuf, *wavbufptr;
  int done = 0;
  int amount;

  long totalsize, framesync;
  ulong syncbytes, inread;
  int hdrsize = 0;

  audiochannel = makeChan();

  themovie->digiChan = open("/dev/mixer",O_READ|O_WRITE);
  if (themovie->digiChan == -1) {
    printf("Digi device not loaded\n");
    exit(1);
  }

  fread(&RiffHdr,1,sizeof(Riff),fp);
  if (RiffHdr.RiffIdent[0]!='R' || RiffHdr.RiffIdent[1]!='I') {
    printf("Bad Wav Audio track!\n");
    exit(1);
  }

  hdrsize += sizeof(Riff);

  while (!done) {
    fread(&Chunk,1,sizeof(RChunk),fp);

    hdrsize += sizeof(RChunk);

    if (Chunk.Ident[0]=='f' && Chunk.Ident[1]=='m') {
      fread(&(themovie->Format),1,Chunk.ChSize,fp);
      hdrsize += Chunk.ChSize;
    } else if (Chunk.Ident[0]=='d')
      done=1;
    else 
      fseek(fp,Chunk.ChSize,SEEK_CUR);
  } 

  sendCon(themovie->digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) themovie->csamplerate, 1, 2);
  
  wavbufptr = wavbuf = malloc(themovie->wmovheader.wavbytes-hdrsize);
  totalsize = inread = fread(wavbuf, 1, themovie->wmovheader.wavbytes-hdrsize, fp);

  newThread(getframes, STACK_DFL, NULL);
  mysleep(20);
  themovie->endofpreview = 1;
  newThread(showframes, STACK_DFL, NULL);

  syncbytes  = themovie->wmovheader.wavbytes;
  syncbytes /= themovie->wmovheader.framecount;

  while(1) {
    framesync = 0;
  
    while (inread <= totalsize && inread > 0) {
      
      if (inread > syncbytes)
        amount = syncbytes;
      else 
        amount = inread;

      write(themovie->digiChan, wavbuf, amount);

      if(themovie->reverse)
        framesync -= 1;
      else
        framesync += 1;
      themovie->cframe = framesync;

      if(themovie->reverse) {
        wavbuf -= amount;
        inread += amount;
      } else {
        wavbuf += amount;
        inread -= amount;
      }
    }

    //block until the entire wav is finished playing.
    write(themovie->digiChan,wavbuf,0);

    RcvID = recvMsg(audiochannel, (void *)&MsgP);
    replyMsg(RcvID,-1);

    themovie->reverse = 0;

    //Reset buf pointer and inread size.
    wavbuf  = wavbufptr;
    inread  = totalsize;
  }
}

void showframes() {
  char *MsgP;
  int RcvID, timer;

  playbackchannel = makeChan();

  while(1) {

    replayflag = 0;

    timer = setTimer(-1, themovie->cframerate, 0, playbackchannel, PMSG_Alarm);

    while(1) {
      RcvID = recvMsg(playbackchannel, (void *)&MsgP);

      if(themovie->cframe < nextframe && themovie->cframe >= 0) {
        if(themovie->reverse)
          memcpy(bmpdata, frames[themovie->cframe--].frame, themovie->imgsize);
        else
          memcpy(bmpdata, frames[themovie->cframe++].frame, themovie->imgsize);
        JWReDraw(bmp);
      } else {
        if(endofmovie || themovie->reverse)
          break;
        printf("lost frame %d\n", themovie->cframe);
        mysleep(10);
      }

      replyMsg(RcvID,-1);
      timer = setTimer(timer,themovie->cframerate, 0, playbackchannel, PMSG_Alarm);
    }

    replayflag = 1;

    RcvID = recvMsg(playbackchannel, (void *)&MsgP);
    //replyMsg(RcvID,-1);

    themovie->reverse = 0;
    themovie->cframe = 0;
  }
}

void getframes() {
  char *MsgP;
  int RcvID, Channel;
  signed char count;
  long length;
  char *output;

  endofmovie = 0;
  nextframe = 0;

  while(1) {
    length = themovie->imgsize;
    output = frames[nextframe].frame = (void *)malloc(themovie->imgsize+4);
 
    /* unrle code  modified from example on www.compuphase.com */

    while (length>0) {
      count=(signed char)fgetc(fp);
      if (count>0) {
        //copied run
        memset(output,fgetc(fp),count);
      } else if (count<0) {
        //literal run
        count=(signed char)-count;
        if(!fread(output, 1, count, fp))
          break;
      } 
      output+=count;
      length-=count;
    } 

    if(length != 0) {
      printf("Stopped reading on frame %d, %d bytes read. %d\n", nextframe, abs(length-themovie->imgsize), length);
      fclose(fp);
      endofmovie = 1;
      Channel = makeChan();
      RcvID = recvMsg(Channel, (void *)&MsgP);
    } else if(nextframe == (themovie->wmovheader.framecount-1)) {
      nextframe = themovie->wmovheader.framecount;
      printf("Frame buffering complete.\n");
      fclose(fp);
      endofmovie = 1;
      Channel = makeChan();
      RcvID = recvMsg(Channel, (void *)&MsgP);
    }
    nextframe++;
  }
}

//the .prv file should have the same name as the .mov file.
//the .prv should in fact be a .rvd file, which contains 
//exactly 4 frames (rle compressed), that are the same size
//as is specified in the header of the .mov it goes with.
//the preview playback is fixed at 4 fps.

void getpreview() {
  int i, count, length;
  FILE * previewfp;
  char * output, * prvfilename;

  //change file extension from .mov to .prv

  prvfilename = strdup(themovie->filename);

  prvfilename[strlen(prvfilename)-3] = 'p';
  prvfilename[strlen(prvfilename)-2] = 'r';
  prvfilename[strlen(prvfilename)-1] = 'v';

  preview = (framestruct *)malloc(4 * sizeof(framestruct));

  previewfp = fopen(prvfilename, "rb");

  if(previewfp) {
    for(i = 0; i < 4; i++) {
      output = preview[i].frame = (void *)malloc(themovie->imgsize+4);
      length = themovie->imgsize; 

      /* unrle code  modified from example on www.compuphase.com */

      while (length>0) {
        count=(signed char)fgetc(previewfp);
        if (count>0) {
          //copied run
          memset(output,fgetc(previewfp),count);
        } else if (count<0) {
          //literal run
          count=(signed char)-count;
          if(!fread(output, 1, count, previewfp))
            break;
        } 
        output+=count;
        length-=count;
      } 
    }

    fclose(previewfp);
    newThread(playpreview, STACK_DFL, NULL);

  }
}

void playpreview() {
  char *MsgP;
  int RcvID, Channel, timer, i=0;

  Channel = makeChan();

  //4fps for the preview... which is only 4 frames.
  timer = setTimer(-1, 250, 0, Channel, PMSG_Alarm);

  while(1) {
    if(i > 3)
      i=0;
    RcvID = recvMsg(Channel, (void *)&MsgP);

    if(!themovie->endofpreview) {
      memcpy(bmpdata, preview[i++].frame, themovie->imgsize);
      JWReDraw(bmp);
    }

    replyMsg(RcvID,-1);
    if(!themovie->endofpreview)
      timer = setTimer(timer,250, 0, Channel, PMSG_Alarm);
  }
}
