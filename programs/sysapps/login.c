#include <stdio.h>
#include <wgslib.h>
#include <unistd.h>
#include <string.h>

#define MAXLINE 160

char buf[MAXLINE];

int main(int argc, char *argv[]) {
	int allowed = 5;
	char *password="sesame";
	char *temp;
	FILE *fp;
	
        chdir(getappdir());
	fp = fopen("passwd", "r");
	if (fp) {
		int num = fread(buf, 1, MAXLINE-1, fp);
		buf[num] = 0;
		temp = strchr(buf, '\n');
		if (temp)
			*temp = 0;
		password = strdup(buf);
		fclose(fp);
	}
	while(allowed--) {
		temp = getpass("Password:");
		if (!strcmp(password, temp)) {
			spawnlp(S_LEADER,"sh",NULL);
			allowed = 0;
		}
	}
}
