#include <stdio.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <memory.h>

void main(int argc, char *argv[]) {
	struct MemInfo mem;
	
	sendChan(MEMM_CHAN, MMSG_Info, &mem);
	printf("Memory Free: %ld ($%lx)\n",mem.Left,mem.Left);
	printf("Largest Block: %ld ($%lx)\n",mem.Large,mem.Large);
	printf("Kernel Free: %u ($%x)\n",mem.KLeft,mem.KLeft);
	printf("Kernel Largest: %u ($%x)\n",mem.KLarge,mem.KLarge);
	printf("Stack Free: %u ($%x)\n",mem.SLeft,mem.SLeft);
	printf("Stack Largest: %u ($%x)\n",mem.SLarge,mem.SLarge);
}
