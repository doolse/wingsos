//A standard Clear the Screen Util. Handy to make terminal clean.

#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]){
  int i;

  if(argc > 1) {
    if (!strcmp(argv[1], "kickass")) {
      for(i = 0; i < 25; i++){
        if(i == 8)
          printf("\t\t   Jos Kicks ass, You know it. -Greg/DAC-\n");
        else 
          printf("\n");
      }
      printf("\x1b[H");
      printf("\x1b[2J");
      fflush(stdout);
    }
  } else {
    printf("\x1b[H");
    printf("\x1b[2J");
    fflush(stdout);
  }
return(0);
}



