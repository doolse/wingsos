#include <stdio.h>

int main(int argc, char *argv[]) {
   	int upto=1;
	
	argc--;
	upto=1;
	while(argc--) {
		printf("%s",argv[upto++]);
		if (argc)
			putchar(' ');
	}
	putchar('\n');
}
