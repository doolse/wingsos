#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include "asm.h"

void usage() {
	printf("Usage: ar65 [-p] [-o archive.a] FILES...\n"
		"-o     output archive\n"
		"-p     strip preceding path\n"
		"Default archive is arch.a\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	
	char *outfile = "arch.a";
	char *cname;
	int strippath=0;
	int i;
	FILE *fp,*fp2;
	struct stat fs;
	
 	if (argc<2)
		usage();
	while((i = getopt(argc, argv, "o:p")) != EOF) {
		switch(i) {
		case 'o':
			outfile = optarg;
			break;
		case 'p':
			strippath=1;
			break;
		}
	}
	if (optind==argc)
		usage();
	fp = fopen(outfile,"w");
	if (!fp) {
		perror("ar65");
		exit(1);
	}
	while(optind<argc) {
		char *str = argv[optind];
		
		cname = NULL;
		if (strippath == 1)
			cname = strrchr(str,'/');
		if (!cname)
			cname=str;
		else cname++;
		fputc(strlen(cname),fp);
		fputs(cname,fp);
		fputc(0,fp);
		fp2 = fopen(str,"r");
		if (fp2 && fstat(fileno(fp2), &fs) != -1) {
			char *file;
			uint32 size;
			
			file = malloc(fs.st_size);
			size = fread(file, 1, fs.st_size, fp2);
			fwrite(&size, 4, 1, fp);
			fwrite(file, 1, size, fp);
			free(file);
			fclose(fp2);
		} else perror(str);
		optind++;
	}
   	fputc(0,fp);
   	fputc(0,fp);
	fclose(fp);
}
