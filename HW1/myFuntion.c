#include <string.h>
#include <stdio.h>
#include "fs.h"

int findFileDesTable()
{
	for(int i = 0 ; i < DESC_ENTRY_NUM ; i ++)
	{
		if(pFileDescTable->pEntry[i].bUsed ==0)
			return i;
	}
}


