#include <stdio.h>
#include <stdlib.h>
#include "buf.h"
#include "queue.h"
#include "Disk.h"
#include "myFuntion.h"
#include <string.h>

int bufCount;

void BufInit(void)
{
	TAILQ_INIT(&pBufList);
	TAILQ_INIT(&pLruListHead);
	TAILQ_INIT(&ppStateListHead[BUF_LIST_CLEAN]);
	TAILQ_INIT(&ppStateListHead[BUF_LIST_DIRTY]);
	bufCount =0;
}

void BufReplaceMent()
{
	Buf* remove_item;

	remove_item = TAILQ_FIRST(&pLruListHead);
	bufCount --;
	TAILQ_REMOVE(&pBufList, remove_item, blist);
	TAILQ_REMOVE(&pLruListHead, remove_item, llist);

	if(remove_item->state == BUF_STATE_CLEAN)
	{
		TAILQ_REMOVE(&ppStateListHead[BUF_LIST_CLEAN], remove_item, slist);
	}
	else
	{
		DevWriteBlock(remove_item->blkno, (char*)remove_item->pMem);
		TAILQ_REMOVE(&ppStateListHead[BUF_LIST_DIRTY], remove_item, slist);
	}

	free(remove_item);
}

void BufRead(int blkno, char* pData)
{
	Buf *item, *temp_item;
	int bufTrue = 0;

	for(item = TAILQ_FIRST(&pBufList); item!=NULL ; item = temp_item)
	{
		temp_item = TAILQ_NEXT(item, blist);
		if( item->blkno == blkno)
		{
			bufTrue = 1;
			break;
		}
	}

	if(bufTrue == 1)//Its in the buffer
	{
		memcpy(pData, (char*)item->pMem,BLOCK_SIZE);
		for(item = TAILQ_FIRST(&pLruListHead); item!=NULL ; item = temp_item)
		{
			temp_item = TAILQ_NEXT(item, llist);
			if( item->blkno == blkno)
			{
				TAILQ_REMOVE(&pLruListHead, item, llist);
				break;
			}
		}
		TAILQ_INSERT_TAIL(&pLruListHead, item, llist);
	}
	else// is not
	{
		item = (Buf*)malloc(sizeof(Buf));
		if(bufCount == MAX_BUF_NUM )// buffer max
		{
			BufReplaceMent();
		}
		item->blkno = blkno;
		item->pMem = (char*)malloc(BLOCK_SIZE);
		DevReadBlock(blkno, item->pMem);
		memcpy(pData, item->pMem, BLOCK_SIZE);
		item->state = BUF_STATE_CLEAN;
		TAILQ_INSERT_HEAD(&pBufList, item, blist);
		TAILQ_INSERT_TAIL(&pLruListHead, item, llist);
		TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_CLEAN], item, slist);
		bufCount++;
	}
}


void BufWrite(int blkno, char* pData)
{
	Buf *item, *temp_item;
	int bufTrue = 0;

	for(item = TAILQ_FIRST(&pBufList); item!=NULL ; item = temp_item)
	{
		temp_item = TAILQ_NEXT(item, blist);
		if( item->blkno == blkno)
		{
			bufTrue = 1;
			break;
		}
	}

	if(bufTrue == 1)//Its in the buffer
	{
		memcpy((char*)item->pMem,pData, BLOCK_SIZE);

		if(item->state == BUF_STATE_CLEAN)
		{
			item->state = BUF_STATE_DIRTY;
			TAILQ_REMOVE(&ppStateListHead[BUF_LIST_CLEAN], item, slist);
			TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_DIRTY], item, slist);
		}
		TAILQ_REMOVE(&pLruListHead, item, llist);
		TAILQ_INSERT_TAIL(&pLruListHead, item, llist);
	}
	else
	{
		item = (Buf*)malloc(sizeof(Buf));
		if(bufCount == MAX_BUF_NUM )// buffer max
		{
			BufReplaceMent();
		}
		item->blkno =blkno;
		item->pMem= (char*)malloc(BLOCK_SIZE);
		memcpy((char *)item->pMem, pData, BLOCK_SIZE);
		item->state =BUF_STATE_DIRTY;
		TAILQ_INSERT_HEAD(&pBufList, item, blist);
		TAILQ_INSERT_TAIL(&pLruListHead, item, llist);
		TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_DIRTY], item, slist);
		bufCount++;
	}
}

void BufSync(void)
{
	Buf* item, *temp_item;

	for(item = TAILQ_FIRST(&ppStateListHead[BUF_LIST_DIRTY]); item!=NULL ; )
	{
		temp_item = TAILQ_NEXT(item,slist);
		TAILQ_REMOVE(&ppStateListHead[BUF_LIST_DIRTY], item, slist);
		TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_CLEAN], item, slist);
		item->state = BUF_STATE_CLEAN;
		DevWriteBlock( item->blkno, (char*)item->pMem);
		item = temp_item;
	}
}

/*
 * GetBufInfoByListNum: Get all buffers in a list specified by listnum.
 *                      This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
void GetBufInfoByListNum(StateList listnum, Buf** ppBufInfo, int* pNumBuf)
{
	Buf* item, *temp_item;
	int i;
	*pNumBuf = 0;
	for(item = TAILQ_FIRST(&ppStateListHead[listnum]); item!=NULL ; item = temp_item)
	{
		temp_item = TAILQ_NEXT(item, slist);
		(*pNumBuf)++;
	}
	for(i = 0, item = TAILQ_FIRST(&ppStateListHead[listnum]) ; i < (*pNumBuf) ; i++)
	{
		temp_item = TAILQ_NEXT(item, slist);
		ppBufInfo[i] = item;
		item = temp_item;
	}
}

/*
 * GetBufInfoInLruList: Get all buffers in a list specified at the LRU list.
 *                         This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
void GetBufInfoInLruList(Buf** ppBufInfo, int* pNumBuf)
{
	Buf* item, *temp_item;
	int i;
	*pNumBuf = 0;

	for(item = TAILQ_FIRST(&pLruListHead); item!=NULL ; item = temp_item)
	{
		temp_item = TAILQ_NEXT(item, llist);
		(*pNumBuf)++;
	}
	for(i = 0, item = TAILQ_FIRST(&pLruListHead) ; i < (*pNumBuf) ; i++)
	{
		temp_item = TAILQ_NEXT(item, slist);
		ppBufInfo[i] = item;
		item = temp_item;
	}
}

void GetBufInfoInBufferList(Buf** ppBufInfo, int* pNumBuf)
{
	Buf* item, *temp_item;
	int i;
	*pNumBuf = bufCount;

	for(i = 0, item = TAILQ_FIRST(&pBufList) ; i < (*pNumBuf) ; i++)
	{
		temp_item = TAILQ_NEXT(item, blist);
		ppBufInfo[i] = item;
		item = temp_item;
	}
}

