#include <stdio.h>
#include <stdlib.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>

void main(int argc, char *argv[]) {
	int ch;
	int tall = 2;
	FILE *stream;
	char *server;
	char *printer;
        char *rest;


       if (argc>2) {
        	rest = strchr(argv[1],'@');
        	server=malloc(strlen(argv[1])+strlen("/dev/tcp/:515")+1);
        	strcpy(server,"/dev/tcp/");
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
			perror("lprm");
			exit(1);
		}
		stream->_flags |= _IOBUFEMP;

		fprintf(stream,"%c%s lp",5,printer);
		free(printer);

		argc--;
		argc--;
		while (argc--){
  			fprintf(stream," %s",argv[tall++]);
			}
		fprintf(stream,"\n");
		fflush(stream);

		while(!feof(stream)) {
			while ((ch = fgetc(stream)) != EOF)
				putchar(ch);
			fflush(stdout);
		}
		exit(1);
	}
	printf("Usage: %s <[queuename@]servername> <user> [id]\n", argv[0]);
}

