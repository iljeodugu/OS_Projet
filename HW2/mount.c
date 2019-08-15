#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"
#include "disk.h"


FileSysInfo* pFileSysInfo = NULL;


void		Mount(MountType type)
{
    if(type == MT_TYPE_FORMAT)
    {
        DevCreateDisk();
        pFileSysInfo = (FileSysInfo*)malloc(sizeof(FileSysInfo));
        pFileSysInfo->blocks =512;
        pFileSysInfo->rootInodeNum = FILESYS_INFO_BLOCK;
        pFileSysInfo->diskCapacity = FS_DISK_CAPACITY;
        pFileSysInfo->numAllocBlocks = 0;
        pFileSysInfo->numFreeBlocks = 493;
        pFileSysInfo->numAllocInodes = 0;
        pFileSysInfo->blockBitmapBlock = BLOCK_BITMAP_BLOCK_NUM;
        pFileSysInfo->inodeBitmapBlock = INODE_BITMAP_BLOCK_NUM;
        pFileSysInfo->inodeListBlock = INODELIST_BLOCK_FIRST;

        pFileSysInfo->numAllocInodes++;
        pFileSysInfo->numFreeBlocks--;
        pFileSysInfo->numAllocInodes++;
        DevWriteBlock(FILESYS_INFO_BLOCK, (char*)pFileSysInfo);
        free(pFileSysInfo);//파일정보를 저장

        char* temp_block = calloc(sizeof(char), BLOCK_SIZE);

        for(int i = 1 ; i < 512; i++)//블록 초기화
        {
            DevWriteBlock(i, temp_block);
        }
        free(temp_block);

        for(int i = 0 ; i < 19 ; i ++)//비트맵 앞에부분 초기화
            SetBlockBitmap(i);

        char my_block[BLOCK_SIZE];

        DirEntry tempDir[4];//루트 엔트리 만들어서 그안에 . 집어넣기
        tempDir[0].inodeNum = 0;
        tempDir[1].inodeNum = -1;
        tempDir[2].inodeNum = -1;
        tempDir[3].inodeNum = -1;
        strcpy(tempDir[0].name, ".");
        int free_block_num = GetFreeBlockNum();
        SetBlockBitmap(GetFreeBlockNum());
        DevWriteBlock(free_block_num, (char*)&tempDir);

        Inode tempInode[4];
        tempInode[0].size = 64;
        tempInode[0].type = FILE_TYPE_DIR;
        tempInode[0].dirBlockPtr[0] = free_block_num;
        tempInode[0].dirBlockPtr[1] = -1;
        tempInode[0].indirBlockPtr = -1;
        PutInode(0, tempInode);
        SetInodeBitmap(0);

        pFileDescTable = (FileDescTable*)malloc(sizeof(FileDescTable));
        for(int i = 0 ; i <64 ; i++)
        {
            pFileDescTable->file[i].bUsed = -1;
        }
    }
    else
    {
        DevOpenDisk();
        pFileSysInfo = (FileSysInfo*)malloc(sizeof(FileSysInfo));

        DevReadBlock(FILESYS_INFO_BLOCK, (char*)pFileSysInfo);
        pFileDescTable = (FileDescTable*)malloc(sizeof(FileDescTable));
        for(int i = 0 ; i <64 ; i++)
        {
            pFileDescTable->file[i].bUsed = -1;
        }
    }
}


void		Unmount(void)
{
    DevCloseDisk();
}

