#include <wgslib.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void helptext() {
  printf("USAGE: trim [-a][-z] string [directory]\nRemoves matching string from beginning or end of all files in the current directory.\n");
  exit(1);
}

void main(int argc, char * argv[]) {
  int side, searchlen;
  char * str, *filename, *renamestr;
  DIR *dir;
  struct dirent *entry;
  struct stat buf;

  if(argc < 3)
    helptext();

  if(!strcmp(argv[1],"-a"))
    side = 0;
  else if(!strcmp(argv[1], "-z"))
    side = 1;
  else
    helptext();

  str = argv[2];
  searchlen = strlen(str);
  renamestr = malloc(80);

  if(argc == 4)
    dir = opendir(argv[3]);
  else
    dir = opendir(".");

  while(entry = readdir(dir)) {
    filename = entry->d_name;

    if(strlen(filename) > searchlen) {
      if(side) {
        //end
        filename = filename + strlen(filename) - searchlen;
        if(!strcmp(filename,str)) {
          filename = strdup(entry->d_name);
          filename[strlen(filename)-searchlen] = 0;
          sprintf(renamestr, "mv \"%s\" \"%s\"", entry->d_name, filename);
          system(renamestr);
          //printf("%s\n", renamestr);
          free(filename);
        }
      } else {
        //start
        if(!strncmp(filename, str, searchlen)) {
          sprintf(renamestr, "mv \"%s\" \"%s\"", filename, filename+searchlen);
          system(renamestr);
          //printf("%s\n", renamestr);
        }
      }
    }
  }  
  closedir(dir);
}
