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
} block;

block *head;

void *find_free(int size) {
    if (head == NULL) return NULL;

    block *current = head;

    if (current_policy == FIRST_FIT) {
        while (current != NULL) {
            if (current -> length > size) {
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
            if (current_length > size && current_length < best_length) {
                best_length = current_length;
                best_block = current;
            }
        }
        return best_block;
    }
}

void *my_malloc(int size) {
    //if memory could not be allocated, return NULL and sets my_malloc_error

    block *ptr = head;

    if (ptr == NULL) {
        printf("\n%s\n\n", "Head is NULL, need to request memory.");

        ptr = &(block){0, NULL, NULL, NULL};
        ptr -> length = MIN_REQUEST_SIZE;
        head = ptr;

        sbrk(sizeof(block) + MIN_REQUEST_SIZE);
    }

    block *free_block = find_free(size);

    if (head == NULL) {
        printf("%s\n", "Init head failed\n");
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

void my_mallinfo() {
    // should print:
    //-total number of bytes allocated
    //-total free space
    //-largest contiguous free space
    //-other info

}

int main(int argc, char *argv[]) {
    long init = (long)sbrk(0);
    printf("Initial Address: %ld\n", init);

    puts("Test find_free with first fit policy:");
    my_mallopt(FIRST_FIT);
    puts("Before allocating, try to find a free block of 3KB, should return NULL.");
    block *free_block = find_free(3*ONE_KB);

    if (free_block == NULL) {
        puts("Pass: Could not find a free block of 3KB.");
    } else {
        puts("Fail: found a free block of 3KB.");
    }

    puts("After allocating 2KB, try to find a free block of 3KB, should return a block of size 128KB (Minimum request to sbrk).");
    my_malloc(2*ONE_KB);
    free_block = find_free(3*ONE_KB);
    if (free_block == NULL) {
        puts("Fail: Could not find a free block of 3KB.");
    } else {
        puts("Pass: found a free block of 3KB.");
    }


    printf("Size of block: %d\n", sizeof(block));
    printf("Size of block*: %d\n", sizeof(block*));
    printf("Size of int: %d\n", sizeof(int));
    printf("Size of char: %d\n", sizeof(char));
    printf("Size of char*: %d\n", sizeof(char*));
    printf("Address of head:%ld\n", head);
    long final = (long)sbrk(0);
    printf("Final Address: %ld\n", final);
    printf("Difference: %ld\n", final - init);

    char *ptrs[32];
    int i;

    // try allocating 32 KB
    for (i=0; i< 32; i++) {
        ptrs[i] = (char*)my_malloc(1024);
        printf("%ld\n", ptrs[i]);
        printf("Address: %ld\n", (long)sbrk(0));
    }

    return 0;
}
