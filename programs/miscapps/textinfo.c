#include <stdio.h>
#include <stdlib.h>
#include <wgslib.h>
#include <string.h>
#include <unistd.h>

int cr = 0;
int lf = 0;
int pet = 0;
int asc = 0;

void main(int argc, char * argv[]){
  FILE * fp;
  int i = 0;  
  int c = 0;

  if(argc < 2 || argc > 2) {
    printf("Usage: textinfo filename.txt\n");
    exit(-1);
  }
  fp = fopen(argv[1], "r");
  if(!fp){
    printf("Couldn't open file!\n");
    exit(-1);
  }

  while((c=fgetc(fp)) && (i < 2049)) {
    i++;
    if(c == 13)
      cr=1;
    else if(c == 10)
      lf=1;
    else if(c == 97 ||c == 101 ||c == 105 ||c == 111 ||c == 117)
      asc=1;
    else if(c == 193 ||c == 197 ||c == 201 ||c == 207 ||c == 213)
      pet=1;
  } 

  printf("cr %d, lf %d, asc %d, pet %d\n", cr, lf, asc, pet);

  if(lf && cr && asc && !pet) {
    printf("This file has Windows/Dos Line endings, and Appears to be in ASCII.  This file was most likely Created on a MicroSoft PC\n");
    exit(-1);
  } else if (!lf && cr && asc && !pet) {
    printf("This file has Both Mac, and C64 Line endings, But it Appears to be in ASCII. Therefore this file was most likes created on a Macintosh.\n");
    exit(-1);
  } else if (!lf && cr && !asc && pet) {
    printf("This file has Both Mac and C64 Line endings, But it Appears to be in PETASCII. This file was most likely created on a C64.\n");
    exit(-1);
  } else if (lf && !cr && asc && !pet) {
    printf("This file has Jos and Unix Style Line endings. And it appears to be in ASCII.  This file was most likely created By JOS, Unix or Linux.\n");
    exit(-1);
  } else if ((!lf && cr && asc && pet) || (!lf && cr && !asc && !pet)) {
    printf("I was not able to determine if the file is PETASCII or ASCII, But it has C64 And Mac line endings. It may have been created on Either a Mac or a C64.\n");
    exit(-1);
  } else if ((lf && !cr && asc && pet) || (lf && !cr && !asc && !pet)) {
    printf("I was not able to determine if the file is PETASCII or ASCII, But it has Unix and JOS line endings. It was most likely created in Unix, JOS or Linux.\n");
    exit(-1);
  } else {
    printf("This is an abnormal file... it's probably made by windows or dos.\n");
    exit(-1);
  }
}
