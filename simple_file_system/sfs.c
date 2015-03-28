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
#define FULL -1

#define FILE_NOT_OPEN -1
#define FILE_DNE -2
#define OPEN_FILES_LIMIT_REACHED -3

#define NO_MORE_FREE_INODE -1
#define ROOT_DIRECTORY_FULL -1

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
	char fname[MAX_FNAME_LENGTH+1];
	int inodeIndex;
} rootDirEntry;

rootDirEntry* rootDir[MAX_NUM_FILES];

inode* inodeTable[NUM_INODES];

inode* rootDirInode;

int freeBlockList[NUM_BLOCKS];

typedef struct fdtEntry {
	char fname[MAX_FNAME_LENGTH+1];
	int hasFile;
	int inodeIndex;
	int rwPtr;
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
	rootDirInode = (inode*)malloc(sizeof(inode));
	rootDirInode -> mode = DIR;
	rootDirInode -> size = 0;
	rootDirInode -> blockCount = 1;
	rootDirInode -> uid = DEFAULT_UID;
	rootDirInode -> gid = DEFAULT_GID;
	rootDirInode -> blockPtrs[0] = NUM_INODES + 1; //use the block after inode table to store root directory data
}

void writeToDisk() {
	write_blocks(0, 1, (void*)&sb);
	int i;
	for (i = 1; i < NUM_INODES; i++) {
		if (inodeTable[i] != NULL)
			write_blocks(i, 1,(void*)inodeTable[i]); //one inode per block
	}
	write_blocks(NUM_INODES+1, 1, (void*)rootDir); //first data block
	write_blocks(NUM_BLOCKS-1, 1, (void*)&freeBlockList);
}

void readFromDisk() {
	read_blocks(0, 1, (void*)&sb);
	int i;
	for (i = 1; i < NUM_INODES-2; i++) {
		read_blocks(i, 1,(void*)inodeTable[i]); //one inode per block
	}
	read_blocks(NUM_INODES+1, 1, (void*)rootDir); //first data block
	read_blocks(NUM_BLOCKS-1, 1, (void*)&freeBlockList);
}

int getFirstFreeBlock() {
	int i;
	for (i = 0; i < NUM_BLOCKS;i++) {
		if (freeBlockList[i] == FREE) {
			return i;
		}
	}
	return FULL;
}

void initFdtEntry(int index) {
	fdt[index].hasFile = 0;
	fdt[index].inodeIndex = -1;
	fdt[index].rwPtr = 0;
	strcpy(fdt[index].fname, "");
}

void initFdt() {
	int i;
	for (i = 0; i < OPEN_FILES_LIMIT; i++) {
		initFdtEntry(i);
	}
}

int mksfs(int fresh){
	char* diskName = "Disk";
	if (fresh) {
		if (init_fresh_disk(diskName, BLOCK_SIZE, NUM_BLOCKS) == -1)
			return -1;

		initRootDirInode();
		initFreeBlockList();
		writeToDisk();

		initFdt();

		return 0;
	} else {
		if (init_disk(diskName, BLOCK_SIZE, NUM_BLOCKS) == -1)
			return -1;

		readFromDisk();
		initFdt();

		return 0;
	}
}

//checks whether a file is open, i.e., exists in FDT.
//returns the index of file in FDT if open, FILE_DNE otherwise.
int isOpen(char* name) {
	int i;
	for (i = 0; i < OPEN_FILES_LIMIT; i++) {
		if (fdt[i].hasFile == 1) {
			if(strcmp(fdt[i].fname, name)==0) {
				return i;
			}
		}	
	}
	//printf("File %s is not open\n", name);
	return FILE_NOT_OPEN;
}

int getFileInodeIndex(char* name) {
	int i;
	for (i = 0; i < MAX_NUM_FILES; i++) {
		if (rootDir[i] != NULL && strcmp(rootDir[i]->fname, name) == 0) {
			return rootDir[i]->inodeIndex;
		}
	}
	return FILE_DNE;
}

int getFirstFreeFdtEntry() {
	int i;
	for(i = 0; i < OPEN_FILES_LIMIT; i++) {
		if(fdt[i].hasFile != 1)	{
			return i;
		}
	}
	return OPEN_FILES_LIMIT_REACHED;
}

int createInode() {
	int i;
	for (i = 0; i < NUM_INODES; i++) {
		if (inodeTable[i] == NULL) {
			inodeTable[i] = (inode*)malloc(sizeof(inode));
			inodeTable[i] -> mode = FILE;
			inodeTable[i] -> size = 1;
			inodeTable[i] -> blockCount = 0;
			inodeTable[i] -> uid = DEFAULT_UID;
			inodeTable[i] -> gid = DEFAULT_GID;
			return i;
		}
	}
	return NO_MORE_FREE_INODE;
}

int getFirstFreeBlockInRootDir() {
	return 0;
}

int createFileInRootDir(char *name, int inodeIndex) {
	int i;
	for (i = 0; i < MAX_NUM_FILES; i++) {
		if (rootDir[i] == NULL) {
			rootDir[i] = (rootDirEntry*)malloc(sizeof(rootDirEntry));
			strcpy(rootDir[i]->fname, name);
			rootDir[i]->inodeIndex = inodeIndex;
			return i;
		}
	}
	return ROOT_DIRECTORY_FULL;
}

void printFdt() {
	printf("\nFile Descriptor Table\n");
	int i;
	for(i = 0; i < OPEN_FILES_LIMIT; i++) {
		if(fdt[i].hasFile == 1)	{
			printf("index: %d, fname: %s, inodeIndex: %d, rwPtr: %d\n", i, fdt[i].fname, fdt[i].inodeIndex, fdt[i].rwPtr);
		}
	}
}

int sfs_fopen(char *name) {
	int open = isOpen(name);

	if (open != FILE_NOT_OPEN) {
		printf("File already open..\n");
		return open;
	}

	// check if file exists
	int fileInodeInd = getFileInodeIndex(name);
	int fdIndex = -1;

	if (fileInodeInd == FILE_DNE) { // file doesn not exist --> create file
		int inodeIndex = createInode();
		if (inodeIndex != NO_MORE_FREE_INODE) {
			createFileInRootDir(name, inodeIndex);
			fileInodeInd = inodeIndex;
		}
		writeToDisk();
	}
	// file now exists --> open
	fdIndex = getFirstFreeFdtEntry();
	if (fdIndex >= 0) {
		strcpy(fdt[fdIndex].fname, name);
		fdt[fdIndex].hasFile = 1;
		fdt[fdIndex].inodeIndex = fileInodeInd;
	}

	//printFdt();
	return fdIndex;
}

int sfs_fclose(int fileID) {
	// check if file is open
	if (fdt[fileID].hasFile != 1) {
		printf("File with index %d is not open..\n", fileID);
		return -1;
	}
	initFdtEntry(fileID);
	//printFdt();
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
