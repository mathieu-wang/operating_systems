#include <stdio.h>      
#include <stdlib.h>  
#include <unistd.h>

#define FIRST_FIT 0
#define BEST_FIT 1
#define ONE_KB 1024


void *my_malloc(int size);
void my_free(void *ptr);
void my_mallopt(int policy);
void my_mallinfo();

//global variables:
extern char *my_malloc_error;
const int min_req_size = 128*ONE_KB;
int current_policy = FIRST_FIT;

int total_allocated;
int total_free;
int largest_contiguous_free;

//Define a block struct and name head as the first node of the doubly linked list of free_list_blocks
typedef struct block {
    int length;
    struct block *prev;
    struct block *next;
} block;

block *head;

void *find_free(int size) {
    if (head == NULL) return NULL;

    block *current = head;

    if (current_policy == FIRST_FIT) {
        while (current != NULL) {
            if (current -> length >= size) {
                return current;
            }
            current = current -> next;
        }
        return NULL; //could not find any free block that fits "size" bytes
    } else { //current_policy == BEST_FIT
        int best_length = current -> length;
        block *best_block = NULL;

        while (current != NULL) {
            int current_length = current->length;
            if (current_length >= size && current_length < best_length) {
                best_length = current_length;
                best_block = current;
            }
        }
        return best_block;
    }
}

void print_pb() {
	long pb = (long)sbrk(0);
	printf("Current Program Break: %ld\n", pb);
}

void remove_block_from_free_list(block* free_block) {
	free_block -> prev -> next = free_block -> next;
	free_block -> next -> prev = free_block -> prev;
}

void print_block(block* bl) {
	printf("Starting Address: %ld\n", (long)bl);
	printf("Length: %d\n", bl->length);
	printf("Prev Address: %ld\n", (long) (bl->prev));
	printf("Next Address: %ld\n", (long) (bl->next));
}

void print_free_list() {
	block *cur = head;
	while (cur != NULL) {
		print_block(cur);
		cur = cur -> next;
	}
}

void *my_malloc(int size) {
    //if memory could not be allocated, return NULL and sets my_malloc_error

    block *ptr = head;

    if (ptr == NULL) {
        printf("%s\n", "Head is NULL, need to request memory.");

        ptr = &(block){0, NULL, NULL};
        ptr -> length = min_req_size;
        head = ptr;

        sbrk(min_req_size);
        total_free = (min_req_size-sizeof(block));

        if (head == NULL) {
            printf("%s\n", "Init head failed\n");
        }

    }
    // puts("HEAD: ");
    // print_block(head);

    block *free_block = find_free(size);
    if (free_block == NULL) { // No block is large enough, so need to sbrk
        while (ptr -> next != NULL) {
            ptr = ptr -> next; //go to the last free block
        }
        puts("No block is large enough, need to sbrk");

        int extra_length = (size/min_req_size + 1) * min_req_size; //request the multiple of min_req_size that's right above "size"
        sbrk(extra_length);
        total_free += extra_length;
        ptr -> length += extra_length;

        // printf("Size: %d\n", size);
        // printf("min_req_size: %d\n", min_req_size);
        // printf("size/min_req_size: %d\n", size/min_req_size);
        // printf("Extra length required: %d\n", extra_length);
    }
    free_block = find_free(size);

    // puts("FREE BLOCK: ");
    // print_block(free_block);

    if (free_block == NULL) {
        puts("Error doing sbrk, should find a free_block now but didn't");
        return (void*)-1;
    }

    //Now there should be a free block that's large enough
    if (free_block -> length == size) { //same size requested as the free block's length, simply remove it from free block list
    	puts("Exact free space left.");
    	remove_block_from_free_list(free_block);
    	total_allocated += size;
    } else { //need to make whatever free space left a new free block
    	block *new_free_block = &(block){free_block->length - size, free_block -> prev, free_block -> next};
    	free_block -> length = size;
    	if (free_block -> prev != NULL)
    		free_block -> prev -> next = new_free_block;
    	if (free_block -> next != NULL)
    		free_block -> next -> prev = new_free_block;
    	ptr = free_block; //free block is no longer free
    	total_allocated += size;
    	total_free -= size;
    }

    return ptr;
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

int find_largest_contiguous_free_space() {
	block *cur = head;
	int largest_free = 0;
	while (cur != NULL) {
		if (cur -> length > largest_free) {
			largest_free = cur -> length;
		}
		cur = cur -> next;
	}
	return largest_free;
}

void my_mallinfo() {
    printf("Total Number of Bytes Allocated: %d\n", total_allocated);
    printf("Total Number of Free Bytes: %d\n", total_free);
    printf("Largest Contiguous Free Space: %d\n", find_largest_contiguous_free_space());
}

int main(int argc, char *argv[]) {
	// printf("min_req_size: %d\n", min_req_size);
    long init = (long)sbrk(0);
    // printf("Initial Address: %ld\n", init);

 //    puts("Test find_free with first fit policy:");
 //    my_mallopt(FIRST_FIT);
 //    puts("Before allocating, try to find a free block of 3KB, should return NULL.");
 //    block *free_block = find_free(3*ONE_KB);

 //    if (free_block == NULL) {
 //        puts("Pass: Could not find a free block of 3KB.");
 //    } else {
 //        puts("Fail: found a free block of 3KB.");
 //    }
    puts("Allocate one KB");

    my_mallinfo();
	my_malloc(ONE_KB);
	my_mallinfo();
	print_free_list();

	long current_pb = (long)sbrk(0);

	// printf("Current pb: %ld\n", current_pb);
	long change_in_pb = current_pb - init;
	// printf("%ld\n%ld\n", change_in_pb, (long)min_req_size);

	if(change_in_pb == (long)min_req_size) {
		puts("Pass: malloc requested 128KB");
	} else {
		printf("Fail: malloc did not request 128KB, %ld instead\n", change_in_pb);
	}
	// print_free_list();

    // puts("After allocating 2KB, try to find a free block of 3KB, should return a block of size 128KB (Minimum request to sbrk).");
    // my_malloc(2*ONE_KB);
    // free_block = find_free(3*ONE_KB);
    // if (free_block == NULL) {
    //     puts("Fail: Could not find a free block of 3KB.");
    // } else {
    //     puts("Pass: found a free block of 3KB.");
    // }


    // printf("Size of block: %d\n", (int)sizeof(block));
    // printf("Size of block*: %d\n", (int)sizeof(block*));
    // printf("Size of int: %d\n", (int)sizeof(int));
    // printf("Size of char: %d\n", (int)sizeof(char));
    // printf("Size of char*: %d\n", (int)sizeof(char*));
    // printf("Address of head:%ld\n", (long)head);
    // long final = (long)sbrk(0);
    // printf("Final Address: %ld\n", final);
    // printf("Difference: %ld\n", final - init);

    // char *ptrs[32];
    // int i;

    // // try allocating 32 KB
    // for (i=0; i< 2; i++) {
    //     ptrs[i] = (char*)my_malloc(1024);
    //     printf("%ld\n", (long)ptrs[i]);
    //     printf("Address: %ld\n", (long)sbrk(0));
    // }

    return 0;
}
