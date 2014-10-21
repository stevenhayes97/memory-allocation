#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "mem.h"

int m_error;
#define E_BAD_ARGS 1 //FIXME
#define E_MEM_FULL 2 //FIXME
#define E_INV_PTR 3 
#define BLOCK_SIZE 8

typedef enum _State{FREE=0,USED=1,BOUNDARY=9}State;

unsigned char *mapHead;
unsigned char *arenaHead;
int bmSize;//bitmapSize
int memAvailable;
int numBlocksFree;



int numNodesFreeList=0;
int memAvailable=0;

int Mem_Init(int size)	{
	static int initialized=0;
	int pageSize;
	//Ensure that Mem_Init is only called once
	if(initialized == 1) 
	{
		fprintf(stderr,"Mem_Init called more than once\n"); // FIXME remove
		m_error = E_BAD_ARGS;
		return -1;
	}	
	// Ensure that size is correct
	if(size <= 0) 
	{
		fprintf(stderr,"Mem_Init called with incorrect size\n"); // FIXME remove
		m_error = E_BAD_ARGS;
		return -1;
	}
	// Align request to page size
	pageSize= getpagesize();
	
	if(size % pageSize != 0)
	{
		size = size + pageSize - (size % pageSize);
	}

	// open the /dev/zero device
	int fd = open("/dev/zero", O_RDWR);
	// sizeOfRegion (in bytes) needs to be evenly divisible by the page size
	void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (ptr == MAP_FAILED) { perror("mmap"); exit(1); }

	//Set up the bitmap at the start and the arena after that
	mapHead=ptr;//bit map at the start
	bmSize = size/(BLOCK_SIZE + 1); //round down the value from TotalSize=bmSize + bmSize*BLOCK_SIZE //Wasting one byte at the end by allocating only 4095
	arenaHead=ptr+bmSize;
	arenaHead=mapHead;//arena after the bitmap

	//Set all of bitmap to be free
	int i;
	for(i=0; i<bmSize; i++)
		mapHead[i]=FREE;

	numBlocksFree=bmSize;
	memAvailable=bmSize*BLOCK_SIZE;//baisically the arena size

	//  close the device (don't worry, mapping should be unaffected)
	close(fd);

	// Set this bit only if Mem_Init called correctly
	initialized=1;
	return 0;
}



void *Mem_Alloc(int size) {
	//Size can't be zero
	if (size==0)
	{
		m_error=E_BAD_ARGS;
		return NULL;
	}

	//Aligning to 8 byte block
	if (size % BLOCK_SIZE != 0) 
		size = size + BLOCK_SIZE - (size%BLOCK_SIZE);

	int blocksReq=size/BLOCK_SIZE;//number of blocks required in the bitmap

	//THIS PART IS SLOW WHERE WE SEARCH FOR CONTIGUOUS FREE BLOCKS FROM EVERY BIT
	//Currently implemented as first fit
	//Search for blocksReq contigous free blocks
	int scanner=0;
	int available;
	int i;
	while(scanner<=bmSize-blocksReq)//keep scanning until at a distance of less than (required blocks from the end)
	{
		//Count available blocks
		available=0;
		for(i=0;i<blocksReq; i++)
		{
			if(mapHead[i+scanner]!=FREE)
				break;
			available++;
		}

		if(available==blocksReq)//Found sufficient available from this location
		{
			void *ptr=arenaHead+(scanner*BLOCK_SIZE);//get the pointer to the corresponding start in the arena
			//Mark the start as the boundary for the chunk in the bitmap
			mapHead[scanner++]=BOUNDARY;//location++ means index at location and then increment location
			//Mark the rest as USED
			for(i=1; i<blocksReq; i++)
				mapHead[scanner++]=USED;

			numBlocksFree-=blocksReq;
			memAvailable-= blocksReq*BLOCK_SIZE;
			return ptr;
		}
		else//No enough available from this location
			scanner=scanner+available+1; //Skip one ahead of the avaialable because that was the first non-FREE block we encountered
	}

	//Failed to allocate, not enough memory
	m_error=E_MEM_FULL;
	return NULL;	
}


int Mem_Free(void* ptr) {
	if(ptr==NULL)
		return -1;
		
	//see where ptr maps to in the bitmap
	int arenaIndex= (int) ( (unsigned char*)ptr - arenaHead);
	int mapIndex=arenaIndex/BLOCK_SIZE;

	//This should be the boundary block at the start of this chunk
	if(mapHead[mapIndex]!=BOUNDARY && arenaIndex%BLOCK_SIZE!=0)//Invalid ptr
	{
		m_error=E_INV_PTR;
		return -1;
	}

	//mark blocks from here on till the next boundary as free
	mapHead[mapIndex++]=FREE;
	int i=1;
	while(mapIndex < bmSize && mapHead[mapIndex]==USED)//Keep going till the end or the next boundary
	{
		mapHead[mapIndex++]=FREE;
		i++;
	}
	numBlocksFree+=i;
	memAvailable+=i*BLOCK_SIZE;
			
	return 0;
}


int Mem_Available() {
	return memAvailable;
}

void Mem_Dump(){
	//Print out global variables
	printf("==================MEM DUMP=================\n");
	printf("Number of Free Blocks:%d Total free space:%d", numBlocksFree, memAvailable);

	//Not sure what to print out, this just pukes all over the console
	//Print out each bit of the bit map(0 free, 1 used. 9 boundary)
	int i;
	for(i=0;i<bmSize;i++)
		printf("%d",(int)mapHead[i]);
	printf("\n");
}
