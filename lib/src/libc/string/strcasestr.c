#include <string.h>
#include <stdlib.h>

char * strcasestr(char * big, char * little) {
  char * ptr;
  int len;
  char firstchar;

  firstchar = little[0];
  len = strlen(little);

  ptr = big;
  while(ptr != NULL) {
    ptr = strchr(ptr, firstchar);
    if(ptr) {
      if(!strncasecmp(ptr, little, len))
        return(ptr);
      ptr++;
    }
  }

  //Switch case of search character
  if(firstchar > 64 && firstchar < 91) 
    firstchar = firstchar + 32;
  else if(firstchar > 96 && firstchar < 123)
    firstchar = firstchar - 32;

  ptr = big;
  while(ptr != NULL) {
    ptr = strchr(ptr, firstchar);
    if(ptr) {
      if(!strncasecmp(ptr, little, len))
        return(ptr);
      ptr++;
    }
  }

  return(NULL);
}
