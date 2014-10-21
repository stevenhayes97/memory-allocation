#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <limits.h>
#include "mem.h"

int m_error;
#define E_BAD_ARGS 1 //FIXME
#define E_MEM_FULL 2 //FIXME
#define E_INV_PTR 3 
#define BLOCK_SIZE 16

//Assuming char is 8 bits, which i hope to God it is or i'm screwed

typedef enum _State{FREE=0,USED=1,BOUNDARY=9}State;

unsigned char *mapHead;
unsigned char *arenaHead;
int numBits;//bitmap size in actual number of bits
int bmSize;//bitmapSize in number of bytes rounded up
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
	
	bmSize= (size/BLOCK_SIZE)/CHAR_BIT;//over compensate
	numBits=(size-bmSize)/BLOCK_SIZE; //Actual number of bits required

	arenaHead=ptr+bmSize;//arena after the bitmap

	//Set all of bitmap to be free by zeroing them
	int i;
	for(i=0; i<bmSize; i++)
			mapHead[i]&=0;//Already optimized to zero the array byte by byte

	numBlocksFree=numBits;
	memAvailable=numBits*BLOCK_SIZE;//baisically the arena size

	//  close the device (don't worry, mapping should be unaffected)
	close(fd);

	// Set this bit only if Mem_Init called correctly
	initialized=1;
	return 0;
}


//This is sort of generic to handle requests for variable sized inputs. Doesn't mark a boundary which causes problems in free
void *Mem_Alloc(int size) {
	//Size can't be zero
	if (size==0)
	{
		m_error=E_BAD_ARGS;
		return NULL;
	}

	//Aligning to 16 byte block
	if (size % BLOCK_SIZE != 0) 
		size = size + BLOCK_SIZE - (size%BLOCK_SIZE);

	int blocksReq=size/BLOCK_SIZE;//number of blocks/bits required in the bitmap

	//This is where we search for contiguous free BLOCK_SIZES or bits to satisfy the blocksReq demand (not required for simple 16byte 1bit case)
	//Currently implemented as first fit
	int scanner=0;
	int available;
	int i, bit;
	while(scanner<=numBits-blocksReq)//keep scanning until at a distance of less than (required blocks/bits from the end)
	{
		//Count available blocks/bits
		available=0;
		for(i=0;i<blocksReq; i++)
		{
			bit = mapHead[ (i+scanner)/CHAR_BIT ] >> ((i+scanner)%CHAR_BIT) & 1;//mask to get each bit out
			if(bit!=FREE)
				break;
			available++;
		}

		if(available==blocksReq)//Found required number of bits from this location onward
		{
			void *ptr=arenaHead+(scanner*BLOCK_SIZE);//get the pointer to the corresponding start in the arena
			//Mark the chunk as USED=1
			for(i=0; i<blocksReq; i++)
				mapHead[(scanner+i)/CHAR_BIT]|= USED << ((scanner+i)%CHAR_BIT);//OPTIMIZE LATER: Instead of bit by bit setting just set a whole byte in one step  

			numBlocksFree-=blocksReq;
			memAvailable-= blocksReq*BLOCK_SIZE;
			return ptr;
		}
		else//Not enough available from this location,
			scanner=scanner+available+1; //OPTIMIZE LATER: Can just jump 16bits ahead instead of moving bit by bit
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

	//There should be a boundary block at the start of this chunk or it should be non-Free atleast in this case
	int boundarybit=mapHead[mapIndex/CHAR_BIT] >> (mapIndex%CHAR_BIT) & 1;//get the bit corresponding to this index
	if(boundarybit==FREE || arenaIndex%BLOCK_SIZE!=0)//Invalid ptr
	{
		m_error=E_INV_PTR;
		return -1;
	}

	//This will be tricky later on for a variable sized malloc (i.e more than 16bytes)
	//For now just clear 16bytes/1bit and free one block
	mapHead[mapIndex/CHAR_BIT] &= ~(1 << mapIndex%CHAR_BIT); 
	numBlocksFree+=1;
	memAvailable+=1*BLOCK_SIZE;
			
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
	int i,bit;
	for(i=0;i<numBits;i++)
	{
		bit=mapHead[i/CHAR_BIT] >> (i%CHAR_BIT) & 1;
		printf("%d", bit);
	}
	printf("\n");
}
