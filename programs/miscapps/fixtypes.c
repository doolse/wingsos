#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <console.h>
#include <wgslib.h>
#include <dirent.h>
#include <sys/stat.h>

void main() {
  char * input = NULL;
  int size = 0;
  DIR * dir;
  struct dirent *entry;
  struct stat buf;
  char * tempstr;

  printf("\nForce all files in this directory to\nend with the extension: (3 characters) ");
  con_update();
  getline(&input,&size,stdin);

  if(input[0] == '.')
    input++;

  if(strlen(input) > 3)
    input[3] = 0;

  dir = opendir("./");
  if(!dir) {
    printf("\nError: directory could not be opened.\n");
    exit(1);
  }

  tempstr = malloc(17);

  while(entry = readdir(dir)) {
    if(entry->d_type != 6) {
      if(strlen(entry->d_name) >= strlen(input)) {
        if(strcasecmp(&entry->d_name[strlen(entry->d_name)-strlen(input)],input)) {
          if(strlen(entry->d_name) + strlen(input) > 15) {
            sprintf(tempstr,"%s",entry->d_name);
            tempstr[strlen(tempstr) + (15-(strlen(tempstr)+strlen(input)))] = 0;
            sprintf(tempstr,"%s.%s",tempstr,input);
          } else {
            sprintf(tempstr,"%s.%s",entry->d_name,input);
          }
          spawnlp(S_WAIT,"mv",entry->d_name,tempstr,NULL);
        }
      } else {
        sprintf(tempstr,"%s.%s",entry->d_name,input);
        spawnlp(S_WAIT,"mv",entry->d_name,tempstr,NULL);
      }
    }
  }
}
