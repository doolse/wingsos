#include <stdio.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <net.h>
#include <string.h>

int globsock;

void fromServer(int *tlc) {
	FILE *stream;
	int ch;
	
	stream = fdopen(*tlc,"wb");
	
	stdin->_flags |= _IOBUFEMP;
	while (stream) {
		while ((ch = getchar()) != EOF)
			fputc(ch,stream);
		fflush(stream);
	}
}

int main(int argc, char *argv[]) {
	int ch;
	FILE *stream;

	if (argc>1) {
		stream = fopen(argv[1],"rb");
		if (!stream) {
			perror("connect");
			exit(1);
		}
		printf("Connected\n");
		globsock = fileno(stream);
		newThread(fromServer,256,&globsock);
		stream->_flags |= _IOBUFEMP;
		while(!feof(stream)) {
			while ((ch = fgetc(stream)) != EOF)
				putchar(ch);
			fflush(stdout);
		}
		printf("Connection closed by remote host!\n");
		exit(1);
	}
	
}

