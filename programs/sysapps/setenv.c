#include <stdio.h>
#include <stdlib.h>

void main(int argc, char *argv[]) {
	if (argc<3) {
		printf("Usage: setenv VAR VALUE\nE.g. setenv PATH /wings/programs:.:/\n");
		exit(1);
	} else {
		setenv(argv[1], argv[2], 1);
	}
}
