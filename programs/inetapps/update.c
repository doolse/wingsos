#include <stdio.h>
#include <stdlib.h>
#include <wgslib.h>
#include <string.h>
#include <unistd.h>
#include <console.h>

int filebuf[1024];

void main(int argc, char * argv[]) {
  FILE * fp;
  FILE * lcfile;
  char byte;
  char * tfile = NULL;
  char * buf   = NULL;
  int  inbytes = 0;
  int  size    = 0;

  if(argc != 2){
    printf("Usage: Update filename\n");
    exit(-1);
  }

  tfile = (char *)malloc(strlen(argv[1]) + 2);
  sprintf(tfile, "~%s", argv[1]);
  
  fp = fopen("/dev/tcp/king.igs.net:80", "r+");

  if(!fp) {
    printf("Connection to server could not be established. Try again later.\n");
    exit(-1);
  }

  printf("Connection to Primary Server established\n");
  fprintf(fp, "GET /~billnacu/wings/wgsdl/%s HTTP/1.0\n\n", argv[1]);    

  fflush(fp);
  getline(&buf, &size, fp);

  if((buf[0] =='H')&&(buf[8] == ' ')&&(buf[9] =='2')) {

    printf("file found.\n");

    lcfile = fopen(tfile, "w");

    //Rip past http crap. 

    do {
      getline(&buf, &size, fp);
    } while(strlen(buf) > 2);

    //Read the File... and Save it out locally!

    do {
      inbytes = fread(filebuf, 1, 1024, fp);
      if(inbytes > 0)
        fwrite(filebuf, 1, inbytes, lcfile);
      printf("+-");
      fflush(stdout);
    } while(inbytes > 0);

    fclose(fp);
    fclose(lcfile);

    spawnlp(S_WAIT, "mv", tfile, argv[1], NULL);

    printf("\n\nFile %s Updated\n", argv[1]);
  } else {
    printf("The specified file could not be found.\n");
    exit(1);
  } 
}
