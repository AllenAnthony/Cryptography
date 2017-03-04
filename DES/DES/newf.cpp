#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<vector>
#include<iostream>
#include<string>
using namespace std;
#if 2147483647L+1L == -2147483648L 
typedef long long32;
typedef unsigned long ulong32;
#else
typedef int long32;           /* In 64-bit systems, long may be 64-bit, */
typedef unsigned int ulong32; /* here we force it to be 32-bit. */
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif

int  des_init(int mode);
void des_set_key(char *key);
void des_encrypt(char *block);
void des_decrypt(char *block);
void des_done(void);
static void sbox_output_perm_table_init(void);
static void perm_init(char perm[16][16][8], char p[64]);
static void permute(char *inblock, char perm[16][16][8], char *outblock);
static void forward_round(int num, ulong32 *block);
static void reverse_round(int num, ulong32 *block);
static long32 f(ulong32 r, unsigned char subkey[8]);

#ifdef LITTLE_ENDIAN
static ulong32 byteswap(ulong32 x);
#endif

/* The (in)famous S-boxes */
static char sbox[8][64] =
{
	/* S1 */
	14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7,
	0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8,
	4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0,
	15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13,

	/* S2 */
	15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10,
	3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5,
	0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15,
	13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9,

	/* S3 */
	10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8,
	13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1,
	13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7,
	1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12,

	/* S4 */
	7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15,
	13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9,
	10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4,
	3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14,

	/* S5 */
	2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9,
	14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6,
	4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14,
	11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3,

	/* S6 */
	12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11,
	10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8,
	9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6,
	4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13,

	/* S7 */
	4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1,
	13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6,
	1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2,
	6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12,

	/* S8 */
	13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7,
	1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2,
	7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8,
	2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11
};

static char plaintext_32bit_expanded_to_48bit_table[48] =
{
	32, 1, 2, 3, 4, 5, /* [%] source Bit32->target Bit1 */
	4, 5, 6, 7, 8, 9,
	8, 9, 10, 11, 12, 13,
	12, 13, 14, 15, 16, 17,
	16, 17, 18, 19, 20, 21,
	20, 21, 22, 23, 24, 25,
	24, 25, 26, 27, 28, 29,
	28, 29, 30, 31, 32, 1
};

/* 32-bit permutation function P used on the output of the S-boxes */
static char sbox_perm_table[32] =
{
	16, 7, 20, 21,
	29, 12, 28, 17,
	1, 15, 23, 26,
	5, 18, 31, 10,
	2, 8, 24, 14,
	32, 27, 3, 9,
	19, 13, 30, 6,
	22, 11, 4, 25
};

string write(unsigned long v)
{
	int cou;
	int bit;
	string B;
	int size = sizeof(v)* 8;//这里的size是输入参数v的位数
	for (cou = 0; cou < size; cou++)
	{
		bit = v&(1 << (size - cou - 1));//提取出第i位的值
		if (bit == 0) B += '0';
		else B += '1';
	}
	return B;
}

static long32 f(ulong32 r, unsigned char subkey[8])//这个函数实现了R32的扩展排列~48位扩展结果与48位密钥异或~通过8个Sbox后还原成32位
{
	register ulong32 res, P;
	unsigned char ch;
	unsigned char row;
	unsigned char col;

    unsigned char plaintext[8];
	int sub, cou;
	res = 0;
	P = 0;

	memset(plaintext, 0, 8);
	for (sub = 0; sub <= 7; sub++)//根据plaintext_32bit_expanded_to_48bit_table这张表, 把r扩展成48位并保存到以下数组内:unsigned char plaintext[8]
	{
		for (cou = 0; cou <= 5; cou++)
		{
			ch = (r >> (32 - plaintext_32bit_expanded_to_48bit_table[6 * sub + cou])) & 0x01;
			plaintext[sub] |= ch << (5 - cou);
		}
	}

	for (cou = 0; cou <= 7; cou++)//把plaintext中的各个元素与subkey中各个元素异或, 异或后的值保存到plaintext内
	{
		plaintext[cou] ^= *(subkey + cou);
	}


	for (cou = 0; cou <= 7; cou++)//取出plaintext[i]查表sbox[i]得4位数, 循环8次可得8个4位数, 按从左到右顺序合并得32位数,其中查sbox的过程必须先把plaintext[i]拆成2位行号4位列号再去查表得4位结果。
	{
		row = 2 * ((plaintext[cou] >> 5) & 0x01) + (plaintext[cou] & 0x01);
		col = (plaintext[cou]>>1)&0x0F;
		ch = sbox[cou][row * 16 + col];
		P |= ch << (4 * (7 - cou));
	}

	for (cou = 0; cou <= 31; cou++)//根据sbox_perm_table把步骤(3)所得32位数打乱得返回值rval, 该过程需要使用32次循环来做,每次循环提取1位。
	{
		res |= (P >> (32-sbox_perm_table[cou])) & 0x00000001;
		if (cou<=30)
		   res=res << 1;
	}

	return res;
}

int main()
{
	unsigned char subkey[8];
	memset(subkey, 0, 8);
	static ulong32 res;
	ulong32 MIN = 0x00ff8066;
	res = f(MIN, subkey);
	string A = write(res);
	cout << A << endl;
	A = write(0x6bde48b3);
	cout << A << endl;
}