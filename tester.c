#include <stdio.h>
#include <stdlib.h>
#include "mem.h"
#define NUM_ITER 51
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
	for (i=NUM_ITER-1; i>=0; i--) {
		Mem_Free(ptr[i]);
	}
	printf("Free space:%d\n", Mem_Available());
//	int *ptr1 = malloc(sizeof(int));
//	Mem_Free(ptr1);
	
	return 0;
}
