#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wgslib.h>

int main(int argc, char *argv[]) {
	int ch;
	FILE *fp,*fp2;
	int pipe1[2];
	
        if(argc < 2) {
          printf("USAGE: thes word\n");
          exit(-1);
        }

	fp = fopen("/dev/tcp/thesaurus.reference.com:80","r+");
	if (!fp) {
          printf("Could Not Connect To Server\n");
	  exit(1);
	}
        fprintf(fp, "GET /search?q=%s HTTP/1.0\n\n", argv[1]); 
	fflush(fp);

	pipe(pipe1);

	redir(pipe1[0], STDIN_FILENO);
	spawnlp(0, "web", NULL);
	close(pipe1[0]);

	fp2 = fdopen(pipe1[1], "w");
	while ((ch = fgetc(fp)) != EOF) {
//		printf("Putting\n");
		fputc(ch, fp2);
//		printf("Getting\n");
	}
return(0);

}
