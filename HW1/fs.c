#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "myFuntion.h"
#include "buf.h"
#include "fat.h"
#include "fs.h"


FileDescTable* pFileDescTable = NULL;


void		
FileSysInit(void)
{
	FileSysInfo* fileSys;
	fileSys = (FileSysInfo*)malloc(sizeof(FileSysInfo) * 4);

	fileSys->blocks = 129 * 32;
	fileSys->rootFatEntryNum = 130;
	fileSys->diskCapacity = 129 * 32 * 4;
	fileSys->numAllocBlocks = 1;
	fileSys->numFreeBlocks = FatGetNumOfFreeEntries();
	fileSys->numAllocFiles = 1;
	fileSys->fatTableStart = 1;
	fileSys->dataStart = 130;

	fileSys->numAllocBlocks ++;
	fileSys->numAllocFiles ++;

	BufWrite(0,(char*)fileSys);
	free(fileSys);//~(end)
}


int
OpenFile(const char* szFileName, OpenFlag flag)
{
	char* name;
	int uDir =130;
	int j;
	char temp[64];
	int find;
	int count =0;
	int dirCount = 1;
	int nowDir;
	int allowBlock;
	int start;

	strcpy(temp,szFileName);
	DirEntry* mydir = (DirEntry*)malloc(sizeof(DirEntry) * 4);

	name = strtok(temp, "/");

	while(name != NULL)
	{
		name = strtok(NULL, "/");
		count++;
	}
	strcpy(temp,szFileName);

	name = strtok(temp, "/");
	start = uDir;
	int save;
	if(flag == OPEN_FLAG_CREATE)
	{
		for(int i = 0 ; i < count ; i ++)
		{
			BufRead(uDir, (char*)mydir);
			nowDir = uDir;
			save = uDir;
			find = 0;
			start = uDir;
			dirCount = 1;

			if(i == count-1)
			{
				while(1)
				{
					for( j = 0 ; j < 4; j ++)//check dir//디렉토리
					{
						if(strcmp(mydir[j].name, name) == 0)
						{
							find=1;
							break;
						}
					}

					if(find == 1)//찾았으면
					{
						break;
					}
					else//
					{
						nowDir = FatGetBlockNum(start,dirCount++);//블록도
						if(nowDir != -1)
						{
							BufRead(nowDir, (char*)mydir);//아직 다
							uDir = nowDir;
						}
						else if(nowDir ==-1)
						{
							break;
						}
					}
				}
			}
			else
			{
				while(1)
				{
					for( j = 0 ; j < 4; j ++)//check dir
					{
						if(strcmp(mydir[j].name , name) ==0)
						{
							find=1;
							break;
						}
					}
					if(find == 1)//find
					{
						uDir = mydir[j].startBlockNum;
						break;
					}
					else
					{
						nowDir = FatGetBlockNum(uDir,dirCount++);
						BufRead(nowDir, (char*)mydir);
					}
				}
				name = strtok(NULL, "/");
			}
		}//---------------------------find empty directory
		//찾은 폴더 번호 폴더에 있다
		//start에 저장 하고 name으로 저장

		save =start;
		dirCount =1;
		if(find ==0)
		{
			while(1)
			{
				find =0;
				BufRead(uDir, (char*)mydir);

				for( j = 0 ; j < 4; j ++)//check dir
				{
					if(mydir[j].startBlockNum ==0)
					{
						find=1;
						break;
					}
				}
				if(find==1)
					break;
				else
				{
					nowDir = FatGetBlockNum(start,dirCount++);//블록도
					if(nowDir != -1)
					{
						uDir = nowDir;
					}
					else//블록 하나 추가해서
					{
						nowDir = FatGetFreeEntryNum();
						FatAdd(uDir, nowDir);
						allowBlock++;
						uDir = nowDir;
						DirEntry* mydir3 = (DirEntry*)malloc(sizeof(DirEntry) * 4);

						for(int i = 0  ; i < 4 ; i ++)
							mydir3[i].startBlockNum = 0;
						BufWrite(uDir, (char*)mydir3);
						free(mydir3);
						j=0;
						break;
					}
				}
			}//-dir에는 내가 마지막 dir 그리고 j번째추가를 하면 된다 a.c를.

			BufRead(uDir, (char*)mydir);
			strcpy(mydir[j].name ,name);
			mydir[j].startBlockNum = -1;
			mydir[j].mode = 7777;
			mydir[j].numBlocks = 0;
			mydir[j].filetype = FILE_TYPE_FILE;
			BufWrite(uDir, (char*)mydir);

			free(mydir);

			File* my_File;
			my_File = (File*)malloc(sizeof(File));

			my_File->dirBlkNum = uDir;
			my_File->entryIndex = j;
			my_File->fileOffset = 0;

			int fileNumber = findFileDesTable();

			pFileDescTable->pEntry[fileNumber].bUsed = 1;
			pFileDescTable->pEntry[fileNumber].pFile = my_File;
			return fileNumber;
		}
		else
		{
			File* my_File;
			my_File = (File*)malloc(sizeof(File));

			my_File->dirBlkNum =uDir;
			my_File->entryIndex = j;
			my_File->fileOffset = 0;

			int fileNumber = findFileDesTable();

			pFileDescTable->pEntry[fileNumber].bUsed = 1;
			pFileDescTable->pEntry[fileNumber].pFile = my_File;

			FileSysInfo* fileSys = (FileSysInfo*)malloc(BLOCK_SIZE);
			BufRead(0,(char*)fileSys);
			fileSys->numAllocBlocks += allowBlock;
			fileSys->numFreeBlocks -=allowBlock;
			fileSys->numAllocFiles +=allowBlock;
			BufWrite(0,(char*)fileSys);

			free(fileSys);

			return fileNumber;
		}
	}
	else
	{
		dirCount =1;
		for(int i = 0 ; i < count ; i ++)
		{
			BufRead(uDir, (char*)mydir);
			nowDir = uDir;
			find = 0;
			start = uDir;
			dirCount = 1;

			if(i == count-1)
			{
				while(1)
				{
					for( j = 0 ; j < 4; j ++)//check dir//디렉토리
					{
						if(strcmp(mydir[j].name, name) == 0)
						{
							find=1;
							break;
						}
					}

					if(find == 1)//찾았으면
					{
						break;
					}
					else//
					{
						nowDir = FatGetBlockNum(start,dirCount++);//블록도
						if(nowDir != -1)
						{
							BufRead(nowDir, (char*)mydir);//아직 다
							uDir = nowDir;
						}
						else if(nowDir ==-1)
						{
							break;
						}
					}
				}
			}
			else
			{
				while(1)
				{
					for( j = 0 ; j < 4; j ++)//check dir
					{
						if(strcmp(mydir[j].name , name) ==0)
						{
							find=1;
							break;
						}
					}
					if(find == 1)//find
					{
						uDir = mydir[j].startBlockNum;
						break;
					}
					else
					{
						nowDir = FatGetBlockNum(uDir,dirCount++);
						BufRead(nowDir, (char*)mydir);
					}
				}
				name = strtok(NULL, "/");
			}
		}

		File* my_File;
		my_File = (File*)malloc(sizeof(File));

		my_File->dirBlkNum =uDir;
		my_File->entryIndex = j;
		my_File->fileOffset = 0;

		int fileNumber = findFileDesTable();

		pFileDescTable->pEntry[fileNumber].bUsed = 1;
		pFileDescTable->pEntry[fileNumber].pFile = my_File;

		FileSysInfo* fileSys = (FileSysInfo*)malloc(BLOCK_SIZE);
		BufRead(0,(char*)fileSys);
		fileSys->numAllocBlocks += allowBlock;
		fileSys->numFreeBlocks -=allowBlock;
		fileSys->numAllocFiles +=allowBlock;
		BufWrite(0,(char*)fileSys);

		free(fileSys);

		return fileNumber;
	}
}


int		
WriteFile(int fileDesc, char* pBuffer, int length)
{
	DirEntry* mydir = (DirEntry*)malloc(sizeof(DirEntry) * 4);

	int dirNumber = pFileDescTable->pEntry[fileDesc].pFile->dirBlkNum;
	int dirCount = pFileDescTable->pEntry[fileDesc].pFile->fileOffset/BLOCK_SIZE;

	BufRead(dirNumber, (char*)mydir);
	int startNumber = mydir[pFileDescTable->pEntry[fileDesc].pFile->entryIndex].startBlockNum;
	mydir[pFileDescTable->pEntry[fileDesc].pFile->entryIndex].numBlocks+= (length/129)+1;

	int newDir = FatGetFreeEntryNum();
	int lastDir;
	if(startNumber !=-1)
	{
		int num = dirCount-1;
		if(num<0)
			num =0;
		lastDir= FatGetBlockNum(startNumber, num);
	}
	else
		lastDir = -1;

	int change= 0;
	if(FatGetBlockNum(startNumber,dirCount) == -1)
	{
		FatAdd(lastDir, newDir);
		change =1;
	}
	else
	{
		mydir[pFileDescTable->pEntry[fileDesc].pFile->entryIndex].numBlocks -=1;
	}
	if(startNumber ==-1)
		mydir[pFileDescTable->pEntry[fileDesc].pFile->entryIndex].startBlockNum = newDir;
	BufWrite(dirNumber, (char*)mydir);

	char* data = malloc(128);
	memcpy(data, pBuffer, 128);
	BufWrite(newDir, data);

	pFileDescTable->pEntry[fileDesc].pFile->fileOffset += length;

	if(change ==1)
	{
		FileSysInfo* fileSys = (FileSysInfo*)malloc(BLOCK_SIZE);
		BufRead(0,(char*)fileSys);
		fileSys->numAllocBlocks += length/129 +1;
		fileSys->numFreeBlocks -= length/129 +1;
		fileSys->numAllocFiles += length/129 +1;
		BufWrite(0,(char*)fileSys);

		free(fileSys);
	}
}

int		
ReadFile(int fileDesc, char* pBuffer, int length)
{
	DirEntry* mydir = (DirEntry*)malloc(sizeof(DirEntry) * 4);

	int dirNumber = pFileDescTable->pEntry[fileDesc].pFile->dirBlkNum;
	int dirCount = pFileDescTable->pEntry[fileDesc].pFile->fileOffset/BLOCK_SIZE;

	BufRead(dirNumber, (char*)mydir);
	int startNumber=0 ;
	startNumber = mydir[pFileDescTable->pEntry[fileDesc].pFile->entryIndex].startBlockNum;

	char* data = malloc(BLOCK_SIZE);

	int num = dirCount-1;
	if(num <0)
		num = 0;

	BufRead(FatGetBlockNum(startNumber, num), data);

	memcpy(pBuffer, data, 128);

	pFileDescTable->pEntry[fileDesc].pFile->fileOffset += 128;
}


int		
CloseFile(int fileDesc)
{
	if(pFileDescTable->pEntry[fileDesc].bUsed ==0)
		return -1;
	pFileDescTable->pEntry[fileDesc].bUsed = 0;
	free(pFileDescTable->pEntry[fileDesc].pFile);
	return 0;
}

int		
RemoveFile(const char* szFileName)
{
	char* name;
	int uDir =130;
	int j;
	char temp[64];
	int find;
	int count =0;
	int dirCount = 1;
	int nowDir;
	int allowBlock;
	int start;

	strcpy(temp,szFileName);
	DirEntry* mydir = (DirEntry*)malloc(sizeof(DirEntry) * 4);

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
		BufRead(uDir, (char*)mydir);
		nowDir = uDir;
		find = 0;
		start = uDir;
		dirCount = 1;

		if(i == count-1)
		{
			while(1)
			{
				for( j = 0 ; j < 4; j ++)//check dir//디렉토리
				{
					if(strcmp(mydir[j].name,name) ==0)
					{
						find=1;
						break;
					}
				}

				if(find == 1)//찾았으면
				{
					break;
				}
				else//
				{
					nowDir = FatGetBlockNum(start,dirCount++);//블록도
					if(nowDir != -1)
					{
						BufRead(nowDir, (char*)mydir);//아직 다
						uDir = nowDir;
					}
					else//블록 하나 추가해서
					{
						return -1;
					}
				}
			}
		}
		else
		{
			while(1)
			{
				for( j = 0 ; j < 4; j ++)//check dir
				{
					if(strcmp(mydir[j].name , name) ==0)
					{
						find=1;
						break;
					}
				}
				if(find == 1)//find
				{
					uDir = mydir[j].startBlockNum;
					break;
				}
				else
				{
					nowDir = FatGetBlockNum(uDir,dirCount++);
					BufRead(nowDir, (char*)mydir);
				}
			}
			name = strtok(NULL, "/");
		}
	}// name에 파일 이름이 있고  find값이 1이면 파일이 있는거고 0 이면 없는 상태

	if(find ==0)
		return -1;
	for(int i = 0 ; i < 128 ; i++)
	{
		if(pFileDescTable->pEntry[i].bUsed)
		{
			if(pFileDescTable->pEntry[i].pFile->dirBlkNum ==
					uDir && pFileDescTable->pEntry[i].pFile->entryIndex == j)
				find =0;
		}
	}
	if(find==0)
		return -1;

	mydir = malloc(BLOCK_SIZE);

	BufRead(uDir,(char*)mydir);
	int tempStart = mydir[j].startBlockNum;
	mydir[j].startBlockNum = 0;
	strcpy(mydir[j].name, "");
	int blocknum = mydir[j].numBlocks;
	mydir[j].numBlocks =0;
	BufWrite(uDir,(char*)mydir);

	free(mydir);
	FatRemove(tempStart, tempStart);

	FileSysInfo* fileSys = (FileSysInfo*)malloc(BLOCK_SIZE);
	BufRead(0,(char*)fileSys);
	fileSys->numAllocBlocks -= blocknum;
	fileSys->numFreeBlocks += blocknum;
	fileSys->numAllocFiles -= blocknum;
	BufWrite(0,(char*)fileSys);
	free(fileSys);

	return 0;
}


int	MakeDir(const char* szDirName)
{
	int dirNum = FatGetFreeEntryNum();//빈 엔트리 -> 디렉토리 블록이 들어갈곳
	FatAdd(-1, dirNum);

	char* name;
	int count=0;
	int uDir =130;
	int nowDir;
	int dirCount = 1;
	int allowBlock = 1;
	int j;
	char temp[64];
	int find;
	int start;

	DirEntry* mydir = (DirEntry*)malloc(sizeof(DirEntry) * 4);

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
		BufRead(uDir, (char*)mydir);
		nowDir = uDir;
		find = 0;
		start = uDir;
		dirCount = 1;

		if(i == count-1)
		{
			while(1)
			{
				for( j = 0 ; j < 4; j ++)//check dir//디렉토리
				{
					if(!mydir[j].startBlockNum)
					{
						find=1;
						break;
					}
				}

				if(find == 1)//찾았으면
				{
					break;
				}
				else//
				{
					nowDir = FatGetBlockNum(start,dirCount++);//블록도
					if(nowDir != -1)
					{
						BufRead(nowDir, (char*)mydir);//아직 다
						uDir = nowDir;
					}
					else//블록 하나 추가해서
					{
						nowDir = FatGetFreeEntryNum();
						FatAdd(uDir, nowDir);
						allowBlock++;
						uDir = nowDir;
						DirEntry* mydir3 = (DirEntry*)malloc(sizeof(DirEntry) * 4);

						for(int i = 0  ; i < 4 ; i ++)
							mydir3[i].startBlockNum = 0;
						BufWrite(uDir, (char*)mydir3);
						free(mydir3);
						j=0;
						break;
					}
				}
			}
		}
		else
		{
			while(1)
			{
				for( j = 0 ; j < 4; j ++)//check dir
				{
					if(strcmp(mydir[j].name , name) ==0)
					{
						find=1;
						break;
					}
				}
				if(find == 1)//find
				{
					uDir = mydir[j].startBlockNum;
					break;
				}
				else
				{
					nowDir = FatGetBlockNum(uDir,dirCount++);
					BufRead(nowDir, (char*)mydir);
				}
			}
			name = strtok(NULL, "/");
		}
	}//---------------------------find empty directory

	BufRead(uDir,(char*)mydir);
	strcpy(mydir[j].name, name);
	mydir[j].startBlockNum = dirNum;
	mydir[j].filetype = FILE_TYPE_DIR;
	mydir[j].mode = 7777;
	mydir[j].numBlocks =1;

	BufWrite(uDir,(char*)mydir);

	DirEntry* mydir2 = (DirEntry*)malloc(sizeof(DirEntry) * 4);

	strcpy(mydir2[0].name, ".");
	mydir2[0].startBlockNum = dirNum;
	mydir2[0].filetype = FILE_TYPE_DIR;
	mydir2[0].mode = 7777;
	mydir2[0].numBlocks =1;

	strcpy(mydir2[1].name, "..");
	mydir2[1].startBlockNum = uDir;
	mydir2[1].filetype = FILE_TYPE_DIR;
	mydir2[1].mode = 7777;
	mydir2[1].numBlocks =1;

	for(int k = 3 ; k < 4 ; k++)
		mydir2[k].startBlockNum = 0;
	BufWrite(dirNum, (char*)mydir2);

	free(mydir);
	free(mydir2);

	FileSysInfo* fileSys = (FileSysInfo*)malloc(BLOCK_SIZE);
	BufRead(0,(char*)fileSys);
	fileSys->numAllocBlocks += allowBlock;
	fileSys->numFreeBlocks -=allowBlock;
	fileSys->numAllocFiles +=allowBlock;
	BufWrite(0,(char*)fileSys);

	free(fileSys);

	return 1;
}


int		
RemoveDir(const char* szDirName)
{
	char* name;
	int uDir =130;
	int j;
	char temp[64];
	int find;
	int count =0;
	int nowDir;
	int dirCount=1;
	int start;

	strcpy(temp,szDirName);
	DirEntry* mydir = (DirEntry*)malloc(sizeof(DirEntry) * 4);

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
		BufRead(uDir, (char*)mydir);
		nowDir = uDir;
		find = 0;
		start = uDir;
		dirCount = 1;

		if(i == count-1)
		{
			while(1)
			{
				for( j = 0 ; j < 4; j ++)//check dir//디렉토리
				{
					if(strcmp(mydir[j].name,name) ==0)
					{
						find=1;
						break;
					}
				}

				if(find == 1)//찾았으면
				{
					break;
				}
				else//
				{
					nowDir = FatGetBlockNum(start,dirCount++);//블록도
					if(nowDir != -1)
					{
						BufRead(nowDir, (char*)mydir);//아직 다
						uDir = nowDir;
					}
					else//블록 하나 추가해서
					{
						return -1;
					}
				}
			}
		}
		else
		{
			while(1)
			{
				for( j = 0 ; j < 4; j ++)//check dir
				{
					if(strcmp(mydir[j].name , name) ==0)
					{
						find=1;
						break;
					}
				}
				if(find == 1)//find
				{
					uDir = mydir[j].startBlockNum;
					break;
				}
				else
				{
					nowDir = FatGetBlockNum(uDir,dirCount++);
					BufRead(nowDir, (char*)mydir);
				}
			}
			name = strtok(NULL, "/");
		}
	}

	int tempDir = uDir;
	uDir = mydir[j].startBlockNum;

	BufRead(uDir, (char*)mydir);

	if(mydir[2].startBlockNum !=0)
		return -1;
	if(mydir[3].startBlockNum !=0)
		return -1;

	int par = mydir[1].startBlockNum;
	strcpy(mydir[0].name, "");
	strcpy(mydir[1].name, "");
	mydir[0].startBlockNum=0;
	mydir[1].startBlockNum=0;
	BufWrite(uDir, (char*)mydir);

	BufRead(par, (char*)mydir);
	mydir[j].startBlockNum = 0;
	strcpy(mydir[j].name, "");
	BufWrite(par, (char*)mydir);

	free(mydir);

	int fat_table[32];
	BufRead(uDir/32, (char*)fat_table);
	fat_table[uDir%32] = 0;
	BufWrite(uDir/32, (char*)fat_table);

	FileSysInfo* fileSys = (FileSysInfo*)malloc(BLOCK_SIZE);
	BufRead(0,(char*)fileSys);
	fileSys->numAllocBlocks --;
	fileSys->numFreeBlocks ++;
	fileSys->numAllocFiles --;
	BufWrite(0,(char*)fileSys);

	free(fileSys);

	return 0;
}


int
EnumerateDirStatus(const char* szDirName, DirEntry* pDirEntry, int* pNum)
{
	char* name;
	int uDir =130;
	int j;
	char temp[64];
	int find;
	int count =0;
	int start;
	int nowDir;
	int dirCount;

	strcpy(temp,szDirName);
	DirEntry* mydir = (DirEntry*)malloc(sizeof(DirEntry) * 4);

	name = strtok(temp, "/");

	BufRead(uDir, (char*)mydir);
	while(name != NULL)
	{
		name = strtok(NULL, "/");
		count++;
	}
	strcpy(temp,szDirName);

	name = strtok(temp, "/");
	for(int i = 0 ; i < count ; i ++)
	{
		BufRead(uDir, (char*)mydir);
		nowDir = uDir;
		find = 0;
		start = uDir;
		dirCount = 1;

		if(i == count-1)
		{
			while(1)
			{
				for( j = 0 ; j < 4; j ++)//check dir//디렉토리
				{
					if(strcmp(mydir[j].name , name) ==0)
					{
						find=1;
						break;
					}
				}
				if(find == 1)//찾았으면
				{
					break;
				}
				else//
				{
					nowDir = FatGetBlockNum(start,dirCount++);//블록도
					if(nowDir != -1)
					{
						BufRead(nowDir, (char*)mydir);//아직 다
						uDir = nowDir;
					}
					else//블록 하나 추가해서
					{
						return -1;
					}
				}
			}
		}
		else
		{
			while(1)
			{
				for( j = 0 ; j < 4; j ++)//check dir
				{
					if(strcmp(mydir[j].name , name) ==0)
					{
						find=1;
						break;
					}
				}
				if(find == 1)//find
				{
					uDir = mydir[j].startBlockNum;
					name = strtok(NULL, "/");
					break;
				}
				else
				{
					nowDir = FatGetBlockNum(uDir,dirCount++);
					BufRead(nowDir, (char*)mydir);
				}
			}
		}
	}
	dirCount = 1;
	int startDir = mydir[j].startBlockNum;
	int dirNumber = startDir;
	int i;
	count= 0 ;

	for( i = 0 ; i < *pNum ; )
	{
		BufRead(dirNumber,mydir);

		if(mydir[count%4].startBlockNum != 0)
		{
			pDirEntry[i].filetype = mydir[count%4].filetype;
			pDirEntry[i].mode = mydir[count%4].mode;
			pDirEntry[i].numBlocks = mydir[count%4].numBlocks;
			pDirEntry[i].startBlockNum = mydir[count%4].startBlockNum;
			strcpy(pDirEntry[i].name, mydir[count%4].name);
			i++;
		}
		count ++;
		if(count != 0 && count%4 ==0)
		{
			dirNumber = FatGetBlockNum(startDir, dirCount++);

			if(dirNumber == -1)
				break;
		}
	}
	*pNum = i;
	return i;
}

void Mount(MountType type)
{
	if(type == MT_TYPE_FORMAT)
	{
		BufInit();
		FatInit();

		int emptyEntry = FatGetFreeEntryNum();
		int i;
		FatAdd(-1, emptyEntry);//fat plus
		DirEntry* rootmydir = (DirEntry*)malloc(sizeof(DirEntry) * 4);

		strcpy(rootmydir[0].name , ".");
		rootmydir[0].mode = 7777;
		rootmydir[0].startBlockNum = 130;
		rootmydir[0].filetype = FILE_TYPE_DIR;
		rootmydir[0].numBlocks = 1;

		for(int i= 1 ; i < 4 ; i++)
			rootmydir[i].startBlockNum=0;

		BufWrite(130, (char*)rootmydir);//~(6)

		free(rootmydir);
		FileSysInit();
	}
	else
	{
		BufInit();
		DevOpenDisk();
		FileSysInit();
	}
	pFileDescTable = malloc(8 * 128 * 2);

	for(int i = 0 ; i < DESC_ENTRY_NUM ; i ++)
		pFileDescTable->pEntry[i].bUsed = 0;
	pFileDescTable->numUsedDescEntry =0;

}

void Unmount(void)
{
	BufSync();
	for(int i = 0 ; i < 128 ; i++)
	{
		if(pFileDescTable->pEntry[i].bUsed)
			free(pFileDescTable->pEntry[i].pFile);
	}
	free(pFileDescTable);
	DevCloseDisk();
}

