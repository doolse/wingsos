#include <wgslib.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void helptext() {
  printf("USAGE: add [-a][-z] string [directory]\nAdds string to beginning or end of all files in the current directory.\n");
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
  filename  = malloc(33);

  if(argc == 4) {
    dir = opendir(argv[3]);
    chdir(argv[3]);
  } else
    dir = opendir(".");

  if(strlen(str) > 15)
    str[15] = 0;

  while(entry = readdir(dir)) {
    if(strlen(entry->d_name) < 16) {
      if(side) {
        //end
        sprintf(filename, "%s%s", entry->d_name, str);
      } else {
        //start
        sprintf(filename, "%s%s", str, entry->d_name);
      }
      if(strlen(filename) > 16)
        filename[16] = 0;
      sprintf(renamestr, "mv \"%s\" \"%s\"", entry->d_name, filename);
      system(renamestr);
      //printf("%s\n", renamestr);
    }
  }  
  closedir(dir);
}
