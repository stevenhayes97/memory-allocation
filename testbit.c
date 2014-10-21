#include <stdio.h>
#include <stdlib.h>
#include "mem.h"
#define NUM_ITER 255

int main(int argc, char *argv[]) 
{
	printf("Mem Init Result:%d\n",Mem_Init(1));
	printf("INITIAL Free space:%d\n", Mem_Available());

	int i=0; 
	void *ptr[NUM_ITER];
	for (; i<NUM_ITER; i++) 
	{
		ptr[i] = Mem_Alloc(1);
		printf("%d allocated, Free space:%d\n", i+1, Mem_Available());
	}
	//printf("Free space:%d\n", Mem_Available());
	
	for (i=NUM_ITER-1; i>=0; i--) {
		Mem_Free(ptr[i]);
	}
	printf("Free space:%d\n", Mem_Available());
	
	return 0;
}
