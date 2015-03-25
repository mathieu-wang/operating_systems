#include "sfs_api.h"
#include "disk_emu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAGIC_NUMBER 0xAABB0005
#define BLOCK_SIZE 512
#define NUM_BLOCKS 1000
#define NUM_INODES 100
#define ROOT_DIR_IND 0

//inode modes
#define FILE 0
#define DIR 1

#define DEFAULT_UID 1
#define DEFAULT_GID 1

#define MAX_FNAME_LENGTH 20
#define MAX_NUM_FILES 150
#define OPEN_FILES_LIMIT 150

#define FREE 0
#define NOT_FREE 1

//Struct and list definitions
typedef struct superBlock {
	//use long for everything because need minimum 4 bytes long
	long magicNumber;
	long blockSize;
	long fsSize;
	long inodeTableLength;
	long rootDirIndex;
	long unused;
} superBlock;

superBlock sb = {MAGIC_NUMBER, BLOCK_SIZE, NUM_BLOCKS, NUM_INODES, ROOT_DIR_IND, 0};


// does not implement atime, ctime, mtime, access mode and indirect blocks
typedef struct inode {
	int mode;
	int size;
	int blockCount;
	int uid;
	int gid;
	int blockPtrs[12];
} inode;

inode* inodeTable[NUM_INODES];

inode* rootDir;

int freeBlockList[NUM_BLOCKS];


void initFreeBlockList() {
	int i;
	for (i = 0; i < NUM_BLOCKS; i++) {
		freeBlockList[i] = FREE;
	}
	freeBlockList[0] = NOT_FREE;
	freeBlockList[1] = NOT_FREE;
	freeBlockList[NUM_BLOCKS-1] = NOT_FREE;
}


void initInodeTable() {
	int i;
	for (i = 0; i < NUM_INODES; i++) {
		inodeTable[i] = (inode*)malloc(sizeof(inode));
	}
	rootDir = inodeTable[ROOT_DIR_IND];
	rootDir -> mode = DIR;
	rootDir -> size = MAX_NUM_FILES; //?
	rootDir -> blockCount = 1; //?
	rootDir -> uid = DEFAULT_UID;
	rootDir -> gid = DEFAULT_GID;
	//TODO: initialize blockPtrs?
}

void writeToDisk() {
	write_blocks(0, 1, (void*)&sb);
	int i;
	for (i = 1; i < NUM_INODES; i++) {
		write_blocks(i, NUM_INODES,(void*)inodeTable[i]); //one inode per block
	}
	write_blocks(NUM_BLOCKS-1,1,(void*)&freeBlockList);
}

int mksfs(int fresh){
	char* diskName = "Disk";
	if (fresh) {
		init_fresh_disk(diskName, BLOCK_SIZE, NUM_BLOCKS);

		initFreeBlockList();
		initInodeTable();

		writeToDisk();

		return 0;
	} else {
		//TODO
	}
}
int sfs_fopen(char *name) {
	return 0;
}
int sfs_fclose(int fileID) {
	return 0;
}
int sfs_fwrite(int fileID, const char *buf, int length) {
	return 0;
}
int sfs_fread(int fileID, char *buf, int length) {
	return 0;
}
int sfs_fseek(int fileID, int offset) {
	return 0;
}
int sfs_remove(char *file) {
	return 0;
}
int sfs_get_next_filename(char* filename) {
	return 0;
}
int sfs_GetFileSize(const char* path) {
	return 0;
}
