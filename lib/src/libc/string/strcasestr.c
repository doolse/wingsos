#include <string.h>

char * strcasestr(char * haystack, char * needle) {
  char * ptr;
  int len;
  char firstchar;

  firstchar = needle[0];
  len = strlen(needle);

  ptr = haystack;
  while(ptr != NULL) {
    ptr = strchr(ptr, firstchar);
    if(ptr) {
      if(!strncasecmp(ptr, needle, len))
        return(ptr);
      ptr++;
    }
  }

  //Switch case of search character
  if(firstchar > 64 && firstchar < 91) 
    firstchar = firstchar + 32;
  else if(firstchar > 96 && firstchar < 123)
    firstchar = firstchar - 32;

  ptr = haystack;
  while(ptr != NULL) {
    ptr = strchr(ptr, firstchar);
    if(ptr) {
      if(!strncasecmp(ptr, needle, len))
        return(ptr);
      ptr++;
    }
  }

  return(NULL);
}
