#include <stdio.h>
#include <wgslib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>

extern int chkide64();

int main() {
	int fd;

	chdir("/boot");
	setenv("PATH", "/boot:/wings:/:.", 1);
	setenv("LIBPATH","/boot:.:/wings:/wings/libs:/", 1);
	spawnl(S_WAIT,"./con.drv",NULL);
	close(0);
	close(1);
	close(2);
	open("/dev/con1",O_READ|O_WRITE);
	dup(0);
	dup(0);

   	spawnl(S_WAIT,"./iec.drv",NULL);
/*	spawnl(S_WAIT,"./xiec.drv",NULL);
	close(0);
	close(1);
	close(2);
	open("/dev/xiec",O_READ|O_WRITE);
	dup(0);
	dup(0);*/
	
	if (chkide64()) {
	   printf("IDE64 Detected\n");
	   spawnl(S_WAIT,"./ide.drv",NULL);
	} 
	spawnl(S_WAIT,"./automount",NULL);
	fd = spawnlp(0,"init",NULL);
	if (fd == -1) {
		printf("Error loading init!\n");
		perror("init");
	}
}

