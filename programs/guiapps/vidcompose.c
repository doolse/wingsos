//Movie composer; Assembles .wav, .sid and .rvd into a complete movie. 

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <winlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

void showhelp() {
  printf("USAGE: vidcompose -v file.rvd -r framerate -c framecount -o outfile\n                  -x xframeresolution -y yframeresolution\n                  [-w wavfile -b wavbytes] [-s sidfile]\n                  [-n numofpreviewframes -p preview.rvd -f previewfps]\n");
  exit(1);
}

void main(int argc, char * argv[]) {
  mheader *movheader;
  int ch, c, i;
  char * videofile, *wavfile, *sidfile, *outfilename, *previewrvd;
  struct stat buf;
  char *fullname, *inbuffer;
  FILE * outfp, *infp;
  int size = 0;
  unsigned int readsize;
  char * textbuf = NULL;

  videofile = wavfile = sidfile = outfilename = previewrvd = NULL;

  movheader = (mheader *)malloc(sizeof(mheader));
  
  movheader->id[0] = 'W';
  movheader->id[1] = 'M';
  movheader->id[2] = 'O';
  movheader->id[3] = 'V';

  movheader->numofpreviewframes = 0;

  movheader->framecount = 0;
  movheader->framerate  = 0;
  movheader->xsize      = 0;
  movheader->ysize      = 0;
  movheader->previewfps = 0;

  movheader->wavbytes   = 0;
  movheader->sidbytes   = 0;

  while ((ch = getopt(argc, argv, "v:r:c:o:x:y:w:s:b:n:p:f:")) != EOF) {
    switch(ch) {

      case 'v':
        videofile = strdup(optarg);
      break;
      case 'w':
        wavfile = strdup(optarg);
      break;
      case 's':
        sidfile = strdup(optarg);
      break;
      case 'o':
        outfilename = strdup(optarg);
      break;


      case 'b':
        movheader->wavbytes = strtol(optarg, NULL, 10);
      break;

      case 'r':
        movheader->framerate = 1000/atoi(optarg);
      break;
      case 'c':
        movheader->framecount = strtoul(optarg, NULL, 10);
      break;


      case 'x':
        movheader->xsize = (uint)atoi(optarg);
      break;
      case 'y':
        movheader->ysize = (uint)atoi(optarg);
      break;

      case 'n':
        movheader->numofpreviewframes = atoi(optarg);
      break;
      case 'p':
        previewrvd = strdup(optarg);
      break;
      case 'f':
        movheader->previewfps = atoi(optarg);
      break;

      default:
        showhelp();
    }
  }

  if(!(outfilename && videofile && movheader->framecount && movheader->framerate && movheader->xsize && movheader->ysize))
    showhelp();

  if(sidfile) {
    printf("sidfile is not yet supported\n");
    exit(1);
  }

  if(wavfile) {
    fullname = fpathname(wavfile, getappdir(), 1);
    stat(fullname, &buf);
    movheader->wavbytes = buf.st_size;
    printf("the wav '%s' is %10ld bytes long.\n", fullname, buf.st_size);
    exit(1);
  }  

  //when sid files are implemented... you can't have wav and sid together.
  if(wavfile && sidfile) {
    printf("A single movie cannot have both Sid and Wav audio.\n");
    exit(1);
  }

/*
  if(sidfile) {
    fullname = fpathname(sidfile, getappdir(), 1);
    stat(fullname, &buf);
    movheader->sidbytes = buf.st_size;
  }
*/

  if(previewrvd == NULL || movheader->previewfps == 0 || movheader->numofpreviewframes == 0) {
    movheader->numofpreviewframes = 0;
    movheader->previewfps = 0;
    previewrvd = NULL;
   }

  outfp = fopen(outfilename, "w+");

  //write out the header

  fwrite(movheader, 1, sizeof(mheader), outfp);

  if(previewrvd) {
    infp = fopen(previewrvd, "r");
    if(!infp) {
      printf("preview rvd file %s cannot be opened.\n", previewrvd);
      exit(1);
    }
    inbuffer = (char *)malloc(64000);
    readsize = 64000;
    while(1) {
      readsize = fread(inbuffer, 1, readsize, infp);
      fwrite(inbuffer, 1, readsize, outfp);
      if(readsize < 64000)
        break;
    }
    free(inbuffer);
    fclose(infp);
  }

  if(wavfile) {
    infp = fopen(wavfile, "r");
    if(!infp) {
      printf("wavfile %s cannot be opened.\n", wavfile);
      exit(1);
    }
    inbuffer = (char *)malloc(64000);
    readsize = 64000;
    while(1) {
      readsize = fread(inbuffer, 1, readsize, infp);
      fwrite(inbuffer, 1, readsize, outfp);
      if(readsize < 64000)
        break;
    }
    free(inbuffer);
    fclose(infp);
  }

  if(sidfile) {
    infp = fopen(sidfile, "r");
    if(!infp) {
      printf("sidfile %s cannot be opened.\n", sidfile);
      exit(1);
    }
    inbuffer = (char *)malloc(64000);
    readsize = 64000;
    while(1) {
      readsize = fread(inbuffer, 1, readsize, infp);
      fwrite(inbuffer, 1, readsize, outfp);
      if(readsize < 64000)
        break;
    }
    free(inbuffer);
    fclose(infp);
  }

  infp = fopen(videofile, "r");
  if(!infp) {
    printf("could not open raw video file %s.\n", videofile);
    exit(1);
  }

  inbuffer = (char *)malloc(64000);
  readsize = 64000;
  while(1) {
    readsize = fread(inbuffer, 1, readsize, infp);
    fwrite(inbuffer, 1, readsize, outfp);
    if(readsize < 64000)
      break;
  }

  fclose(infp);
  fclose(outfp);

  printf("Complete Movie File '%s' Composed!\n", outfilename);
}
