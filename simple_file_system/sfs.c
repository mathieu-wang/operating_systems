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
	int inodeIndex;
} rootDirEntry;

rootDirEntry* rootDir[MAX_NUM_FILES];

inode* inodeTable[NUM_INODES];

inode* rootDirInode;

int freeBlockList[NUM_BLOCKS];

typedef struct fdtEntry {
	char fname[MAX_FNAME_LENGTH];
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
	for (i = 1; i < NUM_INODES; i++) {
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

void initFDT() {
	int i;
	for (i = 0; i < OPEN_FILES_LIMIT; i++) {
		fdt[i].hasFile = 0;
		fdt[i].inodeIndex = -1;
		fdt[i].rwPtr = -1;
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

		initFDT();

		return 0;
	} else {
		if (init_disk(diskName, BLOCK_SIZE, NUM_BLOCKS) == -1)
			return -1;

		readFromDisk();
		initFDT();

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
	return FILE_NOT_OPEN;
}

int getFileInodeIndex(char* name) {
	int i;
	for (i = 0; i < MAX_NUM_FILES; i++) {
		if (strcmp(rootDir[i]->fname, name) == 0) {
			return rootDir[i]->inodeIndex;
		}
	}
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

}

int createFileInRootDir(char *name, int inodeIndex) {
	return 0;
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
		//TODO create file
		int inodeIndex = createInode();
		if (inodeIndex != NO_MORE_FREE_INODE) {
			fdIndex = createFileInRootDir(name, inodeIndex);
		}
		return fdIndex;

	} else { // file exists --> open
		int fdIndex = getFirstFreeFdtEntry();
		if (fdIndex >= 0) {
			fdt[fdIndex].hasFile = 1;
			fdt[fdIndex].inodeIndex = fileInodeInd;
		}
	}

	return fdIndex;
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
