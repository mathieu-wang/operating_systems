#include <stdio.h>      
#include <stdlib.h>  
#include <unistd.h>

#define FIRST_FIT 0
#define BEST_FIT 1
#define ONE_KB 1024
#define MIN_REQUEST_SIZE 128*ONE_KB //Request 128 KB at a time


void *my_malloc(int size);
void my_free(void *ptr);
void my_mallopt(int policy);
void my_mallinfo();

//global variables:
extern char *my_malloc_error;
int current_policy = FIRST_FIT;

//Define a free_list_block struct and name head as the first node of the doubly linked list of free_list_blocks
typedef struct free_list_block {
	int length;
	struct block *prev;
	struct block *next;
	char *data_ptr;
} block;

block *head;

void *my_malloc(int size) {
	//if memory could not be allocated, return NULL and sets my_malloc_error

	if (head == NULL) {
		printf("%s\n", "Head is NULL, need to request memory.\n");

		head = &(block){0, NULL, NULL, NULL};
		head -> length = MIN_REQUEST_SIZE;
		head -> data_ptr = head + 1; //start at next byte

		//brk(head + (sizeof(block) + MIN_REQUEST_SIZE)); ?
		sbrk(sizeof(block) + MIN_REQUEST_SIZE);
	}
	if (head == NULL) {
		printf("%s\n", "Init head failed\n");
	}



}

void my_free(void *ptr) {
	//deallocates block of memory pointed by ptr
	//ptr should be an address previously allocated by the Memory Allocation Package
	//if ptr is NULL, don't do anything
	//does not lower program break all the time: simply adds to list of free blocks
	//lower program break when top free block is larger than 128KB
}

void my_mallopt(int policy) {
	current_policy = policy;
}

void my_mallinfo() {
	// should print:
	//-total number of bytes allocated
	//-total free space
	//-largest contiguous free space
	//-other info

}

int main(int argc, char *argv[]) {
	printf("%ld\n", (long)sbrk(0));
	my_malloc(2);
	printf("Size of block: %d\n", sizeof(block));
	printf("Size of block*: %d\n", sizeof(block*));
	printf("Size of int: %d\n", sizeof(int));
	printf("Size of char: %d\n", sizeof(char));
	printf("Size of char*: %d\n", sizeof(char*));
	printf("Address of head:%ld\n", head);
	printf("%ld\n", (long)sbrk(0));
	printf("%ld\n", (long)sbrk(0));

	return 0;
}
