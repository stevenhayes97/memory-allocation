#include <stdio.h>
#include <stdlib.h>
#include "mem.h"
#define NUM_ITER 10
int main(int argc, char *argv[]) {
	printf("Mem Init Result:%d\n",Mem_Init(1233));
	int i=0; 
	void *ptr[NUM_ITER];
	printf("Address of i=>%p\n",&i);
	for (; i<NUM_ITER; i++) {
		ptr[i] = Mem_Alloc(72);
//		printf("Iteration %d:: Mem_Alloc called, return value:%p\n",i, ptr[i]);
//		printf("Free space:%d\n", Mem_Available());
	}
	printf("Free space:%d\n", Mem_Available());
	Mem_Free(ptr[3]);
	Mem_Free(ptr[6]);
	Mem_Free(ptr[7]);
	printf("Free space:%d\n", Mem_Available());
//	int *ptr1 = malloc(sizeof(int));
//	Mem_Free(ptr1);
	
	return 0;
}
