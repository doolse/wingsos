#include "vidheader.h"

mheader newheader;
mheader_old oldheader;

void main(int argc, char * argv[]) {
  FILE * infp, *outfp;
  char * outfname, * inbuf;
  unsigned int readsize;

  outfname = malloc(17);

  sprintf(outfname,"~%s",argv[1]);

  infp = fopen(argv[1],"r");
  outfp = fopen(outfname,"w");

  fread(&oldheader,sizeof(mheader_old),1,infp);

  newheader.id[0] = oldheader.id[0];
  newheader.id[1] = oldheader.id[1];
  newheader.id[2] = oldheader.id[2];
  newheader.id[3] = oldheader.id[3];

  newheader.framecount = oldheader.framecount;
  newheader.xsize = oldheader.xsize;
  newheader.ysize = oldheader.ysize;
  newheader.framerate = oldheader.framerate;
  newheader.numofpreviewframes = 0;
  newheader.previewfps = 0;
  
  newheader.wavbytes = oldheader.wavbytes;
  newheader.sidbytes = 0;

  fwrite(&newheader,sizeof(mheader),1,outfp);
  
  inbuf = malloc(64000);
  readsize = 64000;

  while(1) {
    readsize = fread(inbuf,1,readsize,infp);
    fwrite(inbuf,1,readsize,outfp);
    if(readsize < 64000)
      break;
  }
  free(inbuf);

  fclose(infp);
  fclose(outfp);
}
