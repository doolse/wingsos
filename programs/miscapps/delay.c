#include <console.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <stdlib.h>

void main(int argc, char * argv[]) {
  int channel,delay;
  char * msgp;

  con_init();

  channel = makeChan();

  if(argc < 2)
    delay = 10;
  else {
    delay = atoi(argv[1]);
    delay *= 1000;
  }

  setTimer(-1,delay,0,channel,PMSG_Alarm);
  recvMsg(channel,(void *)&msgp);

  con_end();

  exit(1);
}
