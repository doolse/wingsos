#include "vidheader.h"

//#include "iconlibrary.h"
//Media control icons 8x8  |<  <<  <  ||  >  >>  >| 

unsigned char ico_flushleftarrow8[] = {
  0,70,94,126,126,94,70,0,
  0xe0
};
unsigned char ico_doubleleftarrow8[] = {
  0,18,54,126,126,54,18,0,
  0xe0
};
unsigned char ico_playleftarrow8[] = {
  0,6,30,126,126,30,6,0,
  0xe0
};
unsigned char ico_pausebars8[] = {
  0,102,102,102,102,102,102,0,
  0xe0
};
unsigned char ico_playrightarrow8[] = {
  0,96,120,126,126,120,96,0,
  0xe0
};
unsigned char ico_doublerightarrow8[] = {
  0,72,108,126,126,108,72,0,
  0xe0
};
unsigned char ico_flushrightarrow8[] = {
  0,98,122,126,126,122,98,0,
  0xe0
};

//Inverse Media control icons 8x8 

unsigned char ico_flushleftarrow8_inv[] = {
  0,70,94,126,126,94,70,0,
  0x6f
};
unsigned char ico_doubleleftarrow8_inv[] = {
  0,18,54,126,126,54,18,0,
  0x6f
};
unsigned char ico_playleftarrow8_inv[] = {
  0,6,30,126,126,30,6,0,
  0x6f
};
unsigned char ico_pausebars8_inv[] = {
  0,102,102,102,102,102,102,0,
  0x6f
};
unsigned char ico_playrightarrow8_inv[] = {
  0,96,120,126,126,120,96,0,
  0x6f
};
unsigned char ico_doublerightarrow8_inv[] = {
  0,72,108,126,126,108,72,0,
  0x6f
};
unsigned char ico_flushrightarrow8_inv[] = {
  0,98,122,126,126,122,98,0,
  0x6f
};

//Media LOOP icon   ()

unsigned char ico_looparrows[] = {
  0,126,66,224,66,7,66,126,
  0xe0
};

unsigned char ico_looparrows_inv[] = {
  0,126,66,224,66,7,66,126,
  0x6f
};

unsigned char app_icon[] = {
0,3,15,30,56,48,113,97,
0,192,240,120,60,124,238,70,
97,113,48,56,30,15,3,0,
70,142,12,28,60,254,198,0,
0x01,0x01,0x01,0x01
};

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
  if(!movieptr->pause) {
    //sleep for the duration of one frame.. to guarantee it 
    //loops at least once, and breaks at the playadjust flag check.
    mysleepm(movieptr->cframerate);
  } else {
    movieptr->pause = 0;
    relMutex(&(movieptr->pausemutex));
  }

  movieptr->cframe = movieptr->firstframe;
  if(movieptr->wmovheader.wavbytes)
    movieptr->cwavposition = movieptr->startofwavbuf;

  sendChan(movieptr->playadjustchannel,PMSG_Alarm);
}

void movetoend(void * button) {
  movie * movieptr;
  movieptr = JWGetData(button);

  movieptr->playadjusted = 1;
  if(!movieptr->pause)
    mysleepm(movieptr->cframerate);
  else {
    movieptr->pause = 0;
    relMutex(&(movieptr->pausemutex));
  }

  movieptr->cframe = movieptr->lastframe;
  if(movieptr->wmovheader.wavbytes)
    movieptr->cwavposition = movieptr->endofwavbuf;

  sendChan(movieptr->playadjustchannel, PMSG_Alarm);
}

void changeplayrate(movie * movieptr, int speed) {
  
  if(speed == HIGHSPEED)
    movieptr->playratepercent = 150;
  else
    movieptr->playratepercent = 100;

  movieptr->cframerate = movieptr->wmovheader.framerate * 100 / movieptr->playratepercent;

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

void toggleloopflag(void * button) {
  movie * movieptr = JWGetData(button);

  if(movieptr->loopflag)
    movieptr->loopflag = 0;
  else
    movieptr->loopflag = 1;

  //printf("loop flag == %d\n",movieptr->loopflag);
}

void playadjust(void * button) {
  movie * movieptr = JWGetData(button);

  movieptr->playadjusted = 1;

  if(movieptr->pause) {
    movieptr->pause = 0;
    relMutex(&(movieptr->pausemutex));
  }
  mysleepm(movieptr->cframerate);

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
  
  sendChan(movieptr->playadjustchannel,PMSG_Alarm);
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
  
  timer = setTimer(-1, movieptr->wmovheader.previewfps, 0, Channel, PMSG_Alarm);

  frameptr = movieptr->prvframes;

  while(1) {
    if(i >= movieptr->wmovheader.numofpreviewframes) {
      i=0;
      frameptr = movieptr->prvframes;
    }

    RcvID = recvMsg(Channel, (void *)&MsgP);

    if(!movieptr->endofpreview) {
      memcpy(movieptr->bmpdata, frameptr->frame, movieptr->imgsize);
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

  movieptr->prvframes = malloc((movieptr->wmovheader.numofpreviewframes+1) * sizeof(framestruct));
  frameptr = movieptr->prvframes;

  for(i = 0; i < movieptr->wmovheader.numofpreviewframes; i++) {
    length = movieptr->imgsize; 
    output = frameptr->frame = malloc(length+2);

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

  while(1) {
    RcvID = recvMsg(movieptr->frameflipchannel, (void *)&MsgP);
    memcpy(movieptr->bmpdata, movieptr->cframe->frame, movieptr->imgsize);
    JWReDraw(movieptr->bmp);
    replyMsg(RcvID,-1);
  }
}

void getframes(movie * movieptr) {
  signed char count;
  int length;
  char *output;
  framestruct * loadframe;
  int i; 

  movieptr->firstframe = malloc((movieptr->wmovheader.framecount+1) * sizeof(framestruct));
  movieptr->lastframe  = movieptr->firstframe + movieptr->wmovheader.framecount - 1;

  loadframe = movieptr->cframe = movieptr->firstframe;

  for(i=0; i<movieptr->wmovheader.framecount; i++) {

    length = movieptr->imgsize;
    output = loadframe->frame = malloc(length+2);
 
    // unrle code  modified from example on www.compuphase.com
    while (length>0) {
      count=(signed char)fgetc(movieptr->fp);
      if (count>0) {
        //copied run
        if(count > length) {
          printf("frame #%d corrupt!\n",i);
          exit(1);
        }
        memset(output,fgetc(movieptr->fp),count);
      } else if (count<0) {
        //literal run
        count=(signed char)-count;
        if(count > length) {
          printf("frame #%d corrupt!\n",i);
          exit(1);
        }
        if(!fread(output, 1, count, movieptr->fp))
          break;
      }
      output+=count;
      length-=count;
    } 

    movieptr->lastframe = loadframe++;
  }
}

void prepwavaudio(movie * movieptr) {
  Riff RiffHdr;
  RChunk Chunk;
  int done = 0, hdrsize = 0;

  movieptr->digiChan = open("/dev/mixer",O_READ|O_WRITE);
  if (movieptr->digiChan == -1) {
    printf("Digi device not loaded!\nLoading digimax version now.\n");
    system("digi.drv -u");

    movieptr->digiChan = open("/dev/mixer",O_READ|O_WRITE);
    if (movieptr->digiChan == -1) {
      printf("Error getting digi device channel.\n");
      exit(1);
    }
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
    } else if (Chunk.Ident[0]=='d')
      done=1;
    else 
      fseek(movieptr->fp,Chunk.ChSize,SEEK_CUR);
  } 
  movieptr->totalwavsize  = movieptr->wmovheader.wavbytes-hdrsize;
  movieptr->startofwavbuf = malloc(movieptr->totalwavsize);

  movieptr->cwavposition  = movieptr->startofwavbuf;
  movieptr->endofwavbuf   = movieptr->startofwavbuf + movieptr->totalwavsize - 1;
}

void dowavaudio(movie * movieptr) {
  char *MsgP;
  ulong syncbytes;
  int amount;

  sendCon(movieptr->digiChan, IO_CONTROL, 0xc0, 8, (unsigned int)movieptr->csamplerate, 1, 2);
  
  //Read wav data into memory
  fread(movieptr->startofwavbuf, 1, movieptr->totalwavsize, movieptr->fp);

  newThread(getframes, STACK_DFL, movieptr);

  //buffer time
  mysleep(20);

  movieptr->endofpreview = 1;
  mysleepm(movieptr->cframerate);

  JTxfSetText(movieptr->statustext, "Playing...");
  newThread(showframes, STACK_DFL, movieptr);

  syncbytes = ((ulong)movieptr->totalwavsize)/movieptr->wmovheader.framecount;

  movieptr->playadjusted = 0;  

  while(1) {
    while (movieptr->cwavposition <= movieptr->endofwavbuf && 
           movieptr->cwavposition >= movieptr->startofwavbuf) {

      getMutex(&(movieptr->pausemutex));
    
      if(movieptr->playadjusted) {
        relMutex(&(movieptr->pausemutex));
        break;
      }
      if(movieptr->cwavposition == movieptr->endofwavbuf && !movieptr->reverse) {
        relMutex(&(movieptr->pausemutex));
        break;
      }
      if(movieptr->cwavposition == movieptr->startofwavbuf && movieptr->reverse) {
        relMutex(&(movieptr->pausemutex));
        break;
      }

      if(movieptr->reverse) {
        if (movieptr->cwavposition - movieptr->startofwavbuf > syncbytes)
          amount = syncbytes;
        else 
          amount = movieptr->cwavposition - movieptr->startofwavbuf;

        sendChan(movieptr->frameflipchannel,PMSG_Alarm);
        write(movieptr->digiChan, movieptr->cwavposition, amount);

        if(movieptr->cframe > movieptr->firstframe)
          movieptr->cframe--;
        movieptr->cwavposition -= amount;

      } else {
        if (movieptr->endofwavbuf - movieptr->cwavposition > syncbytes)
          amount = syncbytes;
        else 
          amount = movieptr->endofwavbuf - movieptr->cwavposition;

        sendChan(movieptr->frameflipchannel,PMSG_Alarm);
        write(movieptr->digiChan, movieptr->cwavposition, amount);

        if(movieptr->cframe < movieptr->lastframe) {
          movieptr->cframe++;
          movieptr->cwavposition += amount;
        } else if(amount == syncbytes) {
          relMutex(&(movieptr->pausemutex));
          break;
        }
      }

      relMutex(&(movieptr->pausemutex));
    }

    if(movieptr->loopflag  && !movieptr->playadjusted) {
      if(movieptr->reverse) {
        movieptr->cwavposition = movieptr->endofwavbuf;
        movieptr->cframe = movieptr->lastframe;
      } else {
        movieptr->cwavposition = movieptr->startofwavbuf;
        movieptr->cframe = movieptr->firstframe;
      }
      //printf("I tried to loop.\n");
      //continue the loop
      setTimer(-1, 1, 0, movieptr->playadjustchannel, PMSG_Alarm);
    } /* else {
      printf("playadjusted == %d\n", movieptr->playadjusted);
      printf("from start of wav = %d\n", movieptr->cwavposition-movieptr->startofwavbuf);
      printf("from end of wav = %d\n", movieptr->endofwavbuf-movieptr->cwavposition);
      printf("loop flag === %d\n",movieptr->loopflag);
    } */

    //block until the end of the wav sample is finished playing.
    write(movieptr->digiChan,movieptr->cwavposition,0);

    replyMsg(recvMsg(movieptr->playadjustchannel,(void *)&MsgP),1);
    movieptr->playadjusted = 0;  
  }
}

void donoaudio(movie * movieptr) {
  char *MsgP;
  int RcvID, timer, Channel = makeChan();

  newThread(getframes, STACK_DFL, movieptr);

  //buffer time
  mysleep(40);

  movieptr->endofpreview = 1;
  mysleepm(movieptr->cframerate);

  JTxfSetText(movieptr->statustext, "Playing...");
  newThread(showframes, STACK_DFL, movieptr);

  movieptr->playadjusted = 0;  
  while(1) {
    timer = setTimer(-1,movieptr->cframerate,0,Channel, PMSG_Alarm);

    while (movieptr->cframe >= movieptr->firstframe && 
           movieptr->cframe <= movieptr->lastframe) {

      RcvID = recvMsg(Channel,(void *)&MsgP);
      getMutex(&(movieptr->pausemutex));
    
      if(movieptr->playadjusted) {
        relMutex(&(movieptr->pausemutex));
        break;
      }

      sendChan(movieptr->frameflipchannel,PMSG_Alarm);

      if(movieptr->reverse)
        movieptr->cframe--;
      else
        movieptr->cframe++;

      relMutex(&(movieptr->pausemutex));
      replyMsg(RcvID,1);

      timer = setTimer(timer,movieptr->cframerate,0,Channel, PMSG_Alarm);
    }

    JTxfSetText(movieptr->statustext, "Stopped.");

    if((movieptr->cframe < movieptr->firstframe || 
        movieptr->cframe > movieptr->lastframe) && 
        movieptr->loopflag && 
        !movieptr->playadjusted) {
      if(movieptr->reverse)
        movieptr->cframe = movieptr->lastframe;
      else
        movieptr->cframe = movieptr->firstframe;

      setTimer(-1,1,0,movieptr->playadjustchannel, PMSG_Alarm);
    }

    replyMsg(recvMsg(movieptr->playadjustchannel,(void *)&MsgP),1);
    movieptr->playadjusted = 0;  
  }
}


void main(int argc, char * argv[]) {
  void *app, *window, *scr, *view, *controlcontainer;
  void * dialog, * msgtext;
  movie * themovie;
  SizeHints sizes;
  JMeta * metadata;
	
  if(argc != 2) {
    exit(1);
  }

  // INITIALIZE THE MOVIE 
    themovie = malloc(sizeof(movie));

    themovie->filename     = strdup(argv[1]);
    themovie->reverse      = 0;
    themovie->pause        = 0;
    themovie->loopflag     = 0;
    themovie->pausemutex.a = -1;
    themovie->pausemutex.b = -1;

    themovie->endofpreview    = 0;
    themovie->playratepercent = 100;

    themovie->playadjustchannel = makeChan();
    themovie->frameflipchannel  = makeChan();

  // END OF MOVIE INIT

  themovie->fp = fopen(themovie->filename, "rb");
  if(!themovie->fp) {
    printf("the file '%s' could not be found.\n", themovie->filename);
    exit(1);
  }

  fread(&(themovie->wmovheader),sizeof(mheader),1,themovie->fp);

  //CHECK TO SEE IF THIS IS A VALID MOVIE
  if(strncmp(themovie->wmovheader.id, "WMOV", 4)) {
    printf("The file %s is not a valid WiNGs movie file.\n",themovie->filename);
    exit(1);
  } 

  app    = JAppInit(NULL,0);

  //Fuck it, it's no longer resizable. Who really gives a damn?

  metadata = malloc(sizeof(JMeta));
  metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
  metadata->title = "Movie Player";
  metadata->icon = app_icon;
  metadata->showicon = 1;
  metadata->parentreg = -1;

  window = JWndInit(NULL, themovie->filename,JWndF_Resizable,metadata);
  JAppSetMain(app, window);


  //((JCnt *)window)->Orient = JCntF_TopBottom;

  //CALCULATE MEMORY SIZE OF AN UNCOMPRESSED FRAME
  themovie->imgsize = (themovie->wmovheader.ysize*(themovie->wmovheader.xsize/8)) + ((themovie->wmovheader.xsize/8)*(themovie->wmovheader.ysize/8));

  themovie->bmpdata = calloc(themovie->imgsize+(themovie->imgsize/2),1);
  themovie->bmp     = JBmpInit(NULL,(int)themovie->wmovheader.xsize,(int)themovie->wmovheader.ysize,themovie->bmpdata);

//  view = JViewWinInit(NULL, themovie->bmp);
//  scr  = JScrInit(NULL, view, 0);

  controlcontainer = JCntInit(NULL);

//  JCntAdd(window, scr);
  JCntAdd(window,themovie->bmp);
  JCntAdd(window, controlcontainer);

  ((JCnt *)controlcontainer)->Orient = JCntF_LeftRight;

  themovie->but_start = JIbtInit(NULL,8,8, ico_flushleftarrow8,  ico_flushleftarrow8_inv);
  themovie->but_rfast = JIbtInit(NULL,8,8, ico_doubleleftarrow8, ico_doubleleftarrow8_inv);
  themovie->but_rplay = JIbtInit(NULL,8,8, ico_playleftarrow8,   ico_playleftarrow8_inv);
  themovie->but_pause = JIbtInit(NULL,8,8, ico_pausebars8,       ico_pausebars8_inv); 
  themovie->but_play  = JIbtInit(NULL,8,8, ico_playrightarrow8,  ico_playrightarrow8_inv);
  themovie->but_fast  = JIbtInit(NULL,8,8, ico_doublerightarrow8,ico_doublerightarrow8_inv);
  themovie->but_end   = JIbtInit(NULL,8,8, ico_flushrightarrow8, ico_flushrightarrow8_inv);
  themovie->but_loop  = JIbtInit(NULL,8,8, ico_looparrows,       ico_looparrows_inv);

  JWSetData(themovie->but_start, themovie);
  JWSetData(themovie->but_rfast, themovie);
  JWSetData(themovie->but_rplay, themovie);
  JWSetData(themovie->but_pause, themovie);
  JWSetData(themovie->but_play,  themovie);
  JWSetData(themovie->but_fast,  themovie);
  JWSetData(themovie->but_end,   themovie);
  JWSetData(themovie->but_loop,  themovie);

  JCntAdd(controlcontainer, themovie->but_start);
  JCntAdd(controlcontainer, themovie->but_rfast);
  JCntAdd(controlcontainer, themovie->but_rplay);
  JCntAdd(controlcontainer, themovie->but_pause);
  JCntAdd(controlcontainer, themovie->but_play);
  JCntAdd(controlcontainer, themovie->but_fast);
  JCntAdd(controlcontainer, themovie->but_end);
  JCntAdd(controlcontainer, themovie->but_loop);

  JWinCallback(themovie->but_start,JBut,Clicked,movetostart);
  JWinCallback(themovie->but_end,  JBut,Clicked,movetoend);

  JWinCallback(themovie->but_pause,JBut,Clicked,pauseplayback);

  JWinCallback(themovie->but_rfast,JBut,Clicked,playadjust);
  JWinCallback(themovie->but_fast, JBut,Clicked,playadjust);
  JWinCallback(themovie->but_rplay,JBut,Clicked,playadjust);
  JWinCallback(themovie->but_play, JBut,Clicked,playadjust);

  JWinCallback(themovie->but_loop, JBut,Clicked,toggleloopflag);

  themovie->statustext = JTxfInit(NULL);
  JTxfSetText(themovie->statustext, "Buffering...");
  JCntAdd(controlcontainer, themovie->statustext);

  //SET the WINDOW
  JCntGetHints(window,&sizes);

  JWSetBounds(window,32,32,sizes.PrefX,sizes.PrefY);
  JWSetMin(window,48,32);
  JWSetMax(window,sizes.PrefX,sizes.PrefY);
  JWndSetProp(window);

  JWinShow(window);

  printf("wave bytes: %ld\n", themovie->wmovheader.wavbytes);
  printf("sid bytes:  %ld\n", themovie->wmovheader.sidbytes);
  printf("framecount: %ld\n", themovie->wmovheader.framecount);
  printf("xsize:      %d\n",  themovie->wmovheader.xsize);
  printf("ysize:      %d\n",  themovie->wmovheader.ysize);
  printf("framerate:  %d\n",  themovie->wmovheader.framerate);

  retexit(1);

  if(!getpreview(themovie))
    printf("\nLoading preview.\n");

  if(themovie->wmovheader.wavbytes) {
    prepwavaudio(themovie);
    changeplayrate(themovie, STANDARD);
    newThread(dowavaudio, STACK_DFL, themovie); 
  } else {
    changeplayrate(themovie, STANDARD);
    newThread(donoaudio, STACK_DFL, themovie);
  }

  JAppLoop(app);
}

