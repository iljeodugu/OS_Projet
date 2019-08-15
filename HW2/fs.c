#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include "fs.h"
#include "chang_lib.h"

FileDescTable* pFileDescTable = NULL;


int		OpenFile(const char* szFileName, OpenFlag flag)
{
    char* name;

    char temp[64];
    int count=0;
    int find_inode = 0;
    int inode_number = 0;
    int need_allocate_dir_number;
    int directory_number;
    char temp_block[BLOCK_SIZE];

    Inode temp_inode;
    GetInode(inode_number, &temp_inode);

    DirEntry directory_block[4];
    Single_direct_block direct_block;

    strcpy(temp,szFileName);
    name = strtok(temp, "/");

    while(name != NULL)
    {
        name = strtok(NULL, "/");
        count++;
    }

    strcpy(temp,szFileName);
    name = strtok(temp, "/");

    for(int i = 0 ; i < count ; i ++)
    {
        GetInode(find_inode, &temp_inode);
        DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);

        if(i == count-1)//다찾았고 블록을 할당하는 단계
        {
            int j = 0;
            DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
            directory_number = temp_inode.dirBlockPtr[0];
            temp_inode.size += 64;
            PutInode(find_inode, &temp_inode);

	    	for(j=0 ; j < 4 ; j ++)
     	    {
				if(directory_block[j].inodeNum != -1 && strcmp(name, directory_block[j].name) == 0)
                {
                    int i;
                    for(i = 0 ; i < 64 ; i ++)
                    {
                        if(pFileDescTable->file[i].bUsed == -1)
                            break;
                    }
                    pFileDescTable->file[i].bUsed= 1;
                    pFileDescTable->file[i].inodeNum = directory_block[j].inodeNum;
                    pFileDescTable->file[i].fileOffset = 0;

                    return i;
                }
	    	}
 	    	if(temp_inode.dirBlockPtr[1] != -1)
    	    {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for (j = 0; j < 4; j++)
                {
                    if(directory_block[j].inodeNum != -1 && strcmp(name, directory_block[j].name) == 0)
                    {
                        int i;
                        for(i = 0 ; i < 64 ; i ++)
                        {
                            if(pFileDescTable->file[i].bUsed == -1)
                                break;
                        }
                        pFileDescTable->file[i].bUsed= 1;
                        pFileDescTable->file[i].inodeNum = directory_block[j].inodeNum;
                        pFileDescTable->file[i].fileOffset = 0;

                        return i;
                    }
            	}
			}
            DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);

            for (j = 0; j < 4; j++)
            {
                if(directory_block[j].inodeNum == -1)
                {
                    need_allocate_dir_number = j;
                    break;
                }
            }

            if (j==4 && temp_inode.dirBlockPtr[1] == -1)//두번째 블록이 없을경우 2번째 블록을 할당하고 종료
            {
                int free_block = GetFreeBlockNum();//새로운 블록 자리 찾고
                SetBlockBitmap(free_block);
                FileSysInfo info;
                DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
                info.numAllocBlocks++;
                info.numFreeBlocks--;
                info.numAllocInodes++;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);

                GetInode(find_inode, &temp_inode);
                temp_inode.dirBlockPtr[1] = free_block;//블록을 inode에 디렉트 하고
                PutInode(find_inode, &temp_inode);//아이노드를 저장 하고
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                directory_number = temp_inode.dirBlockPtr[1];
                j=0;
                need_allocate_dir_number = 0;
                directory_block[0].inodeNum = -1;
                directory_block[1].inodeNum = -1;
                directory_block[2].inodeNum = -1;
                directory_block[3].inodeNum = -1;
                DevWriteBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
            }

            if(j==4)//두번째 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for (j = 0; j < 4; j++)
                {
                    directory_number = temp_inode.dirBlockPtr[1];
                    if(directory_block[j].inodeNum == -1)
                    {
                        need_allocate_dir_number = j;
                        break;
                    }
                }
            }

            if(j == 4 && temp_inode.indirBlockPtr == -1)//찾지 못했는데 directBlock이 없을경우
            {
                int free_block = GetFreeBlockNum();//여기서 free block은 directBlock
                SetBlockBitmap(free_block);
                FileSysInfo info;
                DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
                info.numAllocBlocks++;
                info.numFreeBlocks--;
                info.numAllocInodes++;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);

                GetInode(find_inode, &temp_inode);
                temp_inode.indirBlockPtr = free_block;
                PutInode(find_inode, &temp_inode);

                int free_dir_block = GetFreeBlockNum();//free_dir_block 은 directBlock[0]이다.
                SetBlockBitmap(free_dir_block);//새로운 directory를 위해서 하나 할당받고
                DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
                info.numAllocBlocks++;
                info.numFreeBlocks--;
                info.numAllocInodes++;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);
                directory_number = direct_block.block_number[0];
                j = 0;
                for(int k = 1 ; k < 16 ; k++)
                    direct_block.block_number[k] = -1;
                direct_block.block_number[0] = free_dir_block;
                GetInode(find_inode, &temp_inode);
                DevWriteBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
                need_allocate_dir_number = 0;
                directory_number = free_dir_block;

                DevReadBlock(direct_block.block_number[0], (char*)&directory_block);
                directory_block[0].inodeNum = -1;
                directory_block[1].inodeNum = -1;
                directory_block[2].inodeNum = -1;
                directory_block[3].inodeNum = -1;
                DevWriteBlock(direct_block.block_number[0], (char*)&directory_block);
            }

            else if(j == 4)//다이렉트 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);

                for (int k = 0 ; k < 16; k ++)//16개 모두 확인을 한다음에
                {
                    if(direct_block.block_number[k] != -1)
                    {
                        int dirnumber;
                        DevReadBlock(direct_block.block_number[k], (char *)directory_block);
                        directory_number = direct_block.block_number[k];
                        for (dirnumber = 0; dirnumber < 4; dirnumber++) {
                            if(strcmp(name, directory_block[dirnumber].name) == 0)
                            {
                                int i;
                                for(i = 0 ; i < 64 ; i ++)
                                {
                                    if(pFileDescTable->file[i].bUsed == -1)
                                        break;
                                }
                                pFileDescTable->file[i].bUsed= 1;
                                pFileDescTable->file[i].inodeNum = directory_block[j].inodeNum;
                                pFileDescTable->file[i].fileOffset = 0;

                                return i;
                            }

                            if (directory_block[dirnumber].inodeNum == -1) {
                                need_allocate_dir_number = dirnumber;
                                break;
                            }
                        }
                        if (dirnumber != 4)
                            break;
                    }
                    else
                    {
                        int temp_block_number = GetFreeBlockNum();
                        direct_block.block_number[k] = temp_block_number;
                        SetBlockBitmap(temp_block_number);
                        DevWriteBlock(temp_inode.indirBlockPtr, (char*)&direct_block);

                        FileSysInfo info;
                        DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
                        info.numAllocBlocks++;
                        info.numFreeBlocks--;
                        info.numAllocInodes++;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);

                        directory_number = direct_block.block_number[k];
                        DevReadBlock(direct_block.block_number[k], (char*)&directory_block);
                        for(int j = 0 ; j < 4 ; j ++)
                        {
                            directory_block[j].inodeNum = -1;
                        }
                        DevWriteBlock(direct_block.block_number[k], (char*)&directory_block);
                        need_allocate_dir_number = 0;
                        break;
                    }
                }
            }
        }
        else//최종 블록을 찾아가는 단계
        {
            int j = 0;
            for (j = 0; j < 4; j++)//첫번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
                if (strcmp(name, directory_block[j].name) == 0) {
                    find_inode = directory_block[j].inodeNum;
                    break;
                }
            }

            if(j==4)//두번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for(j=0 ; j < 4 ; j ++)
                {
                    if(strcmp(name, directory_block[j].name) == 0)
                    {
                        find_inode = directory_block[j].inodeNum;
                        break;
                    }
                }
            }

            if(j == 4)//다이렉트 블록 체크
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
                for ( int k = 0; k < 16; k++)//16개 모두 확인을 한다음에
                {
                    DevReadBlock(direct_block.block_number[k], (char*)directory_block);
                    for (j = 0; j < 4; j++) {
                        if (strcmp(name, directory_block[j].name) == 0)
                        {
                            find_inode = directory_block[j].inodeNum;
                            break;
                        }
                    }
                    if (j != 4)
                        break;
                }
            }
            name = strtok(NULL, "/");
        }
    }//여기까지 작업을 끝마치면 need_allocate_block에는 할당할수 있는 directory의 위치 값이 그리고 directoryblock에는
    //현재 저장해야하는 directory 블록이 저장되어있다. directory_number 현재의 directory 저장할 위치가 있다.
//
    int free_inode_block = GetFreeInodeNum();//inode를 배치해야 합니다.
    FileSysInfo info;
    DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
    info.numAllocBlocks++;
    info.numFreeBlocks--;
    info.numAllocInodes++;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);
    SetInodeBitmap(free_inode_block);

    strcpy(directory_block[need_allocate_dir_number].name, name);
    directory_block[need_allocate_dir_number].inodeNum = free_inode_block;
    DevWriteBlock(directory_number, (char*)directory_block);

    Inode new_inode;
    new_inode.type = FILE_TYPE_FILE;
    new_inode.size = 0;
    new_inode.dirBlockPtr[0] = -1;
    new_inode.dirBlockPtr[1] = -1;
    new_inode.indirBlockPtr = -1;
    PutInode(free_inode_block, &new_inode);
    int i;
    for(i = 0 ; i < 64 ; i ++)
    {
        if(pFileDescTable->file[i].bUsed == -1)
            break;
    }
    pFileDescTable->file[i].bUsed= 1;
    pFileDescTable->file[i].inodeNum = free_inode_block;
    pFileDescTable->file[i].fileOffset = 0;

    return i;
}

int		WriteFile(int fileDesc, char* pBuffer, int length)
{
    int count = pFileDescTable->file[fileDesc].fileOffset/BLOCK_SIZE;
    Inode temp_inode;
    GetInode(pFileDescTable->file[fileDesc].inodeNum, &temp_inode);
    int pBuffer_offset = 0;
    int des_pBuffer_offset = length;
    char file_block[BLOCK_SIZE];
    Single_direct_block direct_block;

    for(int i = 0 ; i < 72 ; i++)
    {
        if(i<count)
            continue;
        if(pBuffer_offset >= des_pBuffer_offset)
            break;

        if(i == 0)
        {
            if(temp_inode.dirBlockPtr[0] == -1)
            {
                temp_inode.dirBlockPtr[0] = GetFreeBlockNum();
                SetBlockBitmap(GetFreeBlockNum());
                PutInode(pFileDescTable->file[fileDesc].inodeNum, &temp_inode);
            }
            DevWriteBlock(temp_inode.dirBlockPtr[0], pBuffer + pBuffer_offset);
            pBuffer_offset += 64;
        }
        else if(i == 1)
        {
            if(temp_inode.dirBlockPtr[1] == -1)
            {
                temp_inode.dirBlockPtr[1] = GetFreeBlockNum();
                SetBlockBitmap(GetFreeBlockNum());
                PutInode(pFileDescTable->file[fileDesc].inodeNum, &temp_inode);
            }
            DevWriteBlock(temp_inode.dirBlockPtr[1], pBuffer + pBuffer_offset);
            pBuffer_offset += 64;
        }
        else
        {
            if(temp_inode.indirBlockPtr == -1)
            {
                GetInode(pFileDescTable->file[fileDesc].inodeNum, &temp_inode);
                temp_inode.indirBlockPtr = GetFreeBlockNum();
                SetBlockBitmap(GetFreeBlockNum());
                PutInode(pFileDescTable->file[fileDesc].inodeNum, &temp_inode);

                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
                for(int k = 0 ; k < 16 ; k ++)
                    direct_block.block_number[k] = -1;
                DevWriteBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
            }
            DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);

            if(direct_block.block_number[i-2] == -1)
            {
                direct_block.block_number[i-2] = GetFreeBlockNum();
                SetBlockBitmap(GetFreeBlockNum());
                DevWriteBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
            }
            DevWriteBlock(direct_block.block_number[i-2], pBuffer + pBuffer_offset);

            pBuffer_offset += 64;
        }
    }
    pFileDescTable->file[fileDesc].fileOffset += length;

    return length;
}

int		ReadFile(int fileDesc, char* pBuffer, int length)
{
    int count = pFileDescTable->file[fileDesc].fileOffset/BLOCK_SIZE;
    Inode temp_inode;
    GetInode(pFileDescTable->file[fileDesc].inodeNum, &temp_inode);
    int pBuffer_offset = 0;
    int des_pBuffer_offset = length;
    char file_block[BLOCK_SIZE];
    Single_direct_block direct_block;

    for(int i = 0 ; i < 72 ; i++)
    {
        if(i<count)
            continue;
        if(pBuffer_offset >= des_pBuffer_offset)
            break;

        if(i == 0)
        {
            DevReadBlock(temp_inode.dirBlockPtr[0], file_block);
            memcpy(pBuffer + pBuffer_offset, file_block, BLOCK_SIZE);
            pBuffer_offset += 64;
        }
        else if(i == 1)
        {
            DevReadBlock(temp_inode.dirBlockPtr[1], file_block);
            memcpy(pBuffer + pBuffer_offset, file_block, BLOCK_SIZE);
            pBuffer_offset += 64;
        }
        else
        {
            DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
            DevReadBlock(direct_block.block_number[i-2], file_block);
            memcpy(pBuffer + pBuffer_offset, file_block, BLOCK_SIZE);
            pBuffer_offset += 64;
        }
    }
    pFileDescTable->file[fileDesc].fileOffset += length;

    return length;
}


int		CloseFile(int fileDesc)
{
    pFileDescTable->file[fileDesc].inodeNum = -1;
    pFileDescTable->file[fileDesc].fileOffset = 0;
    pFileDescTable->file[fileDesc].bUsed = -1;

    return 0;
}

int		RemoveFile(const char* szFileName)
{
    char* name;

    char temp[64];
    int count=0;
    int find_inode = 0;
    int inode_number = 0;
    int need_remove_file_number;
    int directory_number;
    char temp_block[BLOCK_SIZE];

    Inode temp_inode;
    GetInode(inode_number, &temp_inode);

    DirEntry directory_block[4];
    Single_direct_block direct_block;

    strcpy(temp,szFileName);
    name = strtok(temp, "/");

    while(name != NULL)
    {
        name = strtok(NULL, "/");
        count++;
    }

    strcpy(temp,szFileName);
    name = strtok(temp, "/");

    for(int i = 0 ; i < count ; i ++)
    {
        GetInode(find_inode, &temp_inode);
        DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);

        if(i == count-1)//다찾았고 블록을 할당하는 단계
        {
            int j = 0;
            DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
            directory_number = temp_inode.dirBlockPtr[0];
            temp_inode.size += 64;
            PutInode(find_inode, &temp_inode);

            for (j = 0; j < 4; j++)
            {
                if(strcmp(name, directory_block[j].name) == 0)
                {
                    need_remove_file_number = j;
                    break;
                }
            }

            if (j==4 && temp_inode.dirBlockPtr[1] == -1)//두번째 블록이 없을경우 2번째 블록을 할당하고 종료
            {
                return -1;
            }

            if(j==4)//두번째 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for (j = 0; j < 4; j++)
                {
                    directory_number = temp_inode.dirBlockPtr[1];
                    if(strcmp(name, directory_block[j].name) == 0)
                    {
                        need_remove_file_number = j;
                        break;
                    }
                }
            }

            if(j == 4 && temp_inode.indirBlockPtr == -1)//찾지 못했는데 directBlock이 없을경우
            {
                return -1;
            }

            else if(j == 4)//다이렉트 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);

                for (int k = 0 ; k < 16; k ++)//16개 모두 확인을 한다음에
                {
                    if(direct_block.block_number[k] != -1)
                    {
                        int dirnumber;
                        DevReadBlock(direct_block.block_number[k], (char *)directory_block);
                        directory_number = direct_block.block_number[k];

                        for (dirnumber = 0; dirnumber < 4; dirnumber++) {
                            if(strcmp(name, directory_block[dirnumber].name) == 0)
                            {
                                need_remove_file_number = dirnumber;
                                break;
                            }
                        }
                        if (dirnumber != 4)
                            break;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
        else//최종 블록을 찾아가는 단계
        {
            int j = 0;
            for (j = 0; j < 4; j++)//첫번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
                if (strcmp(name, directory_block[j].name) == 0) {
                    find_inode = directory_block[j].inodeNum;
                    break;
                }
            }

            if(j==4)//두번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for(j=0 ; j < 4 ; j ++)
                {
                    if(strcmp(name, directory_block[j].name) == 0)
                    {
                        find_inode = directory_block[j].inodeNum;
                        break;
                    }
                }
            }

            if(j == 4)//다이렉트 블록 체크
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
                for ( int k = 0; k < 16; k++)//16개 모두 확인을 한다음에
                {
                    DevReadBlock(direct_block.block_number[k], (char*)directory_block);
                    for (j = 0; j < 4; j++) {
                        if (strcmp(name, directory_block[j].name) == 0)
                        {
                            find_inode = directory_block[j].inodeNum;
                            break;
                        }
                    }
                    if (j != 4)
                        break;
                }
            }
            name = strtok(NULL, "/");
        }
    }//여기까지 작업을 끝마치면 need_allocate_block에는 할당할수 있는 directory의 위치 값이 그리고 directoryblock에는
    //현재 저장해야하는 directory 블록이 저장되어있다. directory_number 현재의 directory 저장할 위치가 있다.

    int remove_file_inode = directory_block[need_remove_file_number].inodeNum;
    for(int i = 0 ; i < 64 ; i ++)
    {
        if(pFileDescTable->file[i].inodeNum == remove_file_inode)
            return -1;
    }
	strcpy(directory_block[need_remove_file_number].name," "); 
    directory_block[need_remove_file_number].inodeNum = -1;
    DevWriteBlock(directory_number, (char*)directory_block);

    ResetInodeBitmap(remove_file_inode);
    GetInode(remove_file_inode, &temp_inode);
    if(temp_inode.dirBlockPtr[0] != -1)
    {
        ResetBlockBitmap(temp_inode.dirBlockPtr[0]);
        temp_inode.dirBlockPtr[0] = -1;
    }
    if(temp_inode.dirBlockPtr[1] != -1)
    {
        ResetBlockBitmap(temp_inode.dirBlockPtr[1]);
        temp_inode.dirBlockPtr[1] = -1;
    }
    if(temp_inode.indirBlockPtr != -1)
    {
        DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
        for(int i = 0 ; i < 16; i ++)
        {
            if(direct_block.block_number[i] != -1)
            {
                ResetBlockBitmap(direct_block.block_number[i]);
                direct_block.block_number[i] = -1;
            }
        }
    }
    PutInode(remove_file_inode, &temp_inode);

    return 0;
}


int		MakeDir(const char* szDirName)
{
    char* name;

    char temp[64];
    int count=0;
    int find_inode = 0;
    int inode_number = 0;
    int need_allocate_dir_number;
    int directory_number;
    char temp_block[BLOCK_SIZE];

    Inode temp_inode;

    DirEntry directory_block[4];
    Single_direct_block direct_block;

    strcpy(temp,szDirName);
    name = strtok(temp, "/");

    while(name != NULL)
    {
        name = strtok(NULL, "/");
        count++;
    }

    strcpy(temp,szDirName);
    name = strtok(temp, "/");

    for(int i = 0 ; i < count ; i ++)
    {
        GetInode(find_inode, &temp_inode);
        DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);

        if(i == count-1)//다찾았고 블록을 할당하는 단계
        {
            int j = 0;
            DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
            directory_number = temp_inode.dirBlockPtr[0];
            temp_inode.size += 64;
            PutInode(find_inode, &temp_inode);

            for (j = 0; j < 4; j++)
            {
                if(strcmp(name, directory_block[j].name) == 0)
                    return -1;
                if(directory_block[j].inodeNum == -1)
                {
                    need_allocate_dir_number = j;
                    break;
                }
            }

            if (j==4 && temp_inode.dirBlockPtr[1] == -1)//두번째 블록이 없을경우 2번째 블록을 할당하고 종료
            {
                int free_block = GetFreeBlockNum();//새로운 블록 자리 찾고
                SetBlockBitmap(free_block);
                FileSysInfo info;
                DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
                info.numAllocBlocks++;
                info.numFreeBlocks--;
                info.numAllocInodes++;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);

                GetInode(find_inode, &temp_inode);
                temp_inode.dirBlockPtr[1] = free_block;//블록을 inode에 디렉트 하고
                PutInode(find_inode, &temp_inode);//아이노드를 저장 하고
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                directory_number = temp_inode.dirBlockPtr[1];
                j=0;
                need_allocate_dir_number = 0;
                directory_block[0].inodeNum = -1;
                directory_block[1].inodeNum = -1;
                directory_block[2].inodeNum = -1;
                directory_block[3].inodeNum = -1;
                DevWriteBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
            }

            if(j==4)//두번째 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for (j = 0; j < 4; j++)
                {
                    directory_number = temp_inode.dirBlockPtr[1];
                    if(strcmp(name, directory_block[j].name) == 0)
                        return -1;
                    if(directory_block[j].inodeNum == -1)
                    {
                        need_allocate_dir_number = j;
                        break;
                    }
                }
            }

            if(j == 4 && temp_inode.indirBlockPtr == -1)//찾지 못했는데 directBlock이 없을경우
            {
                int free_block = GetFreeBlockNum();//여기서 free block은 directBlock
                SetBlockBitmap(free_block);
                FileSysInfo info;
                DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
                info.numAllocBlocks++;
                info.numFreeBlocks--;
                info.numAllocInodes++;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);

                GetInode(find_inode, &temp_inode);
                temp_inode.indirBlockPtr = free_block;
                PutInode(find_inode, &temp_inode);

                int free_dir_block = GetFreeBlockNum();//free_dir_block 은 directBlock[0]이다.
                SetBlockBitmap(free_dir_block);//새로운 directory를 위해서 하나 할당받고
                DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
                info.numAllocBlocks++;
                info.numFreeBlocks--;
                info.numAllocInodes++;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);
                j = 0;
                for(int k = 1 ; k < 16 ; k++)
                    direct_block.block_number[k] = -1;
                direct_block.block_number[0] = free_dir_block;
                GetInode(find_inode, &temp_inode);
                DevWriteBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
                need_allocate_dir_number = 0;
                directory_number = free_dir_block;

                DevReadBlock(direct_block.block_number[0], (char*)&directory_block);
                directory_block[0].inodeNum = -1;
                directory_block[1].inodeNum = -1;
                directory_block[2].inodeNum = -1;
                directory_block[3].inodeNum = -1;
                DevWriteBlock(direct_block.block_number[0], (char*)&directory_block);
            }

            else if(j == 4)//다이렉트 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);

                for (int k = 0 ; k < 16; k ++)//16개 모두 확인을 한다음에
                {
                    if(direct_block.block_number[k] != -1)
                    {
                        int dirnumber;
                        DevReadBlock(direct_block.block_number[k], (char *)directory_block);
                        directory_number = direct_block.block_number[k];
                        for (dirnumber = 0; dirnumber < 4; dirnumber++) {
                            if(strcmp(name, directory_block[dirnumber].name) == 0)
                                return -1;

                            if (directory_block[dirnumber].inodeNum == -1) {
                                need_allocate_dir_number = dirnumber;
                                break;
                            }
                        }
                        if (dirnumber != 4)
                            break;
                    }
                    else
                    {
                        int temp_block_number = GetFreeBlockNum();
                        direct_block.block_number[k] = temp_block_number;
                        SetBlockBitmap(temp_block_number);
                        DevWriteBlock(temp_inode.indirBlockPtr, (char*)&direct_block);

                        FileSysInfo info;
                        DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
                        info.numAllocBlocks++;
                        info.numFreeBlocks--;
                        info.numAllocInodes++;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);

                        directory_number = direct_block.block_number[k];
                        DevReadBlock(direct_block.block_number[k], (char*)&directory_block);
                        for(int j = 0 ; j < 4 ; j ++)
                        {
                            directory_block[j].inodeNum = -1;
                        }
                        DevWriteBlock(direct_block.block_number[k], (char*)&directory_block);
                        need_allocate_dir_number = 0;
                        break;
                    }
                }
            }

        }
        else//최종 블록을 찾아가는 단계
        {
            int j = 0;
            for (j = 0; j < 4; j++)//첫번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
                if (strcmp(name, directory_block[j].name) == 0) {
                    find_inode = directory_block[j].inodeNum;
                    break;
                }
            }

            if(j==4)//두번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for(j=0 ; j < 4 ; j ++)
                {
                    if(strcmp(name, directory_block[j].name) == 0)
                    {
                        find_inode = directory_block[j].inodeNum;
                        break;
                    }
                }
            }

            if(j == 4)//다이렉트 블록 체크
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
                for ( int k = 0; k < 16; k++)//16개 모두 확인을 한다음에
                {
                    DevReadBlock(direct_block.block_number[k], (char*)directory_block);
                    for (j = 0; j < 4; j++) {
                        if (strcmp(name, directory_block[j].name) == 0)
                        {
                            find_inode = directory_block[j].inodeNum;
                            break;
                        }
                    }
                    if (j != 4)
                        break;
                }
            }
            name = strtok(NULL, "/");
        }
    }//여기까지 작업을 끝마치면 need_allocate_block에는 할당할수 있는 directory의 위치 값이 그리고 directoryblock에는
    //현재 저장해야하는 directory 블록이 저장되어있다. directory_number 현재의 directory 저장할 위치가 있다.
//
    Inode last_inode;
    int free_block = GetFreeBlockNum();
    
    FileSysInfo info;
    DevReadBlock(FILESYS_INFO_BLOCK, (char*)&info);
    info.numAllocBlocks++;
    info.numFreeBlocks--;
    info.numAllocInodes++;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char*)&info);
    SetBlockBitmap(free_block);

    int free_inode_block = GetFreeInodeNum();//inode를 배치해야 합니다.
    SetInodeBitmap(free_inode_block);
    DevReadBlock(directory_number, (char*)directory_block);
    strcpy(directory_block[need_allocate_dir_number].name, name);
    directory_block[need_allocate_dir_number].inodeNum = free_inode_block;
    DevWriteBlock(directory_number, (char*)directory_block);

    Inode new_inode;
    new_inode.type = FILE_TYPE_DIR;
    new_inode.size = 0;
    new_inode.dirBlockPtr[0] = free_block;
    new_inode.dirBlockPtr[1] = -1;
    new_inode.indirBlockPtr = -1;
    PutInode(free_inode_block, &new_inode);

    DevReadBlock(free_block, (char*)directory_block);
    directory_block[1].inodeNum = find_inode;
    strcpy(directory_block[1].name, "..");
    directory_block[0].inodeNum = free_inode_block;
    strcpy(directory_block[0].name, ".");
    directory_block[2].inodeNum = -1;
    directory_block[3].inodeNum = -1;
    DevWriteBlock(free_block, (char*)directory_block);

    return 0;
}


int		RemoveDir(const char* szDirName)
{
    char* name;

    char temp[64];
    int count=0;
    int find_inode = 0;
    int inode_number = 0;
    int need_remove_file_number;
    int directory_number;
    char temp_block[BLOCK_SIZE];

    Inode temp_inode;
    GetInode(inode_number, &temp_inode);

    DirEntry directory_block[4];
    Single_direct_block direct_block;

    strcpy(temp,szDirName);
    name = strtok(temp, "/");

    while(name != NULL)
    {
        name = strtok(NULL, "/");
        count++;
    }

    strcpy(temp,szDirName);
    name = strtok(temp, "/");

    for(int i = 0 ; i < count ; i ++)
    {
        GetInode(find_inode, &temp_inode);
        DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);

        if(i == count-1)//다찾았고 블록을 할당하는 단계
        {
            int j = 0;
            DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
            directory_number = temp_inode.dirBlockPtr[0];
            temp_inode.size += 64;
            PutInode(find_inode, &temp_inode);

            for (j = 0; j < 4; j++)
            {
                if(strcmp(name, directory_block[j].name) == 0)
                {
                    need_remove_file_number = j;
                    break;
                }
            }

            if (j==4 && temp_inode.dirBlockPtr[1] == -1)//두번째 블록이 없을경우 2번째 블록을 할당하고 종료
            {
                return -1;
            }

            if(j==4)//두번째 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for (j = 0; j < 4; j++)
                {
                    directory_number = temp_inode.dirBlockPtr[1];
                    if(strcmp(name, directory_block[j].name) == 0)
                    {
                        need_remove_file_number = j;
                        break;
                    }
                }
            }

            if(j == 4 && temp_inode.indirBlockPtr == -1)//찾지 못했는데 directBlock이 없을경우
            {
                return -1;
            }

            else if(j == 4)//다이렉트 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);

                for (int k = 0 ; k < 16; k ++)//16개 모두 확인을 한다음에
                {
                    if(direct_block.block_number[k] != -1)
                    {
                        int dirnumber;
                        DevReadBlock(direct_block.block_number[k], (char *)directory_block);
                        directory_number = direct_block.block_number[k];
                        for (dirnumber = 0; dirnumber < 4; dirnumber++) {
                            if(strcmp(name, directory_block[dirnumber].name) == 0)
                            {
                                need_remove_file_number = dirnumber;
                                break;
                            }
                        }
                        if (dirnumber != 4)
                            break;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
        else//최종 블록을 찾아가는 단계
        {
            int j = 0;
            for (j = 0; j < 4; j++)//첫번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
                if (strcmp(name, directory_block[j].name) == 0) {
                    find_inode = directory_block[j].inodeNum;
                    break;
                }
            }

            if(j==4)//두번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for(j=0 ; j < 4 ; j ++)
                {
                    if(strcmp(name, directory_block[j].name) == 0)
                    {
                        find_inode = directory_block[j].inodeNum;
                        break;
                    }
                }
            }

            if(j == 4)//다이렉트 블록 체크
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
                for ( int k = 0; k < 16; k++)//16개 모두 확인을 한다음에
                {
                    DevReadBlock(direct_block.block_number[k], (char*)directory_block);
                    for (j = 0; j < 4; j++) {
                        if (strcmp(name, directory_block[j].name) == 0)
                        {
                            find_inode = directory_block[j].inodeNum;
                            break;
                        }
                    }
                    if (j != 4)
                        break;
                }
            }
            name = strtok(NULL, "/");
        }
    }//여기까지 작업을 끝마치면 need_allocate_block에는 할당할수 있는 directory의 위치 값이 그리고 directoryblock에는
    //현재 저장해야하는 directory 블록이 저장되어있다. directory_number 현재의 directory 저장할 위치가 있다.

    int remove_file_inode = directory_block[need_remove_file_number].inodeNum;

    ResetInodeBitmap(remove_file_inode);
    directory_block[need_remove_file_number].inodeNum = -1;
    DevWriteBlock(directory_number, (char*)directory_block);

    return 0;
}


int		EnumerateDirStatus(const char* szDirName, DirEntryInfo* pDirEntry, int dirEntrys )
{
    char* name;

    char temp[64];
    int count=0;
    int find_inode = 0;
    int inode_number = 0;
    int need_remove_file_number;
    int directory_number;
    char temp_block[BLOCK_SIZE];

    Inode temp_inode;
    GetInode(inode_number, &temp_inode);

    DirEntry directory_block[4];
    Single_direct_block direct_block;

    strcpy(temp,szDirName);
    name = strtok(temp, "/");

    while(name != NULL)
    {
        name = strtok(NULL, "/");
        count++;
    }
    count++;

    strcpy(temp,szDirName);
    name = strtok(temp, "/");
    dirEntrys = 0;

    for(int i = 0 ; i < count ; i ++)
    {
        GetInode(find_inode, &temp_inode);
        DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);

        if(i == count-1)//다찾았고 블록을 할당하는 단계
        {
            int j = 0;
            directory_number = temp_inode.dirBlockPtr[0];
            PutInode(find_inode, &temp_inode);

            for (j = 0; j < 4; j++)
            {
                if(directory_block[j].inodeNum != -1)
                {
                    Inode type_inode;
                    GetInode(directory_block[j].inodeNum, &type_inode);
                    pDirEntry[dirEntrys].type = type_inode.type;
                    pDirEntry[dirEntrys].inodeNum = directory_block[j].inodeNum;
                    strcpy(pDirEntry[dirEntrys].name, directory_block[j].name);
                    dirEntrys ++;
                }
            }

            if (j==4 && temp_inode.dirBlockPtr[1] == -1)//두번째 블록이 없을경우 2번째 블록을 할당하고 종료
            {
                return dirEntrys;
            }

            if(j==4)//두번째 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for (j = 0; j < 4; j++)
                {
                    directory_number = temp_inode.dirBlockPtr[1];
                    if(directory_block[j].inodeNum != -1)
                    {
                        Inode type_inode;
                        GetInode(directory_block[j].inodeNum, &type_inode);
                        pDirEntry[dirEntrys].type = type_inode.type;
                        pDirEntry[dirEntrys].inodeNum = directory_block[j].inodeNum;
                        strcpy(pDirEntry[dirEntrys].name, directory_block[j].name);
                        dirEntrys ++;
                    }
                }
            }

            if(temp_inode.indirBlockPtr == -1)//찾지 못했는데 directBlock이 없을경우
            {
                return dirEntrys;
            }

            else if(j == 4)//다이렉트 블록의 빈자리 찾기
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
                DevReadBlock(direct_block.block_number[0], (char*)directory_block);

                for (int k = 0 ; k < 16; k ++)//16개 모두 확인을 한다음에
                {
                    if(direct_block.block_number[k] != -1)
                    {
                        int dirnumber;
                        DevReadBlock(direct_block.block_number[k], (char *)directory_block);

                        for (dirnumber = 0; dirnumber < 4; dirnumber++) {
                            if(directory_block[dirnumber].inodeNum != -1)
                            {
                                Inode type_inode;
                                GetInode(directory_block[dirnumber].inodeNum, &type_inode);
                                pDirEntry[dirEntrys].type = type_inode.type;
                                pDirEntry[dirEntrys].inodeNum = directory_block[dirnumber].inodeNum;
                                strcpy(pDirEntry[dirEntrys].name, directory_block[dirnumber].name);
                                dirEntrys ++;
                            }
                        }
                    }
                    else
                    {
                        return dirEntrys;
                    }
                }
            }
        }
        else//최종 블록을 찾아가는 단계
        {
            int j = 0;
            for (j = 0; j < 4; j++)//첫번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
                if (strcmp(name, directory_block[j].name) == 0) {
                    find_inode = directory_block[j].inodeNum;
                    break;
                }
            }

            if(j==4)//두번째 포인터 체크
            {
                DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
                for(j=0 ; j < 4 ; j ++)
                {
                    if(strcmp(name, directory_block[j].name) == 0)
                    {
                        find_inode = directory_block[j].inodeNum;
                        break;
                    }
                }
            }

            if(j == 4)//다이렉트 블록 체크
            {
                DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
                for ( int k = 0; k < 16; k++)//16개 모두 확인을 한다음에
                {
                    DevReadBlock(direct_block.block_number[k], (char*)directory_block);
                    for (j = 0; j < 4; j++) {
                        if (strcmp(name, directory_block[j].name) == 0)
                        {
                            find_inode = directory_block[j].inodeNum;
                            break;
                        }
                    }
                    if (j != 4)
                        break;
                }
            }
            name = strtok(NULL, "/");
        }
    }//여기까지 작업을 끝마치면 need_allocate_block에는 할당할수 있는 directory의 위치 값이 그리고 directoryblock에는
    //현재 저장해야하는 directory 블록이 저장되어있다. directory_number 현재의 directory 저장할 위치가 있다.
    int dir_count = 0;


    DevReadBlock(temp_inode.dirBlockPtr[0], (char*)directory_block);
    for(int j = 0 ; j < 4 ; j ++)
    {
        if(directory_block[j].inodeNum != -1)
        {
            pDirEntry[dir_count].inodeNum = directory_block[j].inodeNum;
            strcpy(pDirEntry[dir_count].name,directory_block[j].name);
            dir_count++;
        }
    }

    if(temp_inode.dirBlockPtr[1] != -1)
    {
        DevReadBlock(temp_inode.dirBlockPtr[1], (char*)directory_block);
        for(int j = 0 ; j < 4 ; j ++)
        {
            if(directory_block[j].inodeNum != -1)
            {
                pDirEntry[dir_count].inodeNum = directory_block[j].inodeNum;
                strcpy(pDirEntry[dir_count].name,directory_block[j].name);
                dir_count++;
            }
        }
    }

    if(temp_inode.indirBlockPtr != -1)
    {
        DevReadBlock(temp_inode.indirBlockPtr, (char*)&direct_block);
        for(int k = 0 ; k < 16; k ++)
        {
            if(direct_block.block_number[k] != -1)
            {
                DevReadBlock(direct_block.block_number[k], (char*)directory_block);
                for(int j = 0 ; j < 4 ; j ++)
                {
                    if(directory_block[j].inodeNum != -1)
                    {
                        pDirEntry[dir_count].inodeNum = directory_block[j].inodeNum;
                        strcpy(pDirEntry[dir_count].name,directory_block[j].name);
                        dir_count++;
                    }
                }
            }
        }
    }
    return dirEntrys;
}


void FileSysInit(void)
{
    DevCreateDisk();
    char* temp_block = calloc(sizeof(char), BLOCK_SIZE);

    for(int i = 0 ; i < 64; i++)
    {
        DevWriteBlock(i, temp_block);
    }
    free(temp_block);
}

void SetInodeBitmap(int blkno)
{
    char* temp_block = (char*)malloc(BLOCK_SIZE);

    DevReadBlock(INODE_BITMAP_BLOCK_NUM,temp_block);
    set_bit(&temp_block[blkno/8], blkno%8);
    DevWriteBlock(INODE_BITMAP_BLOCK_NUM,temp_block);
    free(temp_block);
}

void ResetInodeBitmap(int blkno)
{
    char* temp_block = (char*)malloc(BLOCK_SIZE);

    DevReadBlock(INODE_BITMAP_BLOCK_NUM,temp_block);
    clear_bit(&temp_block[blkno/8], blkno%8);
    DevWriteBlock(INODE_BITMAP_BLOCK_NUM,temp_block);
    free(temp_block);
}

void SetBlockBitmap(int blkno)
{
    char temp_block[BLOCK_SIZE];

    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, temp_block);
    set_bit(&temp_block[blkno/8], blkno%8);
    DevWriteBlock(BLOCK_BITMAP_BLOCK_NUM, temp_block);
}

void ResetBlockBitmap(int blkno)
{
    char* temp_block = (char*)malloc(BLOCK_SIZE);

    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, temp_block);
    clear_bit(&temp_block[blkno/8], blkno%8);
    DevWriteBlock(BLOCK_BITMAP_BLOCK_NUM, temp_block);
    free(temp_block);
}

void PutInode(int inodeno, Inode* pInode)
{
    Inode temp_inode[4];
    DevReadBlock(inodeno/4 + INODELIST_BLOCK_FIRST,(char*)&temp_inode);
    temp_inode[inodeno%4].size = pInode->size;
    temp_inode[inodeno%4].type = pInode->type;
    temp_inode[inodeno%4].indirBlockPtr = pInode->indirBlockPtr;
    temp_inode[inodeno%4].dirBlockPtr[0] = pInode->dirBlockPtr[0];
    temp_inode[inodeno%4].dirBlockPtr[1] = pInode->dirBlockPtr[1];
    DevWriteBlock(inodeno/4 + INODELIST_BLOCK_FIRST, (char*)&temp_inode);
}

void GetInode(int inodeno, Inode* pInode)
{
    Inode temp_inode[4];
    DevReadBlock(inodeno/4 + INODELIST_BLOCK_FIRST, (char*)&temp_inode);
    pInode->size = temp_inode[inodeno%4].size;
    pInode->type = temp_inode[inodeno%4].type;
    pInode->dirBlockPtr[0] = temp_inode[inodeno%4].dirBlockPtr[0];
    pInode->indirBlockPtr = temp_inode[inodeno%4].indirBlockPtr;
    pInode->dirBlockPtr[1] = temp_inode[inodeno%4].dirBlockPtr[1];

}

int GetFreeInodeNum(void)
{
    char* temp_block = (char*)malloc(BLOCK_SIZE);
    DevReadBlock(INODE_BITMAP_BLOCK_NUM, temp_block);
    int returnValue;
    for(int i = 0 ; i < BLOCK_SIZE*8 ; i ++)
    {
        if(get_bit(&temp_block[i/8], i%8) == 0)
        {
            returnValue = i;
            break;
        }
    }
    free(temp_block);
    return returnValue;
}

int GetFreeBlockNum(void)
{
    char temp_block[BLOCK_SIZE];
    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, temp_block);
    int returnValue = 0;
    for(int i = 0 ; i < BLOCK_SIZE*8 ; i ++)
    {
        if(get_bit(&temp_block[i/8], i%8) == 0)
        {
            returnValue = i;
            break;
        }
    }
    return returnValue;
}
