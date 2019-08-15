#include <stdio.h>
#include "fat.h"
#include "Disk.h"
#include <stdlib.h>

void FatInit(void)
{
	int i;
	int* fat_table = (int *)calloc( 32, sizeof(int));

	DevCreateDisk();

	for( i = 0 ; i < 129*32 ; i ++)
	{
		BufWrite(i,(char*)fat_table);
	}

	free(fat_table);
}


/* newBlkNum�� �����ϴ� FAT entry�� value�� 0�� �ƴϸ� -1�� ������.
   lastBlkNum�� �����ϴ� FAT entry�� ���� -1�� �ƴϸ� -1�� ������. */
int FatAdd(int lastBlkNum, int newBlkNum)
{
	int buffer_table[32];
	int next_block;

	if(lastBlkNum ==-1)
	{
		BufRead(newBlkNum/32, (char*)buffer_table);
		buffer_table[newBlkNum%32] =-1;
		BufWrite(newBlkNum/32, (char*)buffer_table);
		BufRead(newBlkNum/32, (char*)buffer_table);
		return 0;
	}

	BufRead(newBlkNum/32, (char*)buffer_table);
	if(buffer_table[newBlkNum%32] != 0)
		return -1;

	BufRead(lastBlkNum/32, (char*)buffer_table);
	if(buffer_table[lastBlkNum%32] == 0)
		return -1;

	BufRead(lastBlkNum/32, (char*)buffer_table);

	buffer_table[lastBlkNum%32] = newBlkNum;
	BufWrite(lastBlkNum/32, (char*)buffer_table);
	BufRead(newBlkNum/32, (char*)buffer_table);
	buffer_table[newBlkNum%32] = -1;
	BufWrite(newBlkNum/32, (char*)buffer_table);
	return 0;
}

/* firstBlkNum�� �����ϴ� FAT entry�� value�� 0�̰ų�
   logicalBlkNum�� �����ϴ� physical block ��ȣ�� -1�̰ų� 0�� ���, -1�� ������ */
int FatGetBlockNum(int firstBlkNum, int logicalBlkNum)
{
	int buffer_table[32];
	int next_block;
	int logical_count = 0;

	BufRead(firstBlkNum/32, (char*)buffer_table);

	if(buffer_table[firstBlkNum%32] == 0)
		return -1;

	if(logicalBlkNum == 0)
	{
		return firstBlkNum;
	}
	else
	{
		next_block = buffer_table[firstBlkNum%32];
		for(logical_count = 1 ; logical_count < logicalBlkNum ; logical_count ++)
		{
			BufRead(next_block/32, (char*)buffer_table);
			next_block = buffer_table[next_block%32];
		}

		if(next_block == -1 || next_block == 0)
			return -1;
		return next_block;
	}
}

/* firstBlkNum�� �����ϴ� FAT entry�� value�� 0�̰ų�
   startBlkNm�� �����ϴ� FAT entry�� value�� 0�� ���, -1�� ������.*/

int FatRemove(int firstBlkNum, int startBlkNum)
{
	int buffer_table[32];
	BufRead(firstBlkNum/32, (char*)buffer_table);
	int temp = firstBlkNum;
	int next_block = buffer_table[firstBlkNum%32];
	int block_count=1;

	if(buffer_table[firstBlkNum%32] == 0)
	{
		return -1;
	}

	if(next_block == startBlkNum)
	{
		temp = firstBlkNum;
	}
	else if(firstBlkNum != startBlkNum)
	{
		while(1)
		{
			BufRead(next_block/32, (char*)buffer_table);
			temp = next_block;
			next_block = buffer_table[next_block%32];
			printf("next_blick is %d\n", next_block);
			if(next_block == startBlkNum)
				break;
		}
	}

	if(firstBlkNum != startBlkNum)
	{
		BufRead(temp/32, (char*)buffer_table);
		buffer_table[temp%32] = -1;
		BufWrite(temp/32, (char*)buffer_table);
	}
	else
	{
		BufRead(temp/32, (char*)buffer_table);
		buffer_table[temp%32] = 0;
		BufWrite(temp/32, (char*)buffer_table);
	}

	while(next_block != -1)
	{
		block_count++;
		BufRead(next_block/32, (char*)buffer_table);
		temp = next_block;
		next_block = buffer_table[next_block%32];
		buffer_table[temp%32] = 0;
		BufWrite(temp/32, (char*)buffer_table);
	}

	return block_count;
}

int FatGetFreeEntryNum(void)
{
	int buffer_table[32];
	int i;
	int find = 0;
	int block_number=1;

	BufRead(block_number, (char*)buffer_table);

	while(1)
	{
		for(i = 0 ; i < 32 ; i++)
		{
			if(buffer_table[i] == 0 && block_number *32 + i >129)
			{
				find = 1;
				break;
			}
		}
		if(find ==1)
			break;
		else
		{
			block_number ++;
			BufRead(block_number, (char*)buffer_table);
		}
	}

	return (block_number *32 + i);
}

int FatGetNumOfFreeEntries(void)
{
	int buffer_table[32];
	int i;
	int find = 0;
	int block_number=1;

	for( block_number = 1; block_number <128 ; )
	{
		BufRead(block_number, (char*)buffer_table);
		for(i = 0 ; i < 32 ; i++)
		{
			if(buffer_table[i] == 0 && (block_number *32 + i) >129)
				find ++;
		}
		block_number++;
	}

	return find;
}
