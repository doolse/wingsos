#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <wgslib.h>
#include <unistd.h>

void transformap();
void transformpa();
void fixending(int c, char * lending);
int c = 0;
FILE * input;

void helptext() {
  printf("Usage: textconvert -ap -pa -dosc64 -dosjos -c64jos -josc64 textfile\n");
  printf("       ap -- ascii to petascii\n");
  printf("       pa -- petascii to ascii\n");
  printf("       dosc64 -- change line endings from dos to c64... \n");
  printf("       either ap or pa is required, line ending changes are optional\n");
  exit(-1);
}

void main(int argc, char * argv[]) {
  char * lending = NULL;

  if(argc < 3 || argc >4)
    helptext();
  
  if(argc == 4) {
    input = fopen(argv[3], "r");
    lending = strdup(argv[2]);
  } else
    input = fopen(argv[2], "r");

  if(!input) {
    printf("Could not open file\n");
    exit(-1);
  }

  if(!strcmp(argv[1], "-ap")) {
    while((c=fgetc(input))!=-1){
      if((c == 10 || c == 13) && lending)
        fixending(c, lending);
      else
        transformap();
    } 
  } else if(!strcmp(argv[1], "-pa")) {
    while((c=fgetc(input))!=-1){
      if((c == 10 || c == 13) && lending)
        fixending(c, lending);
      else
        transformpa();
    }
  } else
		helptext();
}

void transformap(){
  if(c < 123 && c > 96)
    printf("%c", c-32);
  else if(c < 91 && c > 64)
    printf("%c", c+128);
  else
    printf("%c", c);
 fflush(stdout);
}

void transformpa(){
  if(c < 91 && c > 64)
    printf("%c", c+32);
  else if(c < 219 && c > 192)
    printf("%c", c-128);
  else
    printf("%c", c);
 fflush(stdout);
}

void fixending(int c, char * lending) {
  if(lending[1] == 'd' && lending[4] == 'c'){
    if(c == 13)
      printf("\r");
  }
  if(lending[1] == 'd' && lending[4] == 'j'){
    if(c == 10)
      printf("\n");
  }
  if(lending[1] == 'c' && lending[4] == 'j'){
    if(c == 13)
      printf("\n");
  }
  if(lending[1] == 'j' && lending[4] == 'c'){
    if(c == 10)
      printf("\r");
  }
  fflush(stdout);
}

