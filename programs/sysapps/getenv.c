#include <stdio.h>
#include <stdlib.h>

void main(int argc, char *argv[]) {
	if (argc<2) {
		printf("Usage: getenv VAR\nE.g. getenv PATH\n");
		exit(1);
	} else {
		printf("%s=%s\n", argv[1], getenv(argv[1]));
	}
}
