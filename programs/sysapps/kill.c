#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void main(int argc, char *argv[]) {
   	int i;
   
	i = atoi(argv[1]);
	printf("Killing %d\n",i);
	kill(i,1);	
   
}
