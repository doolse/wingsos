#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <wgslib.h>

int recurse = 0;
int summary = 0;
int total = 0;

void helptext() {
  fprintf(stderr, "USAGE: du [-r][-s]\n      recurse, summarize\n");
  exit(1);
}

void getdirinfo(char * dirpath);

void main(int argc, char * argv) {
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

  printf("%d\n", total);
} 

void getdirinfo(char *dirpath) {
  DIR *dir;
  struct dirent *entry;
  struct stat buf;
  char * fullname;
  char * nextdir;
  int subtotal = 0;
  
  dir = opendir(dirpath);
  
  while(entry = readdir(dir)) {
    if(entry->d_type == 6 && recurse) {
      nextdir = (char *)malloc(strlen(dirpath) + strlen(entry->d_name) +3);
      if(nextdir == NULL)
        exit(1);
      sprintf(nextdir, "%s%s/", dirpath, entry->d_name);
      getdirinfo(nextdir);
      free(nextdir);
    } else {
      fullname = fpathname(entry->d_name, dirpath, 1);
      stat(fullname, &buf);
      subtotal = subtotal + buf.st_size;
      free(fullname);
    }
  }
  if(!summary)
    printf("%d %s\n", subtotal, dirpath);
  total = total + subtotal;
}
