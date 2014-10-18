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

typedef struct __header_t {
	int size;
	int magic;
} header_t;

typedef struct __node_t {
	int size;
	struct __node_t *next;
} node_t;

node_t *head;


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
	head->next = NULL;
	
	//  close the device (don't worry, mapping should be unaffected)
	close(fd);

	// Set this bit only if Mem_Init called correctly
	initialized=1;
	return 0;
}



