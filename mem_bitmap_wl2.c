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
#define BITS_PER_BLOCK 3
//Assuming char is 8 bits, which i hope to God it is or i'm screwed

typedef enum _State{FREE=0,SIZE16=1,SIZE80=2, size256=4, BOUNDARY=9}State;

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
	
	bmSize= (size/BLOCK_SIZE)*BITS_PER_BLOCK/CHAR_BIT;//over compensate
	numBits=(size-bmSize)*BITS_PER_BLOCK/BLOCK_SIZE; //Actual number of bits required

	arenaHead=ptr+bmSize;//arena after the bitmap FIXME use numBits?

	//Set all of bitmap to be free by zeroing them
	int i;
	for(i=0; i<bmSize; i++)
			mapHead[i]&=0;//Already optimized to zero the array byte by byte

	numBlocksFree=numBits;
	memAvailable=numBits*BLOCK_SIZE/BITS_PER_BLOCK;//baisically the arena size

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

	//Aligning to 16 byte block NOT NEEDED FOR WL2
	if (size % BLOCK_SIZE != 0) 
		size = size + BLOCK_SIZE - (size%BLOCK_SIZE);

	int blocksReq=size/BLOCK_SIZE;//number of blocks/bits required in the bitmap

	//This is where we search for contiguous free BLOCK_SIZES or bits to satisfy the blocksReq demand (not required for simple 16byte 1bit case)
	//Currently implemented as first fit
	int scanner=0;
	int available,taken;
	int i, bit;
	while(scanner<=numBits-blocksReq*BITS_PER_BLOCK)//keep scanning until at a distance of less than (required blocks/bits from the end)
	{
		//Count available blocks/bits
		available=0;	//FIXME available and i are the same -1
		taken=0;
		//OPTIMIZE SCANNING
//		printf("=======BEGIN SCANNING=======");
		for(i=0;i<blocksReq*BITS_PER_BLOCK; i=i+BITS_PER_BLOCK)
		{
			bit = mapHead[ (i+scanner)/CHAR_BIT ] >> ((i+scanner)%CHAR_BIT) & 1;//mask to get each bit out
			if(bit!=FREE){
//				printf("NOT FREE scanner+i=%d bit=%d\n",scanner+i, bit);
				break;
			}
			available++;
//			printf("Scanner+i:%d available:%d\n",scanner+i, available);
		}
		//Find out how many blocks consumed
//		printf("===========>Scanner+i:%d available:%d\n",scanner+i, available);
				
		if(available==blocksReq)//Found required number of bits from this location onward
		{
			void *ptr=arenaHead+(scanner*BLOCK_SIZE);//get the pointer to the corresponding start in the arena
			//Mark the chunk as SIZE16=3'b100 SIZE80=3'b101 SIZE256=3'b110
//			for(i=0; i<blocksReq; i++)
			mapHead[(scanner)/CHAR_BIT]|= SIZE16 << ((scanner)%CHAR_BIT);
//			printf("ALOCATED AT HEAD->scanner=%d\n",scanner);
			switch (size) {
				case 16:	// NOTHING TO DO AS 3'b100 
					break;
				case 80: mapHead[(scanner+2)/CHAR_BIT]|= SIZE16 << ((scanner+2)%CHAR_BIT);
					break;
				case 256:mapHead[(scanner+1)/CHAR_BIT]|= SIZE16 << ((scanner+1)%CHAR_BIT);
					break;
			}
					  
			//FIXME should this be changed?
			numBlocksFree-=blocksReq;
			memAvailable-= blocksReq*BLOCK_SIZE;
			return ptr;
		}
		else {//Not enough available from this location,
			int bit1,bit2;
			bit1 = mapHead[ (i+scanner+1)/CHAR_BIT ] >> ((i+scanner+1)%CHAR_BIT) & 1;//mask to get each bit out
			bit2 = mapHead[ (i+scanner+2)/CHAR_BIT ] >> ((i+scanner+2)%CHAR_BIT) & 1;//mask to get each bit out
			taken = (bit1==1)? 16*BITS_PER_BLOCK : ((bit2==1)?5*BITS_PER_BLOCK:1*BITS_PER_BLOCK);	
			scanner=scanner+available*BITS_PER_BLOCK+taken; 
//			printf("NOT FREE DEBUG INFO===>bit1=%d bit2=%d taken=%d\n",bit1, bit2,taken);
		}
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
	int bit1, bit2, freed;
	bit1 = mapHead[ (mapIndex+1)/CHAR_BIT ] >> ((mapIndex+1)%CHAR_BIT) & 1;//mask to get each bit out
	bit2 = mapHead[ (mapIndex+2)/CHAR_BIT ] >> ((mapIndex+2)%CHAR_BIT) & 1;//mask to get each bit out
	freed = (bit1==1)? 16: ((bit2==1)?5:1);	
	numBlocksFree+=freed;
	memAvailable+=freed*BLOCK_SIZE;
//	printf("FREE DEBUG INFO===>bit1=%d bit2=%d freed=%d\n",bit1, bit2,freed);
			
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
	//Print out each bit of the bit map(0 free, 1 SIZE16. 9 boundary)
	int i,bit;
	for(i=0;i<numBits;i++)
	{
		bit=mapHead[i/CHAR_BIT] >> (i%CHAR_BIT) & 1;
		printf("%d", bit);
	}
	printf("\n");
}
