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

void main(int argc, char * argv[]) {
  char * lending=NULL;
  lending = (char *)malloc(10); 
  if(lending == NULL) {
    printf("malloc error\n");
    exit(-1);
  }

  if(argc < 3 || argc >4){
    printf("Usage: textconvert -ap -pa -dosc64 -dosjos -c64jos -josc64 textfile\n");
    printf("       ap -- ascii to petascii\n");
    printf("       pa -- petascii to ascii\n");
    printf("       dosc64 -- change line endings from dos to c64... \n");
    printf("       either ap or pa is required, line ending changes are optional\n");
    exit(-1);
  }
  
  if(argc == 4) {
    input = fopen(argv[3], "r");
    strcpy(lending, argv[2]);
//    printf("%c%c%c%c\n", lending[0], lending[1], lending[2], lending[3]);
  } else {
    input = fopen(argv[2], "r");
    strcpy(lending, "NULL");
//    printf("%c%c%c%c\n", lending[0], lending[1], lending[2], lending[3]);
  }

  if(!input) {
    printf("Could not open file\n");
    exit(-1);
  }

  if(!strcmp(argv[1], "-ap")) {
    while((c=fgetc(input))!=-1){
      if((c == 10 || c == 13) && lending[1] != 'U')
        fixending(c, lending);
      else
        transformap();
    } 
  } else if(!strcmp(argv[1], "-pa")) {
    while((c=fgetc(input))!=-1){
      if((c == 10 || c == 13) && lending[1] != 'U')
        fixending(c, lending);
      else
        transformpa();
    }
  } else {
    printf("incorrect arguments\n");  
    exit(-1);
  }
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

