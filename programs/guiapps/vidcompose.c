//Movie composer; Assembles .wav, .sid and .rvd into a complete movie. 

#include "vidheader.h"

char * VERSION = "1.1";

void showhelp(char * error) {
  printf("  WiNGs Video Compose Tool v%s\n\n", VERSION);
  printf("USAGE: vidcompose -v file.rvd -r framerate -c framecount -o outfile\n");
  printf("                  -x xframeresolution -y yframeresolution\n");
  printf("                  [-w wavfile] [-s sidfile]\n");
  printf("                  [-n numofpreviewframes -p preview.rvd -f previewfps]\n");

  if(error) {
    putchar('\n');
    printf("ERROR: %s\n", error);
  }
  exit(1);
}

void main(int argc, char * argv[]) {
  mheader *movheader;
  int ch, c, i, size = 0;
  char * videofile, *wavfile, *sidfile, *previewfile, *outfilename;
  struct stat buf;
  FILE * outfp, *infp;
  unsigned int readsize;
  char *inbuffer, *textbuf = NULL;

  videofile = wavfile = sidfile = previewfile = outfilename = NULL;

  movheader = (mheader *)malloc(sizeof(mheader));
  
  strcpy(movheader->id, "WMOV");

  movheader->framecount = 0;
  movheader->framerate  = 0;
  movheader->xsize      = 0;
  movheader->ysize      = 0;

  //preview frames MUST be the same resolution as the movie frames.

  movheader->previewfps         = 0;
  movheader->numofpreviewframes = 0;

  movheader->wavbytes   = 0;
  movheader->sidbytes   = 0;

  while ((ch = getopt(argc, argv, "v:r:c:o:x:y:w:s:n:p:f:")) != EOF) {
    switch(ch) {

      case 'v':
        videofile = strdup(optarg);
      break;
      case 'r':
        movheader->framerate = 1000/atoi(optarg);
      break;
      case 'c':
        movheader->framecount = strtoul(optarg, NULL, 10);
      break;
      case 'o':
        outfilename = strdup(optarg);
      break;
      case 'x':
        movheader->xsize = (uint)atoi(optarg);
      break;
      case 'y':
        movheader->ysize = (uint)atoi(optarg);
      break;


      case 'w':
        wavfile = strdup(optarg);
      break;
      case 's':
        sidfile = strdup(optarg);
      break;
      case 'n':
        movheader->numofpreviewframes = atoi(optarg);
      break;
      case 'p':
        previewfile = strdup(optarg);
      break;
      case 'f':
        movheader->previewfps = 1000/atoi(optarg);
      break;


      default:
        showhelp("unrecognized option.");
    }
  }

  if(!(outfilename && 
       videofile && 
       movheader->framecount && 
       movheader->framerate && 
       movheader->xsize && 
       movheader->ysize))
    showhelp("missing one or more required options.");

  if(sidfile && wavfile)
    showhelp("a movie cannot have both SID and WAV audio.");

  if(sidfile) {
    if(stat(sidfile, &buf))
      showhelp("sid file not found.");
    movheader->sidbytes = buf.st_size;
    //printf("the SID '%s' is %ld bytes long.\n", sidfile, buf.st_size);
    //exit(1);
  }

  if(wavfile) {
    if(stat(wavfile, &buf))
      showhelp("wav file not found.");
    movheader->wavbytes = buf.st_size;
    //printf("the wav '%s' is %10ld bytes long.\n", wavfile, buf.st_size);
    //exit(1);
  }  

  //if any preview option is missing, clear them all

  if(!previewfile || 
     !movheader->previewfps || 
     !movheader->numofpreviewframes) {

    previewfile = NULL;
    movheader->previewfps = 0;
    movheader->numofpreviewframes = 0;
   }

  outfp = fopen(outfilename, "w+");

  //write out the header

  fwrite(movheader, 1, sizeof(mheader), outfp);

  if(previewfile) {
    infp = fopen(previewfile, "r");
    if(!infp) {
      printf("rvd preview file %s cannot be opened.\n", previewfile);
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
