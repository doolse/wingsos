#include <stdio.h>
#include <wgslib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>

int main() {
	chdir("/dr0");
	mkdir("wings", 0777);
	chdir("wings");
	setenv("PATH", "/boot:/:/system:/wings/system:.", 1);
//	spawnlp(S_WAIT, "gunzip", "/wings.zip", NULL);
}

