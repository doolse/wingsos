#include <stdio.h>
#include <wgslib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

char inbuf[64];

int mkdirs(char *str)
{
	int ch;
	char *str2 = str+1;
	while (ch = *str2)
	{
		if (ch == '/')
		{
			*str2 = 0;
			if (mkdir(str, 0777) && errno != EEXIST)
				return 1;
			*str2 = '/';
		}
		str2++;
	}
	if (mkdir(str, 0777) && errno != EEXIST)
		return 1;
	return 0;
}

int main() {
	char *str2;
	char *str = queryname("/", 0);
	while (str[0])
	{
		str++;
		printf("Hello %s\n", str);
		str += strlen(str)+1;
	}
	do
	{
		printf("Where would you like to install wings?\nE.g. /wings, or /hd8/wings/ etc..\n");
		fgets(inbuf+1, 64, stdin);
	} while (inbuf[1] == '\n');
	if (inbuf[1] == '/')
	{
		inbuf[0] = '/';
		str = inbuf;
	} 
	else
	{
		str = inbuf+1;
	}
	str2 = strchr(inbuf, '\n');
	if (str2)
		str2[0] = 0;
	if (mkdirs(str))
	{
		fprintf(stderr, "Couldn't create dirs\n");
		exit(1);
	}
	chdir(str);
	setenv("PATH", "/boot:/:/system:/wings/system:.", 1);
	spawnlp(S_WAIT, "gunzip", "/wings.zip", NULL);
}

