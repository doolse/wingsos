#include <stdio.h>
#include <stdlib.h>
#include <wgslib.h>

char buffer[512];

void helptext(char * msg) {
  if(msg != "")
    fprintf(stderr, "%s\n", msg);
  fprintf(stderr, "USAGE: gethttp address path/filename\n");
  exit(1);
}

void main(int argc, char * argv[]) {
  FILE * fp;
  int size = 0;
  char * buf = NULL;

  if(argc < 3)
    helptext("");

  sprintf(buffer, "/dev/tcp/%s:80", argv[1]);

  fp = fopen(buffer, "r+");

  if(!fp)
    helptext("bad address");

  fflush(fp);

  fprintf(fp, "GET %s HTTP/1.0\n\n", argv[2]);

  fflush(fp);

  while(-1 != getline(&buf, &size, fp)) {
    printf("%s", buf);
  }

}
