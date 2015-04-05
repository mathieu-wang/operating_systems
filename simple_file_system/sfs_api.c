#include "sfs_api.h"
#include "disk_emu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAGIC_NUMBER 0xAABB0005
#define BLOCK_SIZE 512
#define NUM_BLOCKS 1000
#define NUM_INODES 111
#define ROOT_DIR_IND 0

//inode modes
#define FILE 0
#define DIR 1

#define DEFAULT_UID 1
#define DEFAULT_GID 1

#define NUM_INODE_DIR_BLOCKS 60

#define MAX_FNAME_LENGTH 20
#define MAX_NUM_FILES 110
#define OPEN_FILES_LIMIT 110
#define MAX_BYTES 30000

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

// does not implement atime, ctime, mtime, access mode and more than one level of indirect blocks
typedef struct inode {
	int mode;
	int size;
	int blockCount;
	int uid;
	int gid;
	int blockPtrs[NUM_INODE_DIR_BLOCKS];
	int indirectBlock;
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
	inodeTable[ROOT_DIR_IND] = (inode*)malloc(sizeof(inode));
	inodeTable[ROOT_DIR_IND] -> mode = DIR;
	inodeTable[ROOT_DIR_IND] -> size = 0;
	inodeTable[ROOT_DIR_IND] -> blockCount = 1;
	inodeTable[ROOT_DIR_IND] -> uid = DEFAULT_UID;
	inodeTable[ROOT_DIR_IND] -> gid = DEFAULT_GID;
	inodeTable[ROOT_DIR_IND] -> blockPtrs[0] = NUM_INODES + 1; //use the block after inode table to store root directory data
	int i;
	for (i = 1; i < NUM_INODE_DIR_BLOCKS; i++) {
		inodeTable[ROOT_DIR_IND] -> blockPtrs[i] = -1;
	}
	inodeTable[ROOT_DIR_IND] -> indirectBlock = -1;
}

void writeToDisk() {
	write_blocks(0, 1, (void*)&sb);
	int i;
	for (i = 1; i < NUM_INODES; i++) {
		if (inodeTable[i] != NULL)
			write_blocks(i, 1,(void*)inodeTable[i]); //one inode per block
	}
	int rootDirBlockCount = inodeTable[ROOT_DIR_IND]->blockCount;
	int rootDirBlockPtrStart = inodeTable[ROOT_DIR_IND]->blockPtrs[0];
	write_blocks(rootDirBlockPtrStart, rootDirBlockCount, (void*)rootDir); //first data block
	write_blocks(NUM_BLOCKS-1, 1, (void*)&freeBlockList);
}

void readFromDisk() {
	read_blocks(0, 1, (void*)&sb);
	int i;
	for (i = 1; i < NUM_INODES-10; i++) {
		read_blocks(i, 1,(void*)inodeTable[i]); //one inode per block
	}
	int rootDirBlockCount = inodeTable[ROOT_DIR_IND]->blockCount;
	int rootDirBlockPtrStart = inodeTable[ROOT_DIR_IND]->blockPtrs[0];
	read_blocks(rootDirBlockPtrStart, rootDirBlockCount, (void*)rootDir); //first data block
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

void printRootDir();

int mksfs(int fresh){
	char* diskName = "Disk";
	if (fresh) {
		if (init_fresh_disk(diskName, BLOCK_SIZE, NUM_BLOCKS) == -1)
			return -1;

		initRootDirInode();
		initFreeBlockList();
		writeToDisk();

		initFdt();

		//printRootDir();

		return 0;
	} else {
		if (init_disk(diskName, BLOCK_SIZE, NUM_BLOCKS) == -1)
			return -1;

		readFromDisk();
		initFdt();

		//printRootDir();

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
			inodeTable[i] -> size = 0;
			inodeTable[i] -> blockCount = 1;
			inodeTable[i] -> uid = DEFAULT_UID;
			inodeTable[i] -> gid = DEFAULT_GID;
			int j;
			for (j = 1; j < NUM_INODE_DIR_BLOCKS; j++) {
				inodeTable[i] -> blockPtrs[j] = -1;
			}
			inodeTable[i] -> indirectBlock = -1;
			return i;
		}
	}
	return NO_MORE_FREE_INODE;
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

int removeFileInRootDir(char *name) {
	int i;
	for (i = 0; i < MAX_NUM_FILES; i++) {
		if (rootDir[i] != NULL && strcmp(rootDir[i]->fname, name) == 0) {
			free(rootDir[i]);
			return 0;
		}
	}
	return FILE_DNE;
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

void printRootDir() {
	printf("\nRoot Directory\n");
	int i;
	for(i = 0; i < MAX_NUM_FILES; i++) {
		if(rootDir[i] != NULL)	{
			printf("fname: %s, inodeIndex: %d\n", rootDir[i]->fname, rootDir[i]->inodeIndex);
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
	//printf("GETTING fileInodeInd for %s\n", name);
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
		fdt[fdIndex].rwPtr = inodeTable[fileInodeInd] -> size;
	}

	// printFdt();
	// printRootDir();
	return fdIndex;
}

int sfs_fclose(int fileID) {
	// check if file is open
	if (fdt[fileID].hasFile != 1) {
		printf("File with index %d is not open..\n", fileID);
		return -1;
	}
	initFdtEntry(fileID);
	// printFdt();
	// printRootDir();
	return 0;
}

int getStartingBlockPtrIndex(inode* in, int length) {
	int i;
	for (i = 0; i < NUM_INODE_DIR_BLOCKS-1; i++){
		// printf("Length: %d\n", length);
		if (length >= in->blockPtrs[i]*BLOCK_SIZE && length <= in->blockPtrs[i+1]*BLOCK_SIZE){
			return i;
		}
	}
	return -1;
}

int sfs_fwrite(int fileID, const char *buf, int length) {
	if(length < 0){
		return -1;
	}

	int fileInodeIndex = fdt[fileID].inodeIndex;
	inode* fileInode = inodeTable[fileInodeIndex];
	int rwPtr = fdt[fileID].rwPtr;

	//getFirstFreeBlock
	// printf("file size: %d\n", fileInode -> size);

	if (fileInode -> size == 0) { //empty file
		int numBlocks = length/BLOCK_SIZE;

		if (numBlocks == 0) { //less than a block
			int freeDataBlock = getFirstFreeBlock();
			write_blocks(freeDataBlock, 1, (void*)buf);
		} else {
			//TODO
		}
	} else { //file already has content
		//Not an empty file

		int startingBlockPtrIndex = getStartingBlockPtrIndex(fileInode, length);
		if (startingBlockPtrIndex < 0) {
			return -1;
		}
		int startingBlock = fileInode -> blockPtrs[startingBlockPtrIndex];

		int offsetFromLastBlock = rwPtr - (startingBlock-1)*BLOCK_SIZE;
		int remainingBytesInCurrentBlock = BLOCK_SIZE - offsetFromLastBlock;

		if(length < remainingBytesInCurrentBlock) { //current block is enough
			char* tmpBuffer[BLOCK_SIZE];

			memset(tmpBuffer, 0, sizeof(tmpBuffer));
			read_blocks(startingBlock, 1, tmpBuffer);

			memcpy((void *)&(tmpBuffer[offsetFromLastBlock]),(const void *)buf, length); 
			write_blocks(startingBlock, 1, tmpBuffer); //overwrite block with old + new content

		} else {
			//TODO
		}
	}
	return length;
}
int sfs_fread(int fileID, char *buf, int length) {
	if(fdt[fileID].hasFile != 1 || length < 0){
		return -1;
	}
	int numBlocks = length/BLOCK_SIZE;
	int fileInodeIndex = fdt[fileID].inodeIndex;
	inode* fileInode = inodeTable[fileInodeIndex];
	int rwPtr = fdt[fileID].rwPtr;

	int i;
	for (i = 0; i < numBlocks; i++) {
		printf("blockPtrs[i]: %d\n", fileInode -> blockPtrs[i]);
		if (fileInode -> blockPtrs[i] == -1) { //no data
			break;
		}
		read_blocks(fileInode -> blockPtrs[i], 1, buf + i*BLOCK_SIZE);
	}
	return length;
}

int sfs_fseek(int fileID, int offset) {
	if (fdt[fileID].hasFile != 1) {
		printf("Cannot seek: file with id %d is not open\n",fileID);
		return -1;
	}

	if (offset < 0)	{
		printf("Cannot seek: negative offset\n");
		return -1;
	}

	int inodeIndex = fdt[fileID].inodeIndex;

	if (offset > inodeTable[inodeIndex]->size) {
		printf("Cannot seek: offset greater than file size\n");
		return -1;
	}

	fdt[fileID].rwPtr = offset;
	return 0;
}
int sfs_remove(char *file) {
	// if file does not exist, error, return
	// printf("BEFORE:\n");
	// printRootDir();

	int fileInodeInd = getFileInodeIndex(file);
	if (fileInodeInd == FILE_DNE) {
		return -1;
	}

	int open = isOpen(file);
	if (open != FILE_NOT_OPEN) {
		return -2; //cannot remove file if open
	}

	int removeStatus = removeFileInRootDir(file);
	if (removeStatus == FILE_DNE) {
		return -1;
	}

	inode* in = inodeTable[fileInodeInd];
	//free(in -> blockPtrs);
	free(in);

	writeToDisk();

	// printf("Removed: %s\n", file);
	// printRootDir();
	return 0;
}
int sfs_get_next_filename(char* filename) {
	//NOT USED
	return 0;
}
int sfs_GetFileSize(const char* path) {
	int locInodeIndex = getFileInodeIndex((char*)path);

	if (locInodeIndex < 0) {
		return -1;
	} else {
		return inodeTable[locInodeIndex]->size;
	}
}
