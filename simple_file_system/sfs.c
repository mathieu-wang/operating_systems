#include "sfs_api.h"
#include "disk_emu.h"

#define MAGIC_NUMBER 0xAABB0005
#define BLOCK_SIZE 512
#define NUM_BLOCKS 1000
#define NUM_INODES 100

//inode modes
#define FILE 0
#define DIR 1

#define DEFAULT_UID 1
#define DEFAULT_GID 1

#define MAX_FNAME_LENGTH 20
#define MAX_NUM_FILES 150
#define OPEN_FILES_LIMIT 150

//Struct and list definitions
typedef struct superBlock {
	//use long for everything because need minimum 4 bytes long
	long magicNumber;// = MAGIC_NUMBER;
	long blockSize;// = BLOCK_SIZE;
	long fsSize;// = NUM_BLOCKS;
	long inodeTableLength;// = NUM_INODES;
	long rootDir;
	long unused;
} superBlock;

// does not implement atime, ctime, mtime, access mode and indirect blocks
typedef struct inode {
	int mode;
	int size;
	int blockCount;
	int uid;
	int gid;
	int blockPtrs[12];
} inode;

int mksfs(int fresh){
	char* disk_name = "Disk";
	return 0;
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
