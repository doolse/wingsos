#include <stdio.h>
#include <wgslib.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct strings_s {
  char * string;
} strings;

strings stringarray[256];

int index = 0;
int recurse = 0;
int random = 0;
int silent = 0;

char * silentstr = " >/dev/null 2>/dev/null";

void makeplaylistfromdir(char * directory);

void helptext() {
  fprintf(stderr, "Usage: playlist [-r][-R][-s][-d /start/directory/path/]\n       -r  recurse into directories\n       -R  randomize playlist\n       -s  suppress sound apps text output\n       -d  default starting directory");
}

void main (int argc, char * argv[]) {
  int ch = 0;
  char * startdir = NULL;
  char * tempptr;
  int randomnum, i;
  unsigned int divisor;

  printf("#!sh\n");
  printf("# Greg's Playlist Generator. 2003\n");

  while((ch = getopt(argc, argv, "sRrhd:")) != EOF) {
    switch(ch) {
      case 's':
        silent = 1;
      break;
      case 'r':
        recurse = 1;
      break;
      case 'R':
        random = 1;
      break;
      case 'h':
        helptext();
        exit(1);
      break;
      case 'd':
        startdir = optarg;
      break;
    }
  }

  //Recurse! Recurse and be Merry!!

  if(startdir)
    makeplaylistfromdir(startdir);
  else 
    makeplaylistfromdir("./");

  index--;

  if(random) {
    divisor = 65536/index;  

    for(i = 0; i < index; i++) {
      randomnum = rand()/divisor;
  
      randomnum = abs(randomnum);  

      tempptr = stringarray[i].string;

      stringarray[i].string = stringarray[randomnum].string;
      stringarray[randomnum].string = tempptr;

      //printf("%d,", randomnum);
    }
  }

  for(i = 0; i < index; i++)
    printf("%s",stringarray[i].string);

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

      if(directory[strlen(directory)-1] != '/') {
        nextdir = (char *)malloc(strlen(directory) + strlen(entry->d_name)+3);
        if(nextdir == NULL)
          exit(1);
        sprintf(nextdir, "%s/%s/", directory, entry->d_name);
      } else {
        nextdir = (char *)malloc(strlen(directory) + strlen(entry->d_name)+2);
        if(nextdir == NULL)
          exit(1);
        sprintf(nextdir, "%s%s/", directory, entry->d_name);
      }
      makeplaylistfromdir(nextdir);
      free(nextdir);
      nextdir = NULL;
    } else if(index < 256){

      if(
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 'w') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'a') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'v')) {
        stringarray[index].string = (char *)malloc(strlen("wavplay \n")+strlen(directory)+strlen(entry->d_name)+strlen(silentstr)+1);
        if(silent)
          sprintf(stringarray[index].string,"wavplay %s%s%s\n", directory,entry->d_name,silentstr);
        else
          sprintf(stringarray[index].string,"wavplay %s%s\n", directory,entry->d_name);
        index++;
      } else if (
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 's') && 
         (entry->d_name[strlen(entry->d_name)-2] == '3') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'm')) {
        stringarray[index].string = (char *)malloc(strlen("josmod -h 11000 \n")+strlen(directory)+strlen(entry->d_name)+strlen(silentstr)+1);
        if(silent)
          sprintf(stringarray[index].string,"josmod -h 11000 %s%s%s\n", directory,entry->d_name, silentstr);
        else
          sprintf(stringarray[index].string,"josmod -h 11000 %s%s\n", directory,entry->d_name);
        index++;
      } else if (
         (entry->d_name[strlen(entry->d_name)-3] == '.') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'x') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'm')) {
        stringarray[index].string = (char *)malloc(strlen("josmod -h 11000 \n")+strlen(directory)+strlen(entry->d_name)+strlen(silentstr)+1);
        if(silent)
          sprintf(stringarray[index].string,"josmod -h 11000 %s%s%s\n", directory,entry->d_name, silentstr);
        else
          sprintf(stringarray[index].string,"josmod -h 11000 %s%s\n", directory,entry->d_name);
        index++;
      } else if (
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 'm') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'o') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'd')) {
        stringarray[index].string = (char *)malloc(strlen("josmod \n")+strlen(directory)+strlen(entry->d_name)+strlen(silentstr)+1);
        if(silent)
          sprintf(stringarray[index].string,"josmod %s%s%s\n", directory,entry->d_name, silentstr);
        else
          sprintf(stringarray[index].string,"josmod %s%s\n", directory,entry->d_name);
        index++;
      } else if (
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 'd') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'a') && 
         (entry->d_name[strlen(entry->d_name)-1] == 't')) {
        stringarray[index].string = (char *)malloc(strlen("sidplay \n")+strlen(directory)+strlen(entry->d_name)+strlen(silentstr)+1);
        if(silent)
          sprintf(stringarray[index].string,"sidplay %s%s%s\n", directory,entry->d_name, silentstr);
        else
          sprintf(stringarray[index].string,"sidplay %s%s\n", directory,entry->d_name);
        index++;
      } else if (
         (entry->d_name[strlen(entry->d_name)-4] == '.') && 
         (entry->d_name[strlen(entry->d_name)-3] == 's') && 
         (entry->d_name[strlen(entry->d_name)-2] == 'i') && 
         (entry->d_name[strlen(entry->d_name)-1] == 'd')) {
        stringarray[index].string = (char *)malloc(strlen("sidplay \n")+strlen(directory)+strlen(entry->d_name)+strlen(silentstr)+1);
        if(silent)
          sprintf(stringarray[index].string,"sidplay %s%s%s\n", directory,entry->d_name, silentstr);
        else
          sprintf(stringarray[index].string,"sidplay %s%s\n", directory,entry->d_name);
        index++;
      }
    }
  }

  closedir(dir);
}
