#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <openssl/bn.h>
#include<openssl/rsa.h>
#include<openssl/pem.h>
#include<openssl/err.h>

#define N "FF807E694D915875B13F47ACDDA61CE11F62E034150F84660BF34026ABAF8C37"//大数
#define E "010001"//公钥
#define D "45AEF3CB207EAD939BBDD87C8B0F0CFCC5A366A5AF2AC5FE1261D7547C625F51"//私钥

char* my_encrypt(char *str, char *path_key);//加密
char* my_decrypt(char *str, char *path_key);//解密
void  myxor(unsigned char* data1, unsigned char* IV, unsigned int n);
void PRINT(unsigned char *encrypt, unsigned int n);
void dump_hex(unsigned char *p, int n, unsigned char *q);

int main()
{
	RSA *r;
	BIGNUM *bne, *bnn, *bnd;
	int cou;
	int ret = 1,flen;
	unsigned long e = 65537;

	unsigned char *ciphertext;
	//要加密的明文
	unsigned  char *data = "01. A quick brown fox jumps over the lazy dog.\n02. A quick brown fox jumps over the lazy dog.\n03. A quick brown fox jumps over the lazy dog.\n";
	unsigned char IV[] = "0123456789ABCDEFDEADBEEFBADBEAD!";
	unsigned  char *encrypt1, *encrypt2, *encrypt3, *encrypt4,*tmpdata;//加密后的数据/解密后的数据/临时指针
	unsigned  char *data1, *data2, *data3, *data4;

	unsigned char* bignum = N;
	unsigned char* prikey = D;

	unsigned char* tail;
	tail = (unsigned char *)malloc(13 * sizeof(char));
	tmpdata = (unsigned char *)malloc(40 * sizeof(char));
	data1 = (unsigned char *)malloc(32 * sizeof(char));
	data2 = (unsigned char *)malloc(32 * sizeof(char));
	data3 = (unsigned char *)malloc(32 * sizeof(char));
	data4 = (unsigned char *)malloc(32 * sizeof(char));


	unsigned  char *newdata = (unsigned char *)malloc(142 * sizeof(char));
	newdata[141] = '\0';
	//构建RSA数据结构
	bne = BN_new();
	bnd = BN_new();
	bnn = BN_new();
	BN_hex2bn(&bne, E);
	BN_hex2bn(&bnd, D);
	BN_hex2bn(&bnn, N);


	r = RSA_new();
	r->e = bne;
	r->d = bnd;
	r->n = bnn;
	r->flags |= RSA_FLAG_NO_BLINDING;

	flen = RSA_size(r);// - 11;
	//printf("%d\n", flen);
	ciphertext = (unsigned char *)malloc(2 * flen + 1);
	encrypt1 = (unsigned char *)malloc(flen);
	encrypt2 = (unsigned char *)malloc(flen);
	encrypt3 = (unsigned char *)malloc(flen);
	encrypt4 = (unsigned char *)malloc(flen);
	//bzero(encrypt1, flen);//memset(encrypt1, 0, flen);
	//bzero(encrypt2, flen);
	//bzero(encrypt3, flen);
	//bzero(encrypt4, flen);
	printf("plaintext=\n%s", data);

	memset(encrypt1, 0, flen);
	memset(encrypt2, 0, flen);
	memset(encrypt3, 0, flen);
	memset(encrypt4, 0, flen);
	memset(tail, 0, 13);

	memcpy(data1, data, 32);
	memcpy(data2, data + 32, 32);
	memcpy(data3, data + 64, 32);
	memcpy(data4, data + 96, 32);
	//printf("%s---%s---%s---%s---%s\n",data, data1, data2, data3, data4);
	myxor(data1, IV, 32);
	ret *= RSA_public_encrypt(flen, data1, encrypt1, r, RSA_NO_PADDING);
	myxor(data2, encrypt1, 32);
	ret *= RSA_public_encrypt(flen, data2, encrypt2, r, RSA_NO_PADDING);
	myxor(data3, encrypt2, 32);
	ret *= RSA_public_encrypt(flen, data3, encrypt3, r, RSA_NO_PADDING);
	myxor(data4, encrypt3, 32);
	ret *= RSA_public_encrypt(flen, data4, encrypt4, r, RSA_NO_PADDING);

	memcpy(tail, encrypt4, 13);
	myxor(encrypt4, data + 128, 13);

	memcpy(tmpdata, encrypt4, 32);
	ret *= RSA_public_encrypt(flen, tmpdata, encrypt4, r, RSA_NO_PADDING);
	printf("Encrypting...\nciphertext=\n");
	PRINT(encrypt1, 32);
	PRINT(encrypt2, 32);
	PRINT(encrypt3, 32);
	PRINT(encrypt4, 32);
	PRINT(tail, 13);
	printf("\nDncrypting...\nplaintext = ");
	memset(data1, 0, flen);
	memset(data2, 0, flen);
	memset(data3, 0, flen);
	memset(data4, 0, flen);
	RSA_private_decrypt(flen, encrypt4, data4, r, RSA_NO_PADDING);
	memcpy(tmpdata, data4, 13);
	memcpy(data4, tail, 13);
	memcpy(tail, tmpdata, 13);

	myxor(tail, data4, 13);

	memset(tmpdata, 0, 32);
	RSA_private_decrypt(flen, data4, tmpdata, r, RSA_NO_PADDING);
	memcpy(data4, tmpdata, 32);
	myxor(data4, encrypt3, 32);

	RSA_private_decrypt(flen, encrypt3, data3, r, RSA_NO_PADDING);
	myxor(data3, encrypt2, 32);

	RSA_private_decrypt(flen, encrypt2, data2, r, RSA_NO_PADDING);
	myxor(data2, encrypt1, 32);

	RSA_private_decrypt(flen, encrypt1, data1, r, RSA_NO_PADDING);
	myxor(data1, IV, 32);

	memcpy(newdata, data1, 32);
	memcpy(newdata + 32, data2, 32);
	memcpy(newdata + 64, data3, 32);
	memcpy(newdata + 96, data4, 32);
	memcpy(newdata + 128, tail, 13);

	printf("\n%s", newdata);

	printf("\nmd5=\n");
	MD5(data, strlen(data), tmpdata);
	for (cou = 0; cou<16; cou++)
		printf("%02X", tmpdata[cou]);

	unsigned char* tmpdata1 = (unsigned char *)malloc(40);
	printf("\nsha-1=\n");
	SHA1(data, strlen(data), tmpdata1);
	for (cou = 0; cou<20; cou++)
		printf("%02X", tmpdata1[cou]);

	memcpy(tmpdata + 16, tmpdata1, 20);
	printf("\nmd5+sha-1=\n");
	for (cou = 0; cou<36; cou++)
		printf("%02X", tmpdata[cou]);

	memset(encrypt1, 0, flen);
	memset(encrypt2, 0, flen);
	memset(data1, 0, flen);
	memset(data2, 0, flen);
	memcpy(data1, tmpdata, 32);
	memcpy(data2, tmpdata + 32, 4);
	RSA_private_encrypt(flen, data1, encrypt1, r, RSA_NO_PADDING);
	memcpy(encrypt2, encrypt1, 4);
	memcpy(encrypt1, data2, 4);
	memcpy(data1, encrypt1, 32);
	memset(encrypt1, 0, flen);
	RSA_private_encrypt(flen, data1, encrypt1, r, RSA_NO_PADDING);
	printf("\nEncrypting...\nsignature =\n");
	PRINT(encrypt1, 32);
	PRINT(encrypt2, 4);

	printf("\nDcrypting...\nplaintext =\n");
	RSA_public_decrypt(flen, encrypt1, data1, r, RSA_NO_PADDING);
	memcpy(data2, data1, 4);
	memcpy(data1, encrypt2, 4);
	memcpy(encrypt1, data1, 32);
	memset(data1, 0, 32);
	RSA_public_decrypt(flen, encrypt1, data1, r, RSA_NO_PADDING);
	PRINT(data1, 32);
	PRINT(data2, 4);
	printf("\nSignature is correct.\n");



	return;





	return;
}



void myxor(unsigned char* data1, unsigned char* IV, unsigned int n)
{
	unsigned int cou;
	for (cou = 0; cou < n; cou++)
	{
		*(data1 + cou) = (*(data1 + cou)) ^ (*(IV + cou));
	}
	return;
}

void PRINT(unsigned char *encrypt, unsigned int n)
{
	char ch;
	unsigned int cou;
	unsigned short A;
	for (cou = 0; cou < n; cou++)
	{
		ch = *(encrypt + cou);
		A = ch >> 4;
		A = A & (0x000f);
		printf("%X", A);
		A = (ch & (0x0f));
		A = A & (0x000f);
		printf("%X", A);
	}
	return;
}

