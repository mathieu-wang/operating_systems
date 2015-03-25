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

typedef struct rootDirEntry {
	char fname[MAX_FNAME_LENGTH];
	int inode_index;
} rootDirEntry;

rootDirEntry* rootDir[MAX_NUM_FILES];

inode* inodeTable[NUM_INODES];

inode* rootDirInode;

int freeBlockList[NUM_BLOCKS];

typedef struct fdtEntry
{
	char fname[MAX_FNAME_LENGTH];
	int inodeIndex;
	int writePtr;
	int readPtr;
} fdtEntry;

fdtEntry fdt[OPEN_FILES_LIMIT];

void initFreeBlockList() {
	int i;
	for (i = 0; i <= NUM_INODES + 1; i++) {
		freeBlockList[i] = NOT_FREE;
	}
	for (i = 0; i < NUM_BLOCKS - 1; i++) {
		freeBlockList[i] = FREE;
	}
}

void initRootDirInode() {
	rootDirInode = inodeTable[ROOT_DIR_IND];
	rootDirInode -> mode = DIR;
	rootDirInode -> size = MAX_NUM_FILES; //?
	rootDirInode -> blockCount = 1; //?
	rootDirInode -> uid = DEFAULT_UID;
	rootDirInode -> gid = DEFAULT_GID;
	rootDirInode -> blockPtrs[0] = NUM_INODES + 1; //use the block after inode table
}

void initInodeTable() {
	int i;
	for (i = 0; i < NUM_INODES; i++) {
		inodeTable[i] = (inode*)malloc(sizeof(inode));
	}
	initRootDirInode();
}

void writeToDisk() {
	write_blocks(0, 1, (void*)&sb);
	int i;
	for (i = 1; i < NUM_INODES; i++) {
		write_blocks(i, 1,(void*)inodeTable[i]); //one inode per block
	}
	write_blocks(NUM_INODES+1, 1, (void*)rootDir); //first data block

	//One block for inode table
	// write_blocks(1, 1, (void*)inodeTable);
	// write_blocks(2, 1, (void*)rootDir);

	write_blocks(NUM_BLOCKS-1, 1, (void*)&freeBlockList);
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
