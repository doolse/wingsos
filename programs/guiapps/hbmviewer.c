#include <winlib.h>
#include <stdio.h>
#include <stdlib.h>

int imgsize;

void main(int argc, char * argv[]) {
  void * app, * window, * scr, * view, *bmp, *bmpdata;
  FILE * fp;
  int x, y;

  if(argc < 4) {
    x = atoi(argv[2]);
    y = atoi(argv[3]);
  } else {
    x = 320;
    y = 200;
  }

  imgsize = (y*(x/8)) + ((x/8)*(y/8));

  app = JAppInit(NULL,0);
  window = JWndInit(NULL, "HBM Viewer", JWndF_Resizable);

  JWSetBounds(window, 0,0, x, y);

  JAppSetMain(app, window);

  if(argc < 2) {
    fprintf(stderr, "USAGE: hbmviewer path/filename [x][y]\n");
    fprintf(stderr, "       Default X x Y is 320x200\n");
    exit(1);
  }

  fp = fopen(argv[1], "rb");
  if(!fp)
    exit(1);
  bmpdata = malloc(imgsize);
  
  fseek(fp, 2, SEEK_CUR);
  fread(bmpdata, 1, imgsize, fp);
  fclose(fp);

  bmp  = JBmpInit(NULL, x, y, bmpdata);
  view = JViewWinInit(NULL, bmp);
  scr  = JScrInit(NULL, view, 0);

  JCntAdd(window, scr);
  JWinShow(window);

  retexit(1);
  JAppLoop(app);
}
