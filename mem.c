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
typedef struct __header_t {
	int size;
	int magic;
} header_t;

typedef struct __node_t {
	int size;
	struct __node_t *next;
} node_t;

node_t *head;
node_t *scanner;
int numNodesFreeList=0;
int memAvailable=0;

int Mem_Init(int size)	{
	static int initialized=0;
	int pageSize;
	//Ensure that Mem_Init is only called once
	if(initialized == 1) {
		fprintf(stderr,"Mem_Init called more than once\n"); // FIXME remove
		m_error = E_BAD_ARGS;
		return -1;
	}	
	// Ensure that size is correct
	if(size <= 0) {
		fprintf(stderr,"Mem_Init called with incorrect size\n"); // FIXME remove
		m_error = E_BAD_ARGS;
		return -1;
	}
	// Align request to page size
	pageSize= getpagesize();
//	fprintf(stdout,"Get page size = %d\n",pageSize); // FIXME remove
        if(size % pageSize != 0){
		size = size + pageSize - (size % pageSize);
	}
//	fprintf(stdout,"Asking for size = %d\n",size); // FIXME remove

	// open the /dev/zero device
	int fd = open("/dev/zero", O_RDWR);
	// sizeOfRegion (in bytes) needs to be evenly divisible by the page size
	void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (ptr == MAP_FAILED) { perror("mmap"); exit(1); }

	head = (node_t *) ptr;
	head->size = size-sizeof(node_t);
	head->next = NULL; //FIXME
	scanner = head;
	numNodesFreeList++;
	memAvailable = head->size;
	//  close the device (don't worry, mapping should be unaffected)
	close(fd);

	// Set this bit only if Mem_Init called correctly
	//	fprintf(stdout,"Header size = %lu\n",sizeof(header_t)); // FIXME remove

	initialized=1;
	return 0;
}

void *Mem_Alloc(int size) {
	//ALigning to 8 byte block
	if (size % BLOCK_SIZE != 0) {
		size = size + BLOCK_SIZE - (size%BLOCK_SIZE);
	}
	//Actual size needed is size of region + header
	int sizeAlloc = size + sizeof(header_t); // Optimize inline?
	scanner = head;
	node_t *prev = scanner;
 	while(scanner->size + sizeof(node_t) < sizeAlloc) { //FIXME shouldn't scanner->size plus size of node_t be considered
		// Take care of the scanning logic
		prev = scanner;
		if(scanner->next != NULL) {
			scanner = scanner->next;
			printf("SCANNER->SIZE=%d\n",scanner->size);
		}
		else {
			fprintf(stderr,"Mem_Alloc couldn't allocate of size:%d\n",size); // FIXME remove
			m_error = E_MEM_FULL;
			return NULL;
		}
	}
//	printf("AFTER WHILE, scanner values: size%d\n",scanner->size);
	if(scanner->size + sizeof(node_t) > sizeAlloc) {//If older region fragmented
		node_t *new = (node_t *)((char *)scanner + sizeAlloc);
		new->size = scanner->size - sizeAlloc;
		new->next = scanner->next;
		//Update the previous pointer
		if(prev != scanner) {
			prev->next = new;
		}
		if(scanner == head) {
			head = new;
		}
		memAvailable -= sizeAlloc;
	}
	else {	// Entire region used up, so no addition to free list, unless this is the last chunk of memory
		if((void *)scanner != (void *)head) {
			if(prev != scanner) {
				prev->next = scanner->next;
			}
			memAvailable -= sizeAlloc -sizeof(node_t);
			numNodesFreeList--;
		}
		else if (numNodesFreeList>1) {
			head=head->next;
			numNodesFreeList--;
		}
		else {	// Header is the only thing left
			head = (node_t *)((char *)scanner + sizeAlloc);
			head->next = NULL;
			head->size = 0;
			memAvailable = 0;
// FIXME			numNodesFreeList--; 
		}
	}

	header_t *ptr = (void *)scanner;
	ptr->size=size;
	ptr->magic=12345678;
	ptr=ptr+1; // Getting the address after the header
		
	// Make sure scanner is moved on
//	scanner = prev->next;

//	Mem_Dump(); //FIXME
	
//	printf("PTR RETURN HERE=====>%p\n", (void *)ptr);
	return (void *)ptr;	
}

int Mem_Free(void* ptr) {
	if(ptr == NULL) {
		return -1;	
	} 	
	header_t *block = ((header_t *)ptr - 1);
	// Ensure that this is a valid ptr sent by us
	if (block->magic != 12345678) {
		fprintf(stderr,"Invalid Pointer passed to Mem_Free\n"); // FIXME remove
		m_error = E_INV_PTR;
		return -1;
	}
	node_t *node = head;
	if ((char *)head > (char *)block) {
		node_t *new = (void *) block;
		new->next = head;
		new->size = block->size + sizeof(header_t) - sizeof(node_t); // FIXME INLINE ALL SIZEOFs?  this can be 0 if block size is 8!!!
		memAvailable += new->size;
		head = new;
		numNodesFreeList++;
		if((char *)new + new->size + sizeof(node_t) == (char *)new->next) {
//			printf("------COALESCING ON HEAD------------\n");
			new->size += new->next->size + sizeof(node_t);
			new->next = new->next->next;
			numNodesFreeList--;
			memAvailable += sizeof(node_t);
		} 
	}
	else {
		while ((char *)block > (char *)node->next && node->next != NULL) { //FIXME IS THERE ANY CASE MISSING HERE?
			node = node->next;
		}
		node_t *new = (void *) block;
		new->next = node->next;
		node->next = new;
		new->size = block->size + sizeof(header_t) - sizeof(node_t); // FIXME INLINE ALL SIZEOFs?
		memAvailable += new->size;
		numNodesFreeList++;
		//Lets Coalesce!
		if((char *)node + node->size + sizeof(node_t) == (char *)new) {
//			printf("------COALESCING ON LEFT------------\n");
			node->size += new->size + sizeof(node_t);
			node->next = new->next;
			numNodesFreeList--;
			new = node; // FIXME IS THIS CORRECT? DONE TO ENSURE COALESCING IN BOTH CASES ON RIGH SIDE
			memAvailable += sizeof(node_t);
		}
		if((char *)new + new->size + sizeof(node_t) == (char *)new->next) {
//			printf("------COALESCING ON RIGHT------------\n");
			new->size += new->next->size + sizeof(node_t);
			new->next = new->next->next;
			numNodesFreeList--;
			memAvailable += sizeof(node_t);
		}

	}
//	Mem_Dump();
	return 0;
}


int Mem_Available() {
	return memAvailable;
}

void Mem_Dump(){
	node_t *node = head;
	//Print out global variables
	printf("==================MEM DUMP=================\n");
	printf("Size of Free List:%d\n", numNodesFreeList);

	//Print out the free list
	int i=0;
	for(;i<numNodesFreeList;i++) {
		printf("Size:%d Addr=%p Next=%p\n", node->size, (void *) node, node->next);
		node = node->next;
	}
}
