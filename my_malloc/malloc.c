#include <stdio.h>      
#include <stdlib.h>  
#include <unistd.h>

#define FIRST_FIT 0
#define BEST_FIT 1

void *my_malloc(int size);
void my_free(void *ptr);
void my_mallopt(int policy);
void my_mallinfo();

//global variables:
extern char *my_malloc_error;
int current_policy = FIRST_FIT;

void *my_malloc(int size) {
	//if memory could not be allocated, return NULL and sets my_malloc_error
}

void my_free(void *ptr) {
	//deallocates block of memory pointed by ptr
	//ptr should be an address previously allocated by the Memory Allocation Package
	//if ptr is NULL, don't do anything
	//does not lower program break all the time: simply adds to list of free blocks
	//lower program break when top free block is larger than 128KB
}

int main(int argc, char *argv[]) {
	return 0;
}