#ifndef __CHANG_LIB_H__
#define __CHANG_LIB_H__

typedef struct single_direct_block{
    int block_number[16];
}Single_direct_block;

void set_bit(char *bf, unsigned char n)//앞에 인자에 block을 넣고 두번째 인자는 몇번째 비트
{
    *bf |= (1 << (7-n));
}

void clear_bit(char *bf, unsigned char n)//앞에 인자 Blcok 두번째 인자 몇번째
{
    *bf &= ~(1 << (7-n));
}

int get_bit(char *bf, unsigned char n)
{
    return (*bf & (1 <<(7-n))) >> (7-n);
}

#endif