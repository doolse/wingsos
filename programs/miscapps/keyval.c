#include <stdio.h>
#include <wgslib.h>

void main() {
  int i;

  con_init();

  while(1) {
    i = con_getkey();
    printf("%d\n",i);
    con_update();
  }

}
