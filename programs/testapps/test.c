#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <exception.h>

int makeexcept()
{
	throw(128);
}

int callsomething()
{
	int a;
	Try 
	{
		printf("We're here!\n");
		makeexcept();
		printf("Shouldn't be here!\n");
	}
	Catch(a)
	{
		printf("We're handling it!\n");
		throw(a);
	}
}

int main (int argc, char *argv[])
{
	int a;
	Try {
		xmalloc(1);
		printf("Hello!\n");
		callsomething();
		printf("We should never get here\n");
	}
	Catch(a) {
		printf("Caught an exception %d\n", a);
	}
}
