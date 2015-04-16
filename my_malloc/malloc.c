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
	if (free_block -> prev != NULL) {
		free_block -> prev -> next = free_block -> next;
	}
	if (free_block -> next != NULL) {
		free_block -> next -> prev = free_block -> prev;
	}
}

void print_block(block* bl) {
	// printf("Starting Address: %p\n", bl);
	// printf("Length: %d\n", bl->length);
	// printf("Prev Address: %p\n", bl->prev);
	// printf("Next Address: %p\n", bl->next);
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
    void *init_pb = sbrk(0);

    printf("sbrk(0): %p\n", init_pb);

    if (ptr == NULL) {
        printf("%s\n", "Head is NULL, need to request memory.");

        sbrk(min_req_size);
        void *new_pb = sbrk(0);

        // printf("new pb: %p\n",new_pb);

        ptr = init_pb;
        *ptr = (block){min_req_size-sizeof(block), NULL, NULL};
        head = ptr;

        total_free = (min_req_size-sizeof(block));

        if (head == NULL) {
            printf("%s\n", "Init head failed\n");
        }

    }

    printf("sbrk(0): %ld\n", (long)sbrk(0));


    block *free_block = find_free(size);
    if (free_block == NULL) { // No block is large enough, so need to sbrk
        while (ptr -> next != NULL) { //ptr == head here
            ptr = ptr -> next; //go to the last free block
        }
        puts("No block is large enough, need to sbrk");

        // printf("ptr length: %d\n", ptr->length);

        int extra_length = ((size - ptr->length)/min_req_size + 1) * min_req_size; //request the multiple of min_req_size that's right above "size"
        sbrk(extra_length);
        total_free += extra_length;
        ptr -> length += extra_length;

        // printf("Size: %d\n", size);
        // printf("min_req_size: %d\n", min_req_size);
        // printf("(size - ptr->length)/min_req_size : %d\n", (size - ptr->length)/min_req_size);
        // printf("Extra length required: %d\n", extra_length);
    }
    free_block = find_free(size);

    if (free_block == NULL) {
        puts("Error doing sbrk, should find a free_block now but didn't");
        return (void*)-1;
    }

    //Now there should be a free block that's large enough
    
    //IF FREE_BLOCK IS HEAD, NEED TO HAVE SPECIAL CASE
    if (free_block == head) {
    	print_block(head);
    	// printf("sbrk(0): %p\n", sbrk(0));
    	int new_length = head->length - size;
    	block* next = head -> next;

    	// printf("new length: %d\n", new_length);

    	// printf("old head : %ld\n", (long)head);
    	// printf("old head + size : %ld\n", (long)(head + size));
    	// printf("old head + 10240 : %ld\n", (long)(head + 10240));
    	head = head + size/sizeof(block);

    	// printf("size: %d\n", size);
    	// printf("sbrk(0): %ld\n", (long)sbrk(0));

    	// printf("new head: %ld\n", (long)head);

    	// printf("diff: %ld\n", (long)head - (long)sbrk(0));
    	head -> length = new_length;
    	head -> next = next;

    	ptr -> length -= (head -> length); //update length of old head (ptr)

    	puts("ptr to be returned: ");
    	print_block(ptr);

    	puts("new head:");
    	print_block(head);
    } else {
    	if (free_block -> length == size) { //same size requested as the free block's length, simply remove it from free block list
    		puts("Exact free space left.");
    		remove_block_from_free_list(free_block);
    	} else { //need to make whatever free space left a new free block
    		block *new_free_block = free_block + size/sizeof(block);
    		*new_free_block = (block){free_block->length - size, free_block -> prev, free_block -> next};
    		free_block -> length = size;
    		if (free_block -> prev != NULL) {
    			puts("Setting previous block's next pointer");
    			free_block -> prev -> next = new_free_block;
    		}
    		if (free_block -> next != NULL) {
    			puts("Setting next block's prev pointer");
    			free_block -> next -> prev = new_free_block;
    		}
    		ptr = free_block; //free block is no longer free
    	}
    }
    total_allocated += size;
    total_free -= size;

    return ptr;
}

int is_free(block* ptr) { //checks whether ptr is in the free blocks list
	block *cur = head;
	while (cur != NULL) {
		if (cur == ptr) return 1;
		cur = cur -> next;
	}
	return 0;
}

block* find_left(block* ptr) {
	block *cur = head;
	while (cur != NULL) {
		if (cur + cur->length/sizeof(block) == ptr) {
			return cur;
		}
		cur = cur -> next;
	}
	return (block*)-1;
}

void reduce_last_free_block_if_larger_than_128_kb() {
	block *cur = head;
	//assume head is not null
	while (cur -> next != NULL) { //go to last free block
		cur = cur -> next;
	}
	int excess = cur -> length - 128*ONE_KB;

	if (excess > 0) {
		printf("Last block has %d bytes (%d KB) of free memory, reducing it to 128KB", cur->length, (cur->length)/1024);
		cur -> length = 128*ONE_KB;
		sbrk(-1*(excess+sizeof(block)));//add a sizeof(block) so free memory is really 128KB
		total_free -= excess;
	}
}

void my_free(void *ptr) {
	if (ptr == NULL) {
		return; //do nothing if ptr is null
	}
	block* bl = (block*) ptr;

	int length = bl -> length;

	// printf("length of ptr to be freed: %d, total free: %d\n", length, total_free);

	block* right = bl + length/sizeof(block);

	// puts("Current block: ");
	// print_block(bl);
	// puts("RIGHT BLOCK: ");
	// print_block(right);
	if (is_free(right)) { //if right block free, put free back into list with right length
		puts("right is free, merging...");
		//remove right from list
		if (right -> prev != NULL)
			right -> prev -> next = bl;
		if (right -> next != NULL)
			right -> next -> prev = bl;
		bl -> prev = right -> prev;
		bl -> next = right -> next;
		bl -> length += right -> length + sizeof(block);

		total_free += sizeof(block);
		// puts("block: ");
		// print_block(bl);
	}

	block* left = find_left(bl);

	if ((long)left != -1) { //left is free
		puts("left is free; merging...");
		// puts("LEFT: ");
		// print_block(left);

		//remove bl from list
		if (bl -> prev != NULL)
			bl -> prev -> next = left;
		if (bl -> next != NULL)
			bl -> next -> prev = left;
		left -> prev = bl -> prev;
		left -> next = bl -> next;
		left -> length += bl -> length + sizeof(block);

		// puts("NEW LEFT AFTER MERGING: ");
		// print_block(left);

		bl = left;

		total_free += sizeof(block);
	}

	// puts("head: ");
	// print_block(head);

	if (bl < head) {
		// puts("smaller than head");
		bl -> next = head;
		head = bl;
	} else if (bl == head) {
		//do nothing
	} else { //add bl to free list
		block *cur = head;
		while (cur != NULL) {
			// puts("CUR: ");
			// print_block(cur);
			if (cur -> next == NULL) {
				// puts("CUR HAS NO NEXT");
				cur -> next = bl;
				bl -> prev = cur;
				break;
			} else if (cur < bl && bl < cur -> next) {
				// puts("BL BETWEEN CUR AND CUR -> NEXT");
				bl -> next = cur -> next;
				bl -> prev = cur;
				cur -> next = bl;
				break;
			}
			cur = cur -> next;
		}
	}

	// puts("BLOCK AFTER EVERYTHING");
	// print_block(bl);

	reduce_last_free_block_if_larger_than_128_kb();

	// printf("total free: %d, Length to be added to free: %d\n", total_free, length);
    total_free += length;
    total_allocated -= length;
}

void my_mallopt(int policy) {
    current_policy = policy;
}

int find_largest_contiguous_free_space() {
	block *cur = head;
	int largest_free = 0;
	while (cur != NULL) {
		// print_block(cur);
		if ((cur -> length) > largest_free) {
			largest_free = cur -> length;
		}
		cur = cur -> next;
	}
	return largest_free;
}

void my_mallinfo() {
	int largest_contiguous_free = find_largest_contiguous_free_space();
    printf("\nTotal Number of Bytes Allocated: %d (= %d KB)\n", total_allocated, total_allocated/1024);
    printf("Total Number of Free Bytes:      %d (= %d KB)\n", total_free, total_free/1024);
    printf("Largest Contiguous Free Space:   %d (= %d KB)\n\n", largest_contiguous_free, largest_contiguous_free/1024);
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
    puts("TEST 1: Allocate one KB");

	void *one_kb_ptr = my_malloc(ONE_KB);
	my_mallinfo();
	// print_free_list();

	long current_pb = (long)sbrk(0);

	// printf("Current pb: %ld\n", current_pb);
	long change_in_pb = current_pb - init;
	// printf("%ld\n%ld\n", change_in_pb, (long)min_req_size);

	if(change_in_pb == (long)min_req_size) {
		puts("TEST 1 PASSED: malloc moved program break by 128KB and allocated 1KB\n");
	} else {
		printf("TEST 1 FAILED: malloc did not move program break by 128KB, %ld instead\n", change_in_pb);
	}


	puts("TEST 2: Allocate another 10KB");

	init = (long)sbrk(0);
	void *ten_kb_ptr =my_malloc(10*ONE_KB);
	my_mallinfo();

	current_pb = (long)sbrk(0);
	change_in_pb = current_pb - init;
	if(change_in_pb == 0) {
		puts("TEST 2 PASSED: malloc did not move the program break\n");
	} else {
		printf("TEST 2 FAILED: malloc moved program break by %ld \n\n", change_in_pb);
	}


	puts("TEST 3: Allocate another 300KB");

	init = (long)sbrk(0);
	void *three_hundred_kb_ptr = my_malloc(300*ONE_KB);
	my_mallinfo();

	current_pb = (long)sbrk(0);
	change_in_pb = current_pb - init;

	if(change_in_pb == (long)(2*min_req_size)) {
		puts("TEST 3 PASSED: malloc moved program break by 2*128KB and allocated 300KB\n");
	} else {
		printf("TEST 3 FAILED: malloc did not move program break by 2*128KB, %ld instead\n", change_in_pb);
	}



	puts("TEST 4: Free the 10KB pointer");
	init = (long)sbrk(0);
	my_free(ten_kb_ptr);
	my_mallinfo();

	current_pb = (long)sbrk(0);
	change_in_pb = current_pb - init;
	if(change_in_pb == 0) {
		puts("TEST 4 PASSED: malloc did not move the program break and successfully freed the 10KB pointer\n");
	} else {
		printf("TEST 4 FAILED: malloc moved program break by %ld \n\n", change_in_pb);
	}

	// puts("TEST 5: Free the 1KB pointer");
	// my_free(one_kb_ptr);
	// my_mallinfo();


	puts("TEST 5: Free the 300KB pointer");
	init = (long)sbrk(0);
	my_free(three_hundred_kb_ptr);
	my_mallinfo();

	current_pb = (long)sbrk(0);
	change_in_pb = current_pb - init;
	if(change_in_pb == -(255*1024+2*sizeof(block))) {
		puts("TEST 5 PASSED: malloc moved the program break by -255KB and successfully freed the 300KB pointer\n");
	} else {
		printf("TEST 5 FAILED: malloc did not move program break by -255KB,  %ldKB instead\n\n", change_in_pb);
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

    return 0;
}
