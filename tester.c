#include <stdio.h>
#include <stdlib.h>
#include "mem.h"
int main(int argc, char *argv[]) {
	printf("Mem Init Result:%d\n",Mem_Init(1233));
	int i=0;
	for (; i<10; i++) 
		printf("Iteration %d:: Mem_alloc called, return value:%p\n",i, Mem_Alloc(100));
	return 0;
}
