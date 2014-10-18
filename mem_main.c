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
	fprintf(stdout,"Get page size = %d\n",pageSize); // FIXME remove
        if(size % pageSize != 0){
		size = size + pageSize - (size % pageSize);
	}
	fprintf(stdout,"Asking for size = %d\n",size); // FIXME remove

	// open the /dev/zero device
	int fd = open("/dev/zero", O_RDWR);
	// sizeOfRegion (in bytes) needs to be evenly divisible by the page size
	void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (ptr == MAP_FAILED) { perror("mmap"); exit(1); }

	head = (node_t *) ptr;
	head->size = size-sizeof(node_t);
	head->next = head; //FIXME
	scanner = head;
	numNodesFreeList++;
	//  close the device (don't worry, mapping should be unaffected)
	close(fd);

	// Set this bit only if Mem_Init called correctly
	fprintf(stdout,"Node size = %lu\n",sizeof(node_t)); // FIXME remove
	fprintf(stdout,"Header size = %lu\n",sizeof(header_t)); // FIXME remove

	initialized=1;
	return 0;
}

void *Mem_Alloc(int size) {
	//ALigning to 8 byte block
	size = size + BLOCK_SIZE -(size % BLOCK_SIZE);
	//Actual size needed is size of region + header
	int sizeAlloc = size + sizeof(header_t); // Optimize inline?
	fprintf(stdout,"Size original = %d | with Header:%d\n", size, sizeAlloc); // FIXME remove
	int itr=0;
	node_t *prev = scanner;
	printf("BEFORE WHILE, scanner values: size%d\n",scanner->size);
 	while(scanner->size < sizeAlloc) {
		// Take care of the scanning logic
		itr++;
		prev = scanner;
		scanner = scanner->next;
		if(itr == numNodesFreeList) {
				fprintf(stderr,"Mem_Alloc couldn't allocate of size:%d\n",size); // FIXME remove
				m_error = E_MEM_FULL;
				return NULL;
		}
	}
	printf("AFTER WHILE, scanner values: size%d\n",scanner->size);
	if(scanner->size > sizeAlloc) {//If older region fragmented
		node_t *new = scanner+sizeAlloc;
		new->size = scanner->size - sizeAlloc;
		new->next = scanner->next;
		//Update the previous pointer
		prev->next = new;
		if(scanner == head) 
			head = new;
	}
	else		// Entire region used up, so no addition to free list
		prev->next = scanner->next;

	header_t *ptr = (void *)scanner;
	ptr->size=size;
	ptr->magic=12345678;
	ptr++; // Getting the address after the header
		
	// Make sure scanner is moved on
	scanner = prev->next;

	Mem_Dump(); //FIXME
	

	return (void *)ptr;	
}

void Mem_Dump(){
	node_t *node = head;
	//Print out global variables
	printf("Size of Free List:%d\n", numNodesFreeList);

	//Print out the free list
	int i=0;
	for(;i<numNodesFreeList;i++) {
		printf("Size:%d %p\n", node->size, node);
		node = node->next;
	}
}
int main(int argc, char *argv[]) {
	printf("Mem Init Result:%d\n",Mem_Init(1233));
	int i=0;
	for (; i<10; i++) 
		printf("Iteration %d:: Mem_alloc called, return value:%p\n",i, Mem_Alloc(100));
	return 0;
}
