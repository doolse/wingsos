#include "vidheader.h"
#include "iconlibrary.h"

movie * themovie;

void mysleep(int seconds) {
  char *MsgP;
  int Channel, RcvID;

  Channel = makeChan();
  setTimer(-1, seconds*1000, 0, Channel, PMSG_Alarm);
  RcvID = recvMsg(Channel, (void *)&MsgP);
  replyMsg(RcvID,-1);
}

void mysleepm(int microseconds) {
  char *MsgP;
  int Channel, RcvID;

  Channel = makeChan();
  setTimer(-1, microseconds, 0, Channel, PMSG_Alarm);
  RcvID = recvMsg(Channel, (void *)&MsgP);
  replyMsg(RcvID,-1);
}

void movetostart(void * button) {
  movie * movieptr;
  movieptr = JWGetData(button);

  movieptr->playadjusted = 1;
  if(!movieptr->pause)
    mysleepm(1000);

  movieptr->cframe       = movieptr->firstframe;
  if(movieptr->wmovheader.wavbytes)
    movieptr->cwavposition = movieptr->startofwavbuf;
  setTimer(-1,1,0, movieptr->playadjustchannel, PMSG_Alarm);
}

void movetoend(void * button) {
  movie * movieptr;
  movieptr = JWGetData(button);

  movieptr->playadjusted = 1;
  if(!movieptr->pause)
    mysleepm(1000);

  movieptr->cframe       = movieptr->lastframe;
  if(movieptr->wmovheader.wavbytes)
    movieptr->cwavposition = movieptr->endofwavbuf;
  setTimer(-1,1,0, movieptr->playadjustchannel, PMSG_Alarm);
}

void changeplayrate(movie * movieptr, int speed) {
  
  if(speed == HIGHSPEED)
    movieptr->playratepercent = 200;
  else
    movieptr->playratepercent = 100;

  movieptr->cframerate = movieptr->wmovheader.framerate * movieptr->playratepercent / 100;

  if(movieptr->wmovheader.wavbytes) {
    movieptr->csamplerate = movieptr->Format.SampRate * movieptr->playratepercent / 100;  
    sendCon(movieptr->digiChan, IO_CONTROL, 0xc0, 8, (unsigned int)movieptr->csamplerate, 1, 2);

    if(!movieptr->pause) {
      getMutex(&(movieptr->pausemutex));
      mysleepm(100);
      relMutex(&(movieptr->pausemutex));
    }
  }
}

void playadjust(void * button) {
  movie * movieptr;
  movieptr = JWGetData(button);

  movieptr->playadjusted = 1;

  if(movieptr->pause) {
    movieptr->pause = 0;
    relMutex(&(movieptr->pausemutex));
  } else {
    mysleepm(1000);
  }

  if(button == movieptr->but_play) {
    changeplayrate(movieptr, STANDARD);
    movieptr->reverse = 0;
    JTxfSetText(movieptr->statustext, "Playing...");
  } else if(button == movieptr->but_rplay) {
    changeplayrate(movieptr, STANDARD);
    movieptr->reverse = 1;
    JTxfSetText(movieptr->statustext, "Playing in reverse...");
  } else if(button == movieptr->but_fast) {
    changeplayrate(movieptr, HIGHSPEED);
    movieptr->reverse = 0;
    JTxfSetText(movieptr->statustext, "Playing in highspeed...");
  } else if(button == movieptr->but_rfast) {
    changeplayrate(movieptr, HIGHSPEED);
    movieptr->reverse = 1;
    JTxfSetText(movieptr->statustext, "Highspeed in reverse...");
  }
  
  setTimer(-1,1,0, movieptr->playadjustchannel, PMSG_Alarm);
}

void pauseplayback(void * button) {
  movie * movieptr;
  movieptr = JWGetData(button);

  if(movieptr->pause) {
    movieptr->pause = 0;
    relMutex(&(movieptr->pausemutex));
    JTxfSetText(movieptr->statustext, movieptr->laststatustext);
    movieptr->laststatustext = NULL;
  } else {
    movieptr->pause = 1;
    getMutex(&(movieptr->pausemutex));
    movieptr->laststatustext = strdup(JTxfGetText(movieptr->statustext));
    JTxfSetText(movieptr->statustext, "Paused");
  }
}

void playpreview(movie * movieptr) {
  char *MsgP;
  int RcvID, Channel, timer, i=0;
  framestruct * frameptr;

  Channel = makeChan();
  
  //movieptr->previewfps stored as a micro second delay
  timer = setTimer(-1, movieptr->wmovheader.previewfps, 0, Channel, PMSG_Alarm);

  frameptr = movieptr->prvframes;

  while(1) {
    if(i > (movieptr->wmovheader.numofpreviewframes-1)) {
      i=0;
      frameptr = movieptr->prvframes;
    }

    RcvID = recvMsg(Channel, (void *)&MsgP);

    if(!movieptr->endofpreview) {
      memcpy(movieptr->bmpdata, frameptr->frame, themovie->imgsize);
      JWReDraw(movieptr->bmp);
    }

    //go to next frame;
    i++;
    frameptr++;

    replyMsg(RcvID,-1);
    if(!movieptr->endofpreview)
      timer = setTimer(timer,movieptr->wmovheader.previewfps, 0, Channel, PMSG_Alarm);
  }
}

int getpreview(movie * movieptr) {
  int i, count, length;
  char * output;
  framestruct * frameptr;

  if(!movieptr->wmovheader.numofpreviewframes)
    return(-1);

  frameptr = movieptr->prvframes = malloc(movieptr->wmovheader.numofpreviewframes * sizeof(framestruct));

  for(i = 0; i < movieptr->wmovheader.numofpreviewframes; i++) {
    output = frameptr->frame = malloc(themovie->imgsize+4);
    length = themovie->imgsize; 

    // unrle code  modified from example on www.compuphase.com

    while (length>0) {
      count=(signed char)fgetc(movieptr->fp);
      if (count>0) {
        //copied run
        memset(output,fgetc(movieptr->fp),count);
      } else if (count<0) {
        //literal run
        count=(signed char)-count;
        if(!fread(output, 1, count, movieptr->fp))
          break;
      } 
      output+=count;
      length-=count;
    } 
    frameptr++;
  }
  newThread(playpreview, STACK_DFL, movieptr);
  return(0);
}

void showframes(movie * movieptr) {
  char *MsgP;
  int RcvID;

  movieptr->frameflipchannel = makeChan();

  while(1) {
    RcvID = recvMsg(movieptr->frameflipchannel, (void *)&MsgP);
    memcpy(movieptr->bmpdata, movieptr->cframe, movieptr->imgsize);
    JWReDraw(movieptr->bmp);
    replyMsg(RcvID,-1);
  }
}

void getframes(movie * movieptr) {
  signed char count;
  long length;
  char *output;
  framestruct * loadframe;
  int i; 

  movieptr->firstframe = malloc((movieptr->wmovheader.framecount+1) * sizeof(framestruct));
  movieptr->lastframe  = movieptr->firstframe + movieptr->wmovheader.framecount;

  loadframe = movieptr->cframe = movieptr->firstframe;

  for(i=0; i<movieptr->wmovheader.framecount; i++,loadframe++) {

    length = movieptr->imgsize;
    output = loadframe->frame = malloc(length+4);
 
    // unrle code  modified from example on www.compuphase.com

    while (length>0) {
      count=(signed char)fgetc(movieptr->fp);
      if (count>0) {
        //copied run
        memset(output,fgetc(movieptr->fp),count);
      } else if (count<0) {
        //literal run
        count=(signed char)-count;
        if(!fread(output, 1, count, movieptr->fp))
          break;
      } 
      output+=count;
      length-=count;
    } 
  }
}

void prepwavaudio(movie * movieptr) {
  Riff RiffHdr;
  RChunk Chunk;
  int done = 0, hdrsize = 0;

  movieptr->digiChan = open("/dev/mixer",O_READ|O_WRITE);
  if (movieptr->digiChan == -1) {
    printf("Digi device not loaded\n");
    exit(1);
  }

  fread(&RiffHdr,1,sizeof(Riff),movieptr->fp);
  if (RiffHdr.RiffIdent[0]!='R' || RiffHdr.RiffIdent[1]!='I') {
    printf("Bad Wav Audio track!\n");
    exit(1);
  }

  hdrsize += sizeof(Riff);

  while (!done) {
    fread(&Chunk,1,sizeof(RChunk),movieptr->fp);

    hdrsize += sizeof(RChunk);

    if (Chunk.Ident[0]=='f' && Chunk.Ident[1]=='m') {
      fread(&(movieptr->Format),1,Chunk.ChSize,movieptr->fp);
      hdrsize += Chunk.ChSize;
    } 
    else if (Chunk.Ident[0]=='d')
      done=1;
    else 
      fseek(movieptr->fp,Chunk.ChSize,SEEK_CUR);
  } 
  movieptr->totalwavsize  = movieptr->wmovheader.wavbytes-hdrsize;
  movieptr->startofwavbuf = movieptr->cwavposition = malloc(movieptr->totalwavsize);
  movieptr->endofwavbuf   = movieptr->startofwavbuf + movieptr->totalwavsize;
}

void dowavaudio(movie * movieptr) {
  char *MsgP;
  ulong syncbytes;
  int amount;

  sendCon(movieptr->digiChan, IO_CONTROL, 0xc0, 8, (unsigned int)movieptr->csamplerate, 1, 2);
  
  //Read wav data into memory
  fread(movieptr->startofwavbuf, 1, movieptr->totalwavsize, movieptr->fp);

  newThread(getframes, STACK_DFL, movieptr);
  mysleep(20);
  movieptr->endofpreview = 1;
  JTxfSetText(movieptr->statustext, "Playing...");
  newThread(showframes, STACK_DFL, movieptr);

  syncbytes = movieptr->wmovheader.wavbytes/movieptr->wmovheader.framecount;

  movieptr->playadjustchannel = makeChan();

  while(1) {
    movieptr->playadjusted = 0;  
    printf("Play adjust just set to false.\n");

    while (movieptr->cwavposition <= movieptr->endofwavbuf && 
           movieptr->cwavposition >= movieptr->startofwavbuf) {

      getMutex(&(movieptr->pausemutex));
    
      if(movieptr->playadjusted)
        break;
      if(movieptr->cwavposition == movieptr->endofwavbuf && !movieptr->reverse)
        break;
      if(movieptr->cwavposition == movieptr->startofwavbuf && movieptr->reverse)
        break;

      if(movieptr->reverse) {
        if (movieptr->cwavposition - movieptr->startofwavbuf > syncbytes)
          amount = syncbytes;
        else 
          amount = movieptr->cwavposition - movieptr->startofwavbuf;
        setTimer(-1,1,0,movieptr->frameflipchannel, PMSG_Alarm);
        write(movieptr->digiChan, movieptr->cwavposition, amount);
        if(movieptr->cframe > movieptr->firstframe)
          movieptr->cframe--;
        movieptr->cwavposition -= amount;
      } else {
        if (movieptr->endofwavbuf - movieptr->cwavposition > syncbytes)
          amount = syncbytes;
        else 
          amount = movieptr->endofwavbuf - movieptr->cwavposition;
        setTimer(-1,1,0,movieptr->frameflipchannel, PMSG_Alarm);
        write(movieptr->digiChan, movieptr->cwavposition, amount);
        if(movieptr->cframe < movieptr->lastframe)
          movieptr->cframe++;
        movieptr->cwavposition += amount;
      }

      relMutex(&(movieptr->pausemutex));
    }

    //block until the entire wav is finished playing.
    write(movieptr->digiChan,movieptr->cwavposition,0);

    printf("Left the loop for a play adjust.\n");

    replyMsg(recvMsg(movieptr->playadjustchannel,(void *)&MsgP),1);
  }
}

void donoaudio(movie * movieptr) {
  char *MsgP;
  int Channel, RcvID, timer;

  newThread(getframes, STACK_DFL, movieptr);
  mysleep(20);
  movieptr->endofpreview = 1;
  JTxfSetText(movieptr->statustext, "Playing...");
  newThread(showframes, STACK_DFL, movieptr);

  movieptr->playadjustchannel = makeChan();
  Channel = makeChan();

  while(1) {
    movieptr->playadjusted = 0;  
    printf("playadjusted just set to zero.\n");

    timer = setTimer(-1,movieptr->cframerate,0,Channel, PMSG_Alarm);

    while (movieptr->cframe >= movieptr->firstframe && 
           movieptr->cframe <= movieptr->lastframe) {

      RcvID = recvMsg(Channel,(void *)&MsgP);
      getMutex(&(movieptr->pausemutex));
    
      if(movieptr->playadjusted)
        break;

      setTimer(-1,1,0,movieptr->frameflipchannel, PMSG_Alarm);

      if(movieptr->reverse) {
        if(movieptr->cframe > movieptr->firstframe)
          movieptr->cframe--;
        else
          break;
      } else {
        if(movieptr->cframe < movieptr->lastframe)
          movieptr->cframe++;
        else
          break;
      }

      relMutex(&(movieptr->pausemutex));
      replyMsg(RcvID,1);

      timer = setTimer(timer,movieptr->cframerate,0,Channel, PMSG_Alarm);
    }

    JTxfSetText(movieptr->statustext, "Stopped.");
    printf("Just left the loop.\n");

    replyMsg(recvMsg(movieptr->playadjustchannel,(void *)&MsgP),1);
  }
}


void main(int argc, char * argv[]) {
  void *app, *window, *scr, *view, *controlcontainer;
  uint xsize, ysize;
  uint minxsize, minysize;
  void * dialog, * msgtext;

  if(argc < 2) {
    fprintf(stderr, "USAGE: vidviewer moviefile\n");
    exit(1);
  }

  // INITIALIZE THE MOVIE 
    themovie = (movie *)malloc(sizeof(movie));

    themovie->filename     = strdup(argv[1]);
    themovie->reverse      = 0;
    themovie->pause        = 0;
    themovie->pausemutex.a = -1;
    themovie->pausemutex.b = -1;

    themovie->endofpreview    = 0;
    themovie->playratepercent = 100;

  // END OF MOVIE INIT

  themovie->fp = fopen(themovie->filename, "rb");
  if(!themovie->fp) {
    printf("the file '%s' could not be found.\n", themovie->filename);
    exit(1);
  }

  fread(&(themovie->wmovheader),sizeof(mheader),1,themovie->fp);

  //CHECK TO SEE IF THIS IS A VALID MOVIE
  if(strncmp(themovie->wmovheader.id, "WMOV", 4)) {
    printf("The file %s is not a valid WiNGs movie file.\n", argv[1]);
    exit(1);
  } 

  app    = JAppInit(NULL,0);
  window = JWndInit(NULL, strdup(themovie->filename), JWndF_Resizable);

  ((JCnt *)window)->Orient = JCntF_TopBottom;

  //CALCULATE MEMORY SIZE OF AN UNCOMPRESSED FRAME
  themovie->imgsize = (themovie->wmovheader.ysize*(themovie->wmovheader.xsize/8)) + ((themovie->wmovheader.xsize/8)*(themovie->wmovheader.ysize/8));

  //CALCULATE STARTING SIZE OF GUI WINDOW
  if(themovie->wmovheader.xsize > 296)
    xsize = 296;
  else
    xsize = themovie->wmovheader.xsize;

  if(xsize < 64)
    xsize = 64;

  if(themovie->wmovheader.ysize > 160)
    ysize = 160;
  else
    ysize = themovie->wmovheader.ysize;

  if(xsize > 64)
    minxsize = 64;
  else
    minxsize = xsize;

  minysize = 24;

  JWSetBounds(window, 0,0, xsize, ysize+16);
  JWSetMin(window, minxsize, minysize);
  JWSetMax(window, xsize+8, ysize+24);
  JAppSetMain(app, window);

  themovie->bmpdata = calloc(themovie->imgsize,1);
  themovie->bmp     = JBmpInit(NULL,(int)themovie->wmovheader.xsize,(int)themovie->wmovheader.ysize,themovie->bmpdata);

  view = JViewWinInit(NULL, themovie->bmp);
  scr  = JScrInit(NULL, view, JScrF_VNotEnd|JScrF_HNotEnd);

  JWSetMin(scr, 8,8);

  controlcontainer = JCntInit(NULL);

  JCntAdd(window, scr);
  JCntAdd(window, controlcontainer);

  ((JCnt *)controlcontainer)->Orient = JCntF_LeftRight;

  themovie->but_start = JIbtInit(NULL,8,8, ico_flushleftarrow8,  ico_flushleftarrow8_inv);
  themovie->but_rfast = JIbtInit(NULL,8,8, ico_doubleleftarrow8, ico_doubleleftarrow8_inv);
  themovie->but_rplay = JIbtInit(NULL,8,8, ico_playleftarrow8,   ico_playleftarrow8_inv);
  themovie->but_pause = JIbtInit(NULL,8,8, ico_pausebars8,       ico_pausebars8_inv);
  themovie->but_play  = JIbtInit(NULL,8,8, ico_playrightarrow8,  ico_playrightarrow8_inv);
  themovie->but_fast  = JIbtInit(NULL,8,8, ico_doublerightarrow8,ico_doublerightarrow8_inv);
  themovie->but_end   = JIbtInit(NULL,8,8, ico_flushrightarrow8, ico_flushrightarrow8_inv);

  JWSetData(themovie->but_start, themovie);
  JWSetData(themovie->but_rfast, themovie);
  JWSetData(themovie->but_rplay, themovie);
  JWSetData(themovie->but_pause, themovie);
  JWSetData(themovie->but_play,  themovie);
  JWSetData(themovie->but_fast,  themovie);
  JWSetData(themovie->but_end,   themovie);

  JCntAdd(controlcontainer, themovie->but_start);
  JCntAdd(controlcontainer, themovie->but_rfast);
  JCntAdd(controlcontainer, themovie->but_rplay);
  JCntAdd(controlcontainer, themovie->but_pause);
  JCntAdd(controlcontainer, themovie->but_play);
  JCntAdd(controlcontainer, themovie->but_fast);
  JCntAdd(controlcontainer, themovie->but_end);

  JWinCallback(themovie->but_start,JBut,Clicked,movetostart);
  JWinCallback(themovie->but_end,  JBut,Clicked,movetoend);

  JWinCallback(themovie->but_pause,JBut,Clicked,pauseplayback);

  JWinCallback(themovie->but_rfast,JBut,Clicked,playadjust);
  JWinCallback(themovie->but_fast, JBut,Clicked,playadjust);
  JWinCallback(themovie->but_rplay,JBut,Clicked,playadjust);
  JWinCallback(themovie->but_play, JBut,Clicked,playadjust);

  themovie->statustext = JTxfInit(NULL);
  JTxfSetText(themovie->statustext, "Buffering...");
  JCntAdd(controlcontainer, themovie->statustext);

  JWndSetProp(window);
  JWinShow(window);

  printf("wave bytes: %ld\n", themovie->wmovheader.wavbytes);
  printf("sid bytes:  %ld\n", themovie->wmovheader.sidbytes);
  printf("framecount: %ld\n", themovie->wmovheader.framecount);
  printf("xsize:      %d\n",  themovie->wmovheader.xsize);
  printf("ysize:      %d\n",  themovie->wmovheader.ysize);
  printf("framerate:  %d\n",  themovie->wmovheader.framerate);

  retexit(1);

  getpreview(themovie);  

  if(themovie->wmovheader.wavbytes > 0) {
    prepwavaudio(themovie);
    changeplayrate(themovie, STANDARD);
    newThread(dowavaudio, STACK_DFL, themovie); 
  } else {
    changeplayrate(themovie, STANDARD);
    newThread(donoaudio, STACK_DFL, themovie);
  }

  JAppLoop(app);
}

