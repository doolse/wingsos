#include <stdio.h>
#include <stdlib.h>
#include <wgslib.h>

char buffer[512];

void helptext(char * msg) {
  if(msg != "")
    fprintf(stderr, "%s\n", msg);
  fprintf(stderr, "USAGE: gethttp [-h] [-s path/localfile] -a address -f path/remotefile\n");
  exit(1);
}

void memerror() {
  fprintf(stderr, "Memory allocation error\n");
  exit(1);
}

void main(int argc, char * argv[]) {
  FILE * fp;
  FILE * localfile;
  char * address  = NULL;
  char * pathfile = NULL;
  char * savepath = NULL;
  char * buf      = NULL;
  int size        = 0;
  int ch          = 0;
  int showheader  = 0;

  if(argc < 3)
    helptext("");

  while((ch = getopt(argc, argv, "?hs:a:f:")) != EOF) {
    switch(ch) {
      case '?':
        helptext("");
      break;
      case 'h':
        showheader = 1;
      break;
      case 's':
        savepath = optarg;
      break;
      case 'a':
        address = optarg;
      break;
      case 'f':
        pathfile = optarg;
      break;
      default:
        helptext("");
      break;
    }
  }  

  sprintf(buffer, "/dev/tcp/%s:80", address);

  if(!(fp = fopen(buffer, "r+")))
    helptext("bad address");

  fflush(fp);

  fprintf(fp, "GET %s HTTP/1.0\n\n", pathfile);

  fflush(fp);

  if(savepath != NULL) {
    localfile = fopen(savepath, "w");
    if(!localfile)
      helptext("Unable to open localfile for saving.\nCheck to make sure the path you specified exists\n");
  }

  while(EOF != getline(&buf, &size, fp)) {
    if(buf[0] == 0x0a)
      break;
    else if(showheader) 
      fprintf(stderr, "%s", buf); 
  }

  if(savepath) {
    while(EOF != getline(&buf, &size, fp)) {
      fprintf(localfile, "%s", buf);    
    }
    fclose(localfile);
  } else {
    while(EOF != getline(&buf, &size, fp)) {
      printf("%s", buf);
    }
  }
}

