#include <stdio.h>
#include <fcntl.h>
#include <wgsipc.h>
#include <stdlib.h>
#include <sys/stat.h>

void die() {
	perror("rawplay");
	exit(1);
}

int main(int argc, char *argv[]) {
	int digiChan;
	int stereo=0;
	int bits=8;
	unsigned int hz=11000;
	FILE *fp;
	int ch,j;
	char *buf;
	long left,i;
	long done=0;
	struct stat stbuf;
	
	while ((ch = getopt(argc, argv, "sh:b:")) != EOF) {
		switch(ch) {
		case 's': 
			stereo = 1;
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'h':
			hz = atoi(optarg);
			break;
		}
	}
	if (optind>=argc) {
		fprintf(stderr,"Usage: rawplay [-h hertz] [-s] [-b bits] wavfile\n");
		exit(1);
	}
	digiChan = open("/dev/mixer",O_READ|O_WRITE);
	if (digiChan == -1) {
		fprintf(stderr,"Digi device not loaded\n");
		exit(1);
	}
	ch = stat(argv[optind], &stbuf);
	if (ch == -1)
		die();
	fp = fopen(argv[optind], "rb");
	if (fp) {
		printf("Sample rate: %u, %d bits, %s\n", hz, bits, stereo ? "stereo" : "mono");
		left = stbuf.st_size;
		printf("Sample size %ld\n", left);
		if (bits == 4)
			buf = xmalloc(left*2);
		else buf = xmalloc(left);
		i=0;
		j=0;
		while (i<left) {
			ch = fgetc(fp);
			if (ch == -1)
				break;
			switch(bits) {
				case 4: 
					*(buf+done) = ch & 0xf0;
					done++;
					ch <<= 4;
				default:
					*(buf+done) = ch;
					done++;
					break;
			}
			i++;
			j++;
			if (j >= 4096) {
				j=0;
				printf("%ld bytes done\r",i);
				fflush(stdout);
			}
		}
		sendCon(digiChan, IO_CONTROL, 0xc0, 8, (unsigned int) hz, 1, 2);
		while (done) {
			int amount;
			
			if (done > 32767)
				amount = 32767;
			else amount = done;
			write(digiChan, buf, amount);
			buf += amount;
			done -= amount;
		}
	} else die();
}
