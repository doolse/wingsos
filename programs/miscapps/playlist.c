#include <stdio.h>
#include <wgslib.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


int recurse = 0;
void makeplaylistfromdir(char * directory);

void helptext() {
  fprintf(stderr, "Usage: playlist [-?][-r][-d /start/directory/path/]\n");
}

void main (int argc, char * argv[]) {
  int ch = 0;
  char * startdir = NULL;

  printf("#!sh\n");
  printf("# Greg/DAC's Recursive Playlist Generator. 2002\n");

  while((ch = getopt(argc, argv, "r?d:")) != EOF) {
    switch(ch) {
      case 'r':
        recurse = 1;
      break;

      case '?':
        helptext();
        exit(1);
      break;

      case 'd':
        startdir = optarg;
      break;
    }
  }

  //Recurse! Recurse and be Merry!!

  if(startdir == NULL)
    makeplaylistfromdir("./");
  else 
    makeplaylistfromdir(startdir);
  
}

void makeplaylistfromdir(char * directory) {

DIR * dir;
struct dirent *entry;
char * nextdir;

  dir = opendir(directory);

  while(entry = readdir(dir)) {
    if(entry->d_type == 6 && recurse) {
      //printf("string=%s%s/ , d=%d, n=%d, total=%d\n", directory, entry->d_name, strlen(directory), strlen(entry->d_name), strlen(directory)+strlen(entry->d_name));
      //exit(1);

      nextdir = malloc(strlen(directory) + strlen(entry->d_name)+3);
      if(nextdir == NULL)
        exit(1);
      sprintf(nextdir, "%s%s/", directory, entry->d_name);
      makeplaylistfromdir(nextdir);
      free(nextdir);
      nextdir = NULL;
    } else {

      if(
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 'w') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'a') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'v')) {
        printf("wavplay %s%s\n", directory,entry->d_name);
      } else if (
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 's') && 
         (entry->d_name[strlen(entry->d_name)-2] == '3') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'm')) {
        printf("josmod -h 11000 %s%s\n", directory,entry->d_name);
      } else if (
         (entry->d_name[strlen(entry->d_name)-3] == '.') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'x') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'm')) {
        printf("josmod -h 11000 %s%s\n", directory,entry->d_name);
      } else if (
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 'm') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'o') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'd')) {
        printf("josmod %s%s\n", directory,entry->d_name);
      } else if (
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 'd') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'a') && 
         (entry->d_name[strlen(entry->d_name)-1] == 't')) {
        printf("sidplay %s%s\n", directory,entry->d_name);
      } else if (
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 's') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'i') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'd')) {
        printf("sidplay %s%s\n", directory,entry->d_name);
      }

    }
  }

  closedir(dir);
}
