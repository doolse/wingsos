#include <stdio.h>

int main(int argc, char *argv[]) {
	int ch;
	int count=0;
	int inword=0;
	int whitespace;
	FILE *stream = stdin;
	if (argc>1)
		stream = fopen(argv[1],"r");
	if (!stream) {
		perror("wc");
		exit(1);
	}
	while ((ch = fgetc(stream)) != EOF) {
		whitespace = (ch==' ' || ch=='\n' || ch=='\0' || ch=='\t');
		if (inword && whitespace)
			inword--;
		else if (!inword && !whitespace) {
			inword++;
			count++;
		}
	}
	printf("%d\n",count);
}
