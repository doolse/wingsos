#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <wgslib.h>
#include <stdlib.h>

int recurse = 0;
int summary = 0;
unsigned long total = 0;

void helptext() {
  fprintf(stderr, "USAGE: du [-r][-s]\n      recurse, summarize\n");
  exit(1);
}

void getdirinfo(char * dirpath);

void main(int argc, char * argv[]) {
  int ch = 0;

  while((ch = getopt(argc, argv, "rsh")) != EOF) {
    switch(ch) {
      case 'r':
        recurse = 1;
      break;
      case 's':
        summary = 1;
      break;
      case 'h':
        helptext();
      break;
    }
  }
  getdirinfo("./");

  printf("%10ld KB\n", total);
} 

void getdirinfo(char *dirpath) {
  DIR *dir;
  struct dirent *entry;
  struct stat buf;
  char * fullname = NULL;
  char * nextdir = NULL;
  unsigned long subtotal = 0;
  
  dir = opendir(dirpath);

  if(dir) {
  
  while(entry = readdir(dir)) {
    if((entry->d_type == 6) && recurse) {
      nextdir = (char *)malloc(strlen(dirpath) + strlen(entry->d_name) +3);
      if(nextdir == NULL) {
        printf("get real, couldn't allocate %d bytes\n", strlen(dirpath) + strlen(entry->d_name) +3);
        exit(1);
      }
      sprintf(nextdir, "%s%s/", dirpath, entry->d_name);
      getdirinfo(nextdir);
      free(nextdir);
      nextdir = NULL;
    } else {
      fullname = fpathname(entry->d_name, dirpath, 1);
      stat(fullname, &buf);
      printf("%ld\n", buf.st_size);
      subtotal = subtotal + (buf.st_size/1024);
      free(fullname);
    }
  }
  if(!summary)
    printf("%10ld KB %s\n", subtotal, dirpath);
  total = total + subtotal;
  }
}
