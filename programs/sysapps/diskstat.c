#include <stdio.h>
#include <wgslib.h>

void main(int argc, char *argv[]) {
	struct DiskStat ds;
	
	if (argc>1)
	{
		diskstat(argv[1], &ds);
		printf("Blocks in cache:%d\n", ds.CacheSz);
		printf("Cached in use:%d\n", ds.InUse);
		printf("Unsynced blocks in cache:%d\n", ds.Changed);
	} else printf("Usage: diskstat /filesystem\n");
}
