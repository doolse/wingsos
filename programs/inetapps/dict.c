#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wgslib.h>

void showhelp() {
	fprintf(stderr, "Usage: dict word\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	int ch;
	FILE *fp,*fp2;
	int pipe1[2];
	int pipe2[2];
	
	while ((ch = getopt(argc, argv, "h")) != EOF) {
		switch (ch) {
		case 'h':
			showhelp();
		}
	}
	if (optind>=argc)
		showhelp();
	fp = fopen("/dev/tcp/dictionary.reference.com:80","r+");
	if (!fp) {
		perror("dict");
		exit(1);
	}
	fprintf(fp, "GET /search?q=%s HTTP/1.0\n\n", argv[optind]);
	fflush(fp);
	pipe(pipe1);
	pipe(pipe2);
	redir(pipe1[0], STDIN_FILENO);
//	redir(pipe2[1], STDOUT_FILENO);
	spawnlp(0, "web", NULL);
	close(pipe1[0]);
	close(pipe2[1]);
//	resetredir();
//	redir(pipe2[0], STDIN_FILENO);
//	spawnlp(0, "more", NULL);
//	close(pipe2[0]);
	fp2 = fdopen(pipe1[1], "w");
	while ((ch = fgetc(fp)) != EOF) {
//		printf("Putting\n");
		fputc(ch, fp2);
//		printf("Getting\n");
	}
}
