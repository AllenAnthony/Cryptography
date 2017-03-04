// rsa.cpp : Defines the entry point for the console application.
/* 
This program is written by Black White(iceman@zju.edu.cn), and 
is for teaching purpose. Please do not upload it to anywhere 
outside Zhejiang University.

padding modes:
#define RSA_PKCS1_PADDING	1
#define RSA_SSLV23_PADDING	2
#define RSA_NO_PADDING		3
#define RSA_PKCS1_OAEP_PADDING	4
#define RSA_X931_PADDING	5

RSA_PKCS1_PADDING	至少保留11字节用于填充
RSA_PKCS1_OAEP_PADDING 至少保留42字节用于填充
RSA_public_encrypt()的最后参数表示填充方式, 例如
RSA_PKCS1_PADDING方式，如果要加密"ABCDE"，则填充
后的明文如下：
00,02,8个字节随机数,0,"ABCDE"
如果要加密"AB"，则填充后的明文如下：
00,02,11个字节随机数,0,"AB"
其中填充的各个字节随机数值都≠0。
RSA_public_encrypt()的第1个参数表示明文字节长度。
当采用RSA_PKCS1_PADDING填充方式时，函数的第1个参
数值<=RSA_size(prsa)-11，其中RSA_size(prsa)返回
密钥N的字节长度，对于128位密钥，此函数返回16。
当采用RSA_NO_PADDING填充方式时，函数的第1个参数
值必须=RSA_size(prsa)

keys:
(1) 128-bit key:
#define N "D7AC79A0C3D06DE0777D6A605F302B7D"
#define E "10001"
#define D "D141AC451A9889719F5F8DF67F02A881"
P=F910697533BDEFA1
Q=DDAE0595A4535E5D

(2) 256-bit key:
#define N "FF807E694D915875B13F47ACDDA61CE11F62E034150F84660BF34026ABAF8C37"
#define E "010001"
#define D "45AEF3CB207EAD939BBDD87C8B0F0CFCC5A366A5AF2AC5FE1261D7547C625F51"
P=FFFE554834BB229ADEB990C7F0FAD66B
Q=FF82284F55911F1143F87E44949FDC65

(3) 512-bit key:
#define N "C19A249331DAE13CE5BB21AB8D37F64D8D83F6ADF1A9AFCCAA3D3DAF009B50E11EA929693BF0E5" \
          "ECA43D2AA5597206C46E02A68C5146312EDC8325827B8532ED"
#define E "010001"
#define D "5E8EB3B874CC2BE07B6FF794FB674ED437FF31176A05EFC82D89B5BBE8B6F33BF62BE179C8FF32" \
          "8B2E456BC2CCFA75F58E5C54111E9F1F4BEC75DE32C8818AA1"
p=F526A9FA5F0A5CD0158E44B07CD6E18BE1928FC4CEC3A10FC8BED91079238125
q=CA2B7A221ACDA25F9892547DDB77E0354A7B37EB1E47BF881D15F97082123429

(4) 1024-bit key:
#define N "E5EE313B4290CD3BC83E5A6EA70C9C8E18A71ADD5E01EAAEF642B8599A71AB2547653BE1005EB8" \
          "40E5089EE9138900DC128063BDDF3A3E2BC0F1BAAEB6DCAD0E16B56D178193A6E9F98E26A7B6DA60" \
          "8BB4B073D4A5A43845BB153F007F4A672D6122CFF7EC0833106EE92DB42B43B66D595BAD5B286E9F" \
          "5FB7C8BDE173C65B47"
#define E "010001"
#define D "D47F4650963C6CF08B27D53BDE76F15901BFAE3C57DD3D9F65485447BB4CC1F739FC7D527C9D0D" \
          "7C2C2FC36D74F8712AF28E659FDCE65EFEE0DA86C5618AA1035B54A63B4BB15A916F12BF6A6CEF4F" \
          "66138C0DE34F7FFDFC580C29565042C6C28E876CB68E16D434DD473AF6BEE905A4FE233C6AA86DF7" \
          "DCC0A1216AF9D9BE01"
p=FE72C2337DE0059895FA4C5856F7D0EB13500229E1D42D404584120B67F5AD3D4B6B3E545393EE
68F3809918BEB16CDA4053ADF9B9685EADCCE5AAFA9767B0E1
q=E755282298393BCC7EFB7F29599F49C7C60E1A4BDCB814812807FD4DD81879AB8E8EA424B76782
C98F3D5514A2FA5864A7FE1D028884574DF6A9B7CAA5928927

*/

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")


#define N "D7AC79A0C3D06DE0777D6A605F302B7D" /* 128-bit */
#define E "10001"
#define D "D141AC451A9889719F5F8DF67F02A881" /* 128-bit */
/* P=F910697533BDEFA1, Q=DDAE0595A4535E5D */

void dump_rsa(RSA *prsa)
{
    printf("rsa size=%d bits\n", RSA_size(prsa)*8);
    printf("n=%s\n", BN_bn2hex(prsa->n));
    printf("p=%s\n", BN_bn2hex(prsa->p));
    printf("q=%s\n", BN_bn2hex(prsa->q));
    printf("d=%s\n", BN_bn2hex(prsa->d));
    printf("e=%s\n", BN_bn2hex(prsa->e));
}

void dump_hex(unsigned char *p, int n, unsigned char *q)
{
   int i;
   for(i=0; i<n; i++)
   {
      sprintf((char *)&q[i*2], "%02X", p[i]);
   }
   q[i*2] = '\0';
}

void scan_hex(unsigned char *p, int n, unsigned char *q)
{
   int i;
   for(i=0; i<n; i++)
   {
      sscanf((char *)&p[i*2], "%02X", &q[i]);
   }
}

void generate_rsa_key(int key_bits)
{
   RSA *prsa = RSA_new();
   prsa = RSA_generate_key(key_bits, 0x10001, NULL, NULL); 
                        /* key_bits指定N的位数, 0x10001是公钥e */
   dump_rsa(prsa);
   RSA_free(prsa);
}

int encrypt_decrypt(unsigned char bufin[], unsigned char bufout[], BIGNUM *pn, BIGNUM *pe)
{
   int n;
   BIGNUM *pin, *pout;
   BN_CTX *ctx;
   ctx = BN_CTX_new();
   pin = BN_new();
   pout = BN_new();
   n = BN_num_bytes(pn);
   BN_bin2bn(bufin, n, pin);
   BN_mod_exp(pout, pin, pe, pn, ctx);
   n = BN_bn2bin(pout, bufout);
   BN_CTX_free(ctx);
   BN_free(pin);
   BN_free(pout);
   return n;
}

void do_encrypt_decrypt_myself(void)
{
   unsigned char plaintext[256] = "A Quick BrownFox" /* 128-bit */
                                  "for 256-bit key." /* 256-bit */
                                  "The 512-bit key is being tested."  /* 512-bit */
                                  "A quick brown fox jumps over the"
                                  " lazy dog. Testing 1024-bit key."; /* 1024-bit */

   unsigned char ciphertext[512];
   unsigned char bufin[256];
   unsigned char bufout[256];
   int n;

   BIGNUM *pn, *pe, *pd;
   pn = BN_new();
   pe = BN_new();
   pd = BN_new();
   BN_hex2bn(&pn, N); /* N、E、D是调用RSA_generate_key()产生的 */
   BN_hex2bn(&pe, E);
   BN_hex2bn(&pd, D);
   strncpy((char *)bufin, (char *)plaintext, 16);
   puts("Encrypting...");
   n = encrypt_decrypt(bufin, bufout, pn, pe);
   dump_hex(bufout, n, ciphertext);
   puts((char *)ciphertext);
   puts("Decrypting...");
   memcpy(bufin, bufout, n);
   memset(bufout, 0, sizeof(bufout));
   n = encrypt_decrypt(bufin, bufout, pn, pd);
   bufout[n] = '\0';
   puts((char *)bufout);
   BN_free(pn);
   BN_free(pe);
   BN_free(pd);
}

main()
{
   /*
   generate_rsa_key();
    */
   unsigned char plaintext[256] = "A Quick BrownFox" /* 128-bit */
                                  "for 256-bit key." /* 256-bit */
                                  "The 512-bit key is being tested."  /* 512-bit */
                                  "A quick brown fox jumps over the"
                                  " lazy dog. Testing 1024-bit key."; /* 1024-bit */

   unsigned char ciphertext[512];
   unsigned char bufin[256];
   unsigned char bufout[256];
   int n;

   /* 
   do_encrypt_decrypt_myself(); 
    */

   RSA *prsa;
   BIGNUM *pn, *pe, *pd;
   prsa = RSA_new();
   prsa->flags |= RSA_FLAG_NO_BLINDING; /* 在blinding模式下, 使用私钥加密或解密时都会
                                           使用公钥; 设置flags关闭blinding模式 */
   pn = BN_new();
   pe = BN_new();
   pd = BN_new();
   BN_hex2bn(&pn, N); /* N、E、D是调用RSA_generate_key()产生的 */
   BN_hex2bn(&pe, E);
   BN_hex2bn(&pd, D);

   /*==============公钥加密,私钥解密,NO_PADDING=============*/
   prsa->n = pn;
   prsa->e = pe;
   prsa->d = NULL;
   n = RSA_size(prsa); /* 返回N的字节数 */
   memset(bufin, 0, sizeof(bufin));
   strncpy((char *)bufin, (char *)plaintext, n);
   puts("Encrypting by calling RSA_public_encrypt()...");
   printf("plaintext=%s\n", bufin);
   n = RSA_public_encrypt(n, bufin, bufout, prsa, RSA_NO_PADDING);
   dump_hex(bufout, n, ciphertext);
   printf("ciphertext=%s\n", ciphertext);
   puts("Decrypting by calling RSA_private_decrypt()...");
   prsa->e = NULL;
   prsa->d = pd;
   n = strlen((char *)ciphertext)/2;
   scan_hex(ciphertext, n, bufin);
   memset(bufout, 0, sizeof(bufout));
   n = RSA_private_decrypt(n, bufin, bufout, 
      prsa, RSA_NO_PADDING);
   puts((char *)bufout);
   puts("=================================================");

   /*==============私钥加密,公钥解密,NO_PADDING============*/
   prsa->n = pn;
   prsa->e = NULL;
   prsa->d = pd;
   n = RSA_size(prsa); /* 返回N的字节数 */
   memset(bufin, 0, sizeof(bufin));
   strncpy((char *)bufin, (char *)plaintext, n);
   puts("Encrypting by calling RSA_private_encrypt()...");
   printf("plaintext=%s\n", bufin);
   n = RSA_private_encrypt(n, bufin, bufout, 
      prsa, RSA_NO_PADDING);
   dump_hex(bufout, n, ciphertext);
   printf("ciphertext=%s\n", ciphertext);
   puts("Decrypting by calling RSA_public_decrypt()...");
   prsa->e = pe;
   prsa->d = NULL;
   n = strlen((char *)ciphertext)/2;
   scan_hex(ciphertext, n, bufin);
   memset(bufout, 0, sizeof(bufout));
   n = RSA_public_decrypt(n, bufin, bufout, prsa, RSA_NO_PADDING);
   puts((char *)bufout);
   puts("=================================================");



   /*==============公钥加密,私钥解密,PKCS1_PADDING=============*/
   prsa->n = pn;
   prsa->e = pe;
   prsa->d = NULL;
   n = RSA_size(prsa)-11; /* 明文最大字节数 = RSA_size(prsa)-11 */
   memset(bufin, 0, sizeof(bufin));
   strncpy((char *)bufin, (char *)plaintext, n);
   puts("Encrypting by calling RSA_public_encrypt()...");
   printf("plaintext=%s\n", bufin);
   n = RSA_public_encrypt(n, bufin, bufout, prsa, RSA_PKCS1_PADDING);
   dump_hex(bufout, n, ciphertext);
   printf("ciphertext=%s\n", ciphertext);
   puts("Decrypting by calling RSA_private_decrypt()...");
   prsa->e = NULL;
   prsa->d = pd;
   n = strlen((char *)ciphertext)/2;
   scan_hex(ciphertext, n, bufin);
   memset(bufout, 0, sizeof(bufout));
   n = RSA_private_decrypt(n, bufin, bufout, prsa, RSA_PKCS1_PADDING);
   puts((char *)bufout);
   puts("=================================================");

   /*==============私钥加密,公钥解密,PKCS1_PADDING============*/
   prsa->n = pn;
   prsa->e = NULL;
   prsa->d = pd;
   n = RSA_size(prsa)-11; /* 明文最大字节数 = RSA_size(prsa)-11 */
   memset(bufin, 0, sizeof(bufin));
   strncpy((char *)bufin, (char *)plaintext, n);
   puts("Encrypting by calling RSA_private_encrypt()...");
   printf("plaintext=%s\n", bufin);
   n = RSA_private_encrypt(n, bufin, bufout, prsa, RSA_PKCS1_PADDING);
   dump_hex(bufout, n, ciphertext);
   printf("ciphertext=%s\n", ciphertext);
   puts("Decrypting by calling RSA_public_decrypt()...");
   prsa->e = pe;
   prsa->d = NULL;
   n = strlen((char *)ciphertext)/2;
   scan_hex(ciphertext, n, bufin);
   memset(bufout, 0, sizeof(bufout));
   n = RSA_public_decrypt(n, bufin, bufout, prsa, RSA_PKCS1_PADDING);
   puts((char *)bufout);
   puts("=================================================");

   RSA_free(prsa);

   return 0;
}
