#include <wgslib.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

int    recurse = 0;
int    summary = 0;
uint32 total   = 0;

void helptext() {
  fprintf(stderr, "USAGE: du [-h][-r][-s]\n     help, recurse, summarize\n");
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

  printf("Total: %ld KB\n", total);
} 

void getdirinfo(char *dirpath) {
  DIR *dir;
  struct dirent *entry;
  struct stat buf;
  char * fullname = NULL;
  char * nextdir  = NULL;
  uint32 subtotal = 0;
  
  dir = opendir(dirpath);

  if(dir) {
  
    while(entry = readdir(dir)) {
      if((entry->d_type == 6) && recurse) {

        nextdir = (char *)malloc(strlen(dirpath) + strlen(entry->d_name) +3);
        if(nextdir == NULL) 
          exit(1);

        sprintf(nextdir, "%s%s/", dirpath, entry->d_name);
        getdirinfo(nextdir);
        free(nextdir);
        nextdir = NULL;

      } else {
  
        fullname = fpathname(entry->d_name, dirpath, 1);
        stat(fullname, &buf);
        subtotal = subtotal + buf.st_size;
        free(fullname);
      }
    }

    closedir(dir);

    if(!summary)
      printf("%10ld Bytes %s\n", subtotal, dirpath);
    total = total + (subtotal/1024);
  }
}
