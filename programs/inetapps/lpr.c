#include <stdio.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

int globsock;

void getstate(FILE *write, FILE *read, int ping) {
	
	char buffer[1];
	buffer[0]= '\0';

	if((ping) && (fwrite(buffer,1,1,write)<1)){
		printf("Error writing to stream\n");
		exit(0);
		}	
	fflush(write);	
        if(fread(buffer,1,1,read)<1){
	        if(fread(buffer,1,1,read)<1){  //retry 1 (in case of EOF)
			printf("Error reading from stream\n");
			exit(0);
			}
		}
	switch(buffer[0]) {
		case 0:
//			printf("Accepted, proceed\n");
			break;
		case 1:
			printf("Queue not accepting jobs\n");
			fclose(write);
			fclose(read);
			exit(0);
		case 2:
			printf("Queue temporarily full, retry later\n");
			fclose(write);
			fclose(read);
			exit(0);
		case 3:
			printf("Bad job format, do not retry\n");
			fclose(write);
			fclose(read);
			exit(0);
		default:
			printf("Unknown error: %d\n",buffer);
			perror("lpr");
			fclose(write);
			fclose(read);
			exit(0);
		}	
		fflush(stdout);
	}


void main(int argc, char *argv[]) {
	int ch;
	int pipe=0;
        int num= 001;		// Actualy this should be something like the 
				// last 3 numbers of the pid, but how do I 
				// call it???
        FILE *fp;
	FILE *write;
	FILE *read;
	char *server;
        char *rest;
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
		write = fopen(server,"wb");
		free(server);
		if (!write) {
			perror("lpr");
			exit(1);
		}
		read=fdopen(fileno(write),"rb");
		if (!read) {
			perror("lpr");
			exit(1);
		}

//		printf("Sending information:\n");
		write->_flags |= _IOBUFEMP;
		read->_flags |= _IOBUFEMP;

//transfer a printer job
		if (fprintf(write,"%c%s\n",2,printer)<0){	
			printf("Error writing t stream (1)");
			exit(0);
			}
		free(printer);
		getstate(write,read,0);			//OK to proceed?

//sending control file
		if (fprintf(write,"%c%d cfA%03dc64\n",2,35,num)<0){	
					//bytelenght of control file and space
					//sending filename+LF
			printf("Error writing to stream (2)");
			exit(0);
			}
		getstate(write,read,0);			//OK to proceed?



//Control file:
	fprintf(write,"Hc64\n");		//sending hostname
	fprintf(write,"Pjos\n");		//sending User ID
	fprintf(write,"CA\n");			//sending Class info
	fprintf(write,"ldfA%03dc64\n",num);	//sending "print formated page"
	fprintf(write,"UdfA%03dc64\n",num);	//sending "Unlink"
//End
		getstate(write,read,1);		// Ok to proceed??
//		printf("Controlfile sent...\n");	

//sending data file
		fprintf(write,"%c%d dfA%03dc64\n",3,0,num);
					// LPRng with friends: bytelenght 0 =
					// from stdin and sending filename
					//bytelenght of control file and space
		getstate(write,read,0);	// Ok to proceed??
//		printf("Sending file...\n");

	       	ch=0;
		if (argc>2){
			if (strcmp(argv[2],"-") == 0){
				pipe = 1;
//				printf("redirecting from stdin to lpd\n");
				fp = stdin;
				}
			else {
				fp = fopen(argv[2],"r");

				}
			}
		else {
			pipe=1;
			fp=stdin;
			}
                if (!fp) {
                        perror("lpr");
                        exit(1);
                }

                while((ch = fgetc(fp)) != EOF){
                        if (fputc(ch,write) == EOF) {
                                perror("lpr");
				exit(1);
				}
//			putchar(ch);
                        }
		fflush(write);
		if(!pipe){
	                fclose(fp);
			}

//		getstate(write,read,1);	// Ok to proceed??
//		printf("Done sending file...\n");
		
//		For debug only (will not end program in no error occur)
/*
		while(!feof(read)) {
			while ((ch = fgetc(read)) != EOF)
				putchar(ch);
			fflush(stdout);
		}
*/
		fclose(read);
		fclose(write);
		exit(1);
	}
	printf("Usage: %s <[queuename@]servername> [file]\n", argv[0]);	
	printf("       'stdin' used if [file] is '-' or [file] not speified.");
}

