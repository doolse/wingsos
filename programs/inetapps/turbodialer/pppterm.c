//Separate binary for the shell authenticating term. 
//Multi threaded... but when the main thread detects ppp has 
//been initiated, it exit()'s which terminates all threads,
//and returns to the calling program.

//some of these includes are probably not necessary.

#include <stdio.h>
#include <console.h>
#include <exception.h>
#include <fcntl.h>
#include <net.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <xmldom.h>
#include <sys/types.h>
#include <sys/stat.h>

long bauds[12] = {0, 110, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400};

int globsock;

void toterminal(int *tlc) {
  FILE *stream;
  int ch;
	
  stream = fdopen(*tlc,"w");

  stdin->_flags |= _IOBUFEMP;

  while (!feof(stdin)) {
    while ((ch = getchar()) != EOF)
      fputc(ch,stream);
    fflush(stream);
  }
}

void main(int argc, char * argv[]) {
  int ch,pppcheckpos,firstchar = 0,charcount = 0;
  FILE *stream;
  struct termios T1;
  unsigned int baud=0;
  long brate,min=1000000,dif;
  char pppcheck[4];

  con_clrscr();
  con_update();

  brate = strtol(argv[2], NULL, 0);
  ch = 0;
  while(ch<11) {
    dif = bauds[ch] - brate;
    if(dif<0) 
      dif = -dif;
    if(dif<min) {
      baud = ch;
      min = dif;
    }
    ch++;
  }

  stream = fopen("/dev/ser0","w+");
  gettio(fileno(stream),&T1);
  T1.flags = TF_OPOST|TF_IGNCR;
  T1.Speed = baud;
  settio(fileno(stream),&T1);
  gettio(STDIN_FILENO,&T1);
  T1.flags &= ~(TF_ICANON|TF_ECHO|TF_ICRLF);
  settio(STDIN_FILENO,&T1);

  fprintf(stream, "atdt%s\n", argv[1]);

  charcount = strlen("atdt\n") + strlen(argv[1]) + 1;

  fclose(stream);

  stream = fopen("/dev/ser0","rb");
  gettio(fileno(stream),&T1);
  T1.flags = TF_OPOST|TF_IGNCR;
  T1.Speed = baud;
  settio(fileno(stream),&T1);
  gettio(STDIN_FILENO,&T1);
  T1.flags &= ~(TF_ICANON|TF_ECHO|TF_ICRLF);
  settio(STDIN_FILENO,&T1);

  globsock = fileno(stream);

  stream->_flags |= _IOBUFEMP;

  pppcheckpos = 0;

  while(!feof(stream)) {
    while ((ch = fgetc(stream)) != EOF) {
      if(charcount > 0) {
        //should skip messages like "NO CARRIER", or "RING", 
        //and only start counting down when it finds the 'a' in "atdt"...
        if(!firstchar && ch == 'a')
          firstchar = 1;
        if(firstchar) {
          charcount--;
        }
      } else if(charcount == 0) {
        newThread(toterminal,256,&globsock);
        charcount = -1;
      }
      putchar(ch);
      pppcheck[pppcheckpos++] = ch;
      if(pppcheckpos == 1) {
        if(pppcheck[0] != '~')
          pppcheckpos = 0;
      } else if(pppcheckpos == 2) {
        //Nothing... 
      } else if(pppcheckpos == 3) {
        if(pppcheck[2] != '}')
          pppcheckpos = 0;
      } else if(pppcheckpos == 4) {
        if(pppcheck[3] != '#')
          pppcheckpos = 0;
        else {
          goto endterm;
        }
      } else 
        pppcheckpos = 0;
    }
    con_update();
  }
  endterm:
  fclose(stream);
  exit(0);
}
