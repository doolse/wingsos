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
} framestr;

typedef struct mheader_s {
  //Media ID Characters; Should be WMOV
  char id[4];

  //Video Details
  ulong framecount;
  uint xsize;
  uint ysize;
  int framerate;
  
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

void getframes();
void showframes();
void dowavaudio();
void donoaudio();

//Global Variables

void *bmp, *bmpdata;
FILE * fp;
long imgsize;

//FLAGS

int endofmovie;
int endofpreview;
int replayflag;

int playbackchannel;
int audiochannel;

framestr * preview;

mheader wmovheader;
framestr * frames;
long nextframe;

void getpreview(char * filename);
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

  wmovheader.framerate = atoi(JTxfGetText(frameadjust));
  
}

void replay() {
  if(replayflag) {
    setTimer(-1, 1, 0, playbackchannel, PMSG_Alarm);
    setTimer(-1, 1, 0, audiochannel,    PMSG_Alarm);
  }
}

// *** THE MAIN FUNCTION ***

void main(int argc, char * argv[]) {
  void * app, * window, * scr, * view, *button, *frameadjust;
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

  fread(&wmovheader,sizeof(mheader),1,fp);

  if(argc > 2)
    wmovheader.framerate = atoi(argv[2]);

  if(!(wmovheader.id[0] == 'W' && 
       wmovheader.id[1] == 'M' && 
       wmovheader.id[2] == 'O' &&
       wmovheader.id[3] == 'V')) {
    printf("The file %s is not a valid WiNGs movie file.\n", argv[1]);
    exit(1);
  } else
    printf("Valid movie file.\n");

  app    = JAppInit(NULL,0);
  window = JWndInit(NULL, "Video Viewer", JWndF_Resizable);

  ((JCnt *)window)->Orient = JCntF_TopBottom;

  //Calculate how much ram is needed for the image. color mem + bitmap mem
  imgsize = (wmovheader.ysize*(wmovheader.xsize/8)) + ((wmovheader.xsize/8)*(wmovheader.ysize/8));

  if(wmovheader.xsize > 296)
    xsize = 298;
  else
    xsize = wmovheader.xsize;

  if(wmovheader.ysize > 160)
    ysize = 160;
  else
    ysize = wmovheader.ysize;

  JWSetBounds(window, 0,0, xsize+8, ysize+24);
  JAppSetMain(app, window);

  frames = (framestr *)malloc((wmovheader.framecount+1) * sizeof(framestr));

  printf("\n\n%ld\n\n", wmovheader.framecount * sizeof(framestr));

  bmpdata = calloc(imgsize,1);
  bmp     = JBmpInit(NULL,(int)wmovheader.xsize,(int)wmovheader.ysize,bmpdata);
  view    = JViewWinInit(NULL, bmp);
  scr     = JScrInit(NULL, view, 0);

  button  = JButInit(NULL," Play ");
  frameadjust = JTxfInit(NULL);

  JCntAdd(window, scr);
  JCntAdd(window, button);
  JCntAdd(window, frameadjust);  

  JWinCallback(button, JBut, Clicked, replay);
  JWinCallback(frameadjust, JTxf, Entered, changeframerate);

  JWinShow(window);

  printf("wave bytes: %ld\n", wmovheader.wavbytes);
  printf("sid bytes:  %ld\n", wmovheader.sidbytes);
  printf("framecount: %ld\n", wmovheader.framecount);
  printf("xsize:      %d\n", wmovheader.xsize);
  printf("ysize:      %d\n", wmovheader.ysize);
  printf("framerate:  %d\n", wmovheader.framerate);

  retexit(1);

  if(wmovheader.framerate == 100)
    wmovheader.framerate = 90;

  if(wmovheader.wavbytes > 0) {
    getpreview(strdup(argv[1]));  
    newThread(dowavaudio, STACK_DFL, NULL); 
  } else {
    newThread(donoaudio, STACK_DFL, NULL);
  }

  JAppLoop(app);
}

void donoaudio() {
  newThread(getframes, STACK_DFL, NULL);
  if(wmovheader.framerate < 100)
    mysleep(20);
  else if(wmovheader.framerate < 125)
    mysleep(15);
  else
    mysleep(10);
  newThread(showframes, STACK_DFL, NULL);
}

void dowavaudio() {
  char *MsgP;
  int RcvID;
  int digiChan;
  Riff RiffHdr;
  RChunk Chunk;
  RifForm Format;
  char *wavbuf, *wavbufptr;
  int done = 0;
  int amount;
  long inread, totalsize;
  int hdrsize = 0;

  audiochannel = makeChan();

  digiChan = open("/dev/mixer",O_READ|O_WRITE);
  if (digiChan == -1) {
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
      fread(&Format,1,Chunk.ChSize,fp);
      hdrsize += Chunk.ChSize;
    } else if (Chunk.Ident[0]=='d')
      done=1;
    else 
      fseek(fp,Chunk.ChSize,SEEK_CUR);
  } 

  sendCon(digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) Format.SampRate, 1, 2);
  
  wavbufptr = wavbuf = malloc(wmovheader.wavbytes-hdrsize);
  totalsize = inread = fread(wavbuf, 1, wmovheader.wavbytes-hdrsize, fp);

  newThread(getframes, STACK_DFL, NULL);
  if(wmovheader.framerate < 100)
    mysleep(20);
  else if(wmovheader.framerate < 125)
    mysleep(15);
  else
    mysleep(10);
  endofpreview = 1;
  newThread(showframes, STACK_DFL, NULL);

  while(1) {

    while (inread) {

      if (inread > 32767)
        amount = 32767;
      else 
        amount = inread;

      write(digiChan, wavbuf, amount);
      wavbuf += amount;
      inread -= amount;
    }

    //block until the entire wav is finished playing.
    //write(digiChan,wavbuf,1);

    RcvID = recvMsg(audiochannel, (void *)&MsgP);
    //replyMsg(RcvID,-1);

    //Reset buf pointer and inread size.
    wavbuf = wavbufptr;
    inread = totalsize;
  }
}

void showframes() {
  char *MsgP;
  int RcvID, timer;
  long currentframe = 0;

  playbackchannel = makeChan();

  while(1) {

    replayflag = 0;

    timer = setTimer(-1, wmovheader.framerate, 0, playbackchannel, PMSG_Alarm);

    while(1) {
      RcvID = recvMsg(playbackchannel, (void *)&MsgP);

      if(currentframe < nextframe) {
        memcpy(bmpdata, frames[currentframe++].frame, imgsize);
        JWReDraw(bmp);
      } else {
        if(endofmovie)
          break;
        printf("lost frame %d\n", currentframe);
      }

      replyMsg(RcvID,-1);
      timer = setTimer(timer,wmovheader.framerate, 0, playbackchannel, PMSG_Alarm);
    }

    replayflag = 1;

    RcvID = recvMsg(playbackchannel, (void *)&MsgP);
    //replyMsg(RcvID,-1);

    currentframe = 0;
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
    length = imgsize;
    output = frames[nextframe].frame = (void *)malloc(imgsize+4);
 
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
      printf("Stopped reading on frame %d, %d bytes read. %d\n", nextframe, abs(length-imgsize), length);
      fclose(fp);
      endofmovie = 1;
      Channel = makeChan();
      RcvID = recvMsg(Channel, (void *)&MsgP);
    } else if(nextframe == (wmovheader.framecount-1)) {
      nextframe = wmovheader.framecount;
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

void getpreview(char * filename) {
  int i, count, length;
  FILE * previewfp;
  char * output;

  //change file extension from .mov to .prv

  filename[strlen(filename)-3] = 'p';
  filename[strlen(filename)-2] = 'r';
  filename[strlen(filename)-1] = 'v';

  preview = (framestr *)malloc(4 * sizeof(framestr));

  previewfp = fopen(filename, "rb");

  if(previewfp) {
    for(i = 0; i < 4; i++) {
      output = preview[i].frame = (void *)malloc(imgsize+4);
      length = imgsize; 

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

    if(!endofpreview) {
      memcpy(bmpdata, preview[i++].frame, imgsize);
      JWReDraw(bmp);
    }

    replyMsg(RcvID,-1);
    if(!endofpreview)
      timer = setTimer(timer,250, 0, Channel, PMSG_Alarm);
  }
}
