#include <stdio.h>
#include <stdlib.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>

int globsock;

void main(int argc, char *argv[]) {
	int ch;
	int mode = 3;
	FILE *stream;
	char *rest;
	char *server;
	char *printer;

	if (argc>1) {
		rest = strchr(argv[1],'@');
		server=malloc(strlen(argv[1])+strlen("/tcp/:515")+1);
		strcpy(server,"/tcp/");
		if (rest !=NULL){
			strcat(server,rest+1);
			printer=malloc(strlen(argv[1]) - strlen(rest)+1);
			strncpy(printer,argv[1],strlen(argv[1]) - strlen(rest));
			printer[strlen(argv[1]) - strlen(rest)]='\0';
			}
		else{
			strcat(server,argv[1]);
			printer=malloc(3);
			strcpy(printer,"lp");
			}
		strcat(server,":515");
		stream = fopen(server,"r+b");
		free(server);
		if (!stream) {
			perror("lpq");
			exit(1);
		}
		if(argc>2){
			if((strcmp(argv[2],"l") == 0)){
				mode=4;
				}
			if((strcmp(argv[2],"lpc") == 0)){
				mode=9;
				}
			}
//		printf("Printer queue information:\n");
		stream->_flags |= _IOBUFEMP;

//		fputc(3,stream); //short format
//		fputc(4,stream); //long format
//		fputc(9,stream); //LPRng format
		fprintf(stream,"%c%s\n",mode,printer);
		free(printer);
//		fputc(10,stream);
		fflush(stream);

		while(!feof(stream) && !ferror(stream)) {
			while ((ch = fgetc(stream)) != EOF)
				putchar(ch);
			fflush(stdout);
		}
//		printf("Connection closed by remote host!\n");
		exit(1);
	}
	printf("Usage: %s <[queuename@]servername> [l|lpc]\n", argv[0]);	
}

