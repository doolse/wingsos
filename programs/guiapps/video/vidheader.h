//Movie player/composer common header file

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <winlib.h>
#include <unistd.h>

#define STANDARD     0
#define HIGHSPEED    1

#define RCHNK_FORMAT 1
#define RCHNK_DATA   2

typedef struct framestr_s {
  void * frame;
} framestruct;

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

typedef struct header_s {
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

typedef struct header_s_old {
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
} mheader_old;

typedef struct movie_s {
  char * filename;
  int playadjustchannel;
  int frameflipchannel;
  FILE *fp;

  //controls
  int playadjusted;
  int reverse;
  int pause;
  int loopflag;
  struct wmutex pausemutex;
  int endofpreview;
  int playratepercent;
  int cframerate;
  int csamplerate;
  framestruct * cframe;
  char * cwavposition;

  //window elements
  void * bmp, *bmpdata;
  void * but_start;
  void * but_rfast, *but_rplay, *but_pause, *but_play, *but_fast;
  void * but_end, * but_loop;
  void * statustext;
  char * laststatustext;

  //video info/data
  mheader wmovheader;
  int imgsize;
  framestruct * prvframes;
  framestruct * firstframe;
  framestruct * lastframe;
 
  //audio info
  int digiChan;
  RifForm Format;
  char * startofwavbuf;
  char * endofwavbuf;
  long totalwavsize;

} movie;
