#include <stdio.h>

int main(int argc, char *argv[]) {
	FILE *fp;
	int ch=0;
   	int upto=1; /* the arguement we're upto */
	
	/* Check the number of arguments 
	and if not enough, explain how to
	use and then exit */
	
	if (argc<2) {
		fprintf(stderr,"Usage: cat [FILE ...]\n");
		exit(1);
	}
	
	/* subtract 1, because we don't need to do 
	the command arguement */
	argc--;
	
	/* decrease argc, until it equals zero */
	
   	while(argc--) {
		/* open file for current arguement */
		fp = fopen(argv[upto++],"r");
		
		/* if unable to open, print error and exit */
		if (!fp) {
			perror("cat");
			exit(1);
		}
		
		/* print the whole file to stdout 
		(the screen), one character at a 
		time checking for EOF on input, 
		and errors on output */
		
		while((ch = fgetc(fp)) != EOF)
			if (putchar(ch) == EOF) {
				perror("cat");
				exit(1);
			}
			
		/* we got to the end of the file
		close it and loop back */
	   	fclose(fp);
	}
	/* we're done! */
}
