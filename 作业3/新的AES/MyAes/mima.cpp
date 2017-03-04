#include <stdio.h>
#include <string.h>

#if 2147483647L+1L == -2147483648L
typedef unsigned long ulong32;
#else
typedef unsigned int ulong32;
#endif


/*
���º�����Դ������ɾ��, ��Щ��������������ɵĻ�������ת�Ƶ�aes.lib��:
void aes_polynomial_mul(unsigned char x[4], unsigned char y[4], unsigned char z[4]);
void ByteSubInverse(unsigned char *p, int n);
void ShiftRowInverse(unsigned char *p);
void MixColumnInverse(unsigned char *p, unsigned char a[4], int do_mul);
void aes_decrypt(unsigned char *bufin, unsigned char *bufout, unsigned char *key);
��Ա�����(MyAes.c)�������޸�:
(1) ��д��������
(2) ɾ��#pragma comment(lib, "aes.lib")
Ҫ��:
(1) ��sbox��, �����в����Բ����������������
(2) �޸ĺ�ĳ��������������������
(3) ���в������������뱾����һ��
*/

unsigned int aes_8bit_mul_mod_0x101(unsigned int x, unsigned int y);
unsigned int aes_8bit_mul_mod_0x11B(unsigned int x, unsigned int y);
unsigned int aes_8bit_inverse(unsigned int x);
void aes_polynomial_mul(unsigned char x[4], unsigned char y[4], unsigned char z[4]);
unsigned int get_msb_mask(unsigned int x);
void rol_a_row(unsigned char *p, int n);
void shr_a_row(unsigned char *p, int n);
void ror_a_row(unsigned char *p, int n);
void get_column(unsigned char *p, int j, int r, unsigned char *q);
void put_column(unsigned char *p, unsigned char *q, int j, int r);

void build_sbox(void);
void build_sbox_inverse(void);
void ByteSub(unsigned char *p, int n);
void ByteSubInverse(unsigned char *p, int n);
void ShiftRow(unsigned char *p);
void ShiftRowInverse(unsigned char *p);
void MixColumn(unsigned char *p, unsigned char a[4], int do_mul);
void MixColumnInverse(unsigned char *p, unsigned char a[4], int do_mul);
void AddRoundKey(unsigned char *p, unsigned char *k);
void aes_init(void);
int  aes_set_key(unsigned char *seed_key, int bits, unsigned char *key);
void aes_encrypt(unsigned char *bufin, unsigned char *bufout, unsigned char *key);
void aes_decrypt(unsigned char *bufin, unsigned char *bufout, unsigned char *key);

unsigned char sbox[256];
unsigned char sbox_inverse[256];
int key_rounds = 0;

/*2 3 1 1
   1 2 3 1
   1 1 2 3
   3 1 1 2*/
void aes_polynomial_mul(unsigned char x[4], unsigned char y[4], unsigned char z[4])//aes_8bit_mul_mod_0x11B(unsigned int x, unsigned int y)
{
	int cou;
	for (cou = 0; cou < 4; cou++)
	{
		z[cou] = aes_8bit_mul_mod_0x11B(y[0], x[(3 - cou) % 4]) ^ aes_8bit_mul_mod_0x11B(y[1], x[(4 - cou) % 4]) ^ aes_8bit_mul_mod_0x11B(y[2], x[(5 - cou) % 4]) ^ aes_8bit_mul_mod_0x11B(y[3], x[(6 - cou) % 4]);
	}
}

void ByteSubInverse(unsigned char *p, int n)
{
	/* ��p��ָ���n�ֽ��滻��sbox�е�ֵ */
	int i;
	for (i = 0; i<n; i++)
	{
		p[i] = sbox_inverse[p[i]];
	}
}


void ShiftRowInverse(unsigned char *p)
{
	/* ��p��ָ���4*4����������ѭ�����Ʋ���:
	��0��: ���ƶ�;
	��1��: ����1�ֽ�
	��2��: ����2�ֽ�
	��3��: ����3�ֽ�
	*/

	int i;
	for (i = 1; i<4; i++)
	{
		ror_a_row(p + i * 4, i);
	}
}

void MixColumnInverse(unsigned char *p, unsigned char a[4], int do_mul)
{
	int  j;
	unsigned char T[4][4];
	for (j = 0; j<4; j++)
	{
		if (do_mul) /* �ڼ������һ���Լ����ܵ�һ�ֵ�MixColumn�����в���Ҫ���˷�; */
			aes_polynomial_mul(a, p + j * 4, p + j * 4); /* �����ֶ�Ҫ���˷�: b = a*b mod (X^4+1); */
	}
	T[0][0] = *(p + 0 * 4 + 0);
	T[1][0] = *(p + 0 * 4 + 1);
	T[2][0] = *(p + 0 * 4 + 2);
	T[3][0] = *(p + 0 * 4 + 3);

	T[1][3] = *(p + 1 * 4 + 0);
	T[2][3] = *(p + 1 * 4 + 1);
	T[3][3] = *(p + 1 * 4 + 2);
	T[0][3] = *(p + 1 * 4 + 3);

	T[2][2] = *(p + 2 * 4 + 0);
	T[3][2] = *(p + 2 * 4 + 1);
	T[0][2] = *(p + 2 * 4 + 2);
	T[1][2] = *(p + 2 * 4 + 3);

	T[3][1] = *(p + 3 * 4 + 0);
	T[0][1] = *(p + 3 * 4 + 1);
	T[1][1] = *(p + 3 * 4 + 2);
	T[2][1] = *(p + 3 * 4 + 3);

	memcpy(p, T, 4 * 4);
}

void aes_decrypt(unsigned char *bufin, unsigned char *bufout, unsigned char *key)
{
	/* ʹ��key��bufin�а�����16�ֽ����Ľ��н���, ����16�ֽ����Ĵ�ŵ�bufout */
	int i;
	unsigned char a[4] = { 0x0B, 0x0D, 0x09, 0x0E }; 
	unsigned char matrix[4][4];
	memcpy(matrix, bufin, 4 * 4); /* ��������16�ֽڵ�matrix */
	
	for (i = key_rounds; i >=1 ; i--)
	{  /* ��1��key_rounds��, �����²���: ByteSub, ShiftRow, MixColumn, AddRoundKey */
		
		AddRoundKey((unsigned char *)matrix, key + i*(4 * 4));

		
		if (i != key_rounds)
			MixColumnInverse((unsigned char *)matrix, a, 1); /* do mul */
		else
			MixColumnInverse((unsigned char *)matrix, a, 0); /* don't mul */

		ShiftRowInverse((unsigned char *)matrix);

		ByteSubInverse((unsigned char *)matrix, 16);
	}

	AddRoundKey((unsigned char *)matrix, key); /* ��0��ֻ��AddRoundKey() */
	memcpy(bufout, matrix, 4 * 4); /* ���ĸ��Ƶ�bufout */
}


unsigned int aes_8bit_mul_mod_0x101(unsigned int x, unsigned int y)
{
	/* ����ũ���㷨�� x * y mod (X^8 + 1); */
	/*      8         0
	X         X
	n = 1 0000 0001 = 0x101
	*/
	unsigned int p = 0; /* the product of the multiplication */
	int i;
	for (i = 0; i < 8; i++)
	{
		if (y & 1) /* if y is odd, then add the corresponding y to p */
			p ^= x; /* since we're in GF(2^m), addition is an XOR */
		y >>= 1;   /* equivalent to y/2 */
		x <<= 1;   /* equivalent to x*2 */
		if (x & 0x100) /* GF modulo: if x >= 256, then apply modular reduction */
			x ^= 0x101; /* XOR with the primitive polynomial x^8 + 1 */
		/* Actually, it's is the same as ROL. Commented by Black White. */
	}
	return p;
}

unsigned int aes_8bit_mul_mod_0x11B(unsigned int x, unsigned int y)
{
	/* ����ũ���㷨�� x * y mod (X^8 + X^4 + X^3 + X + 1);
	����ũ���㷨��ο�:
	(1) https://en.wikipedia.org/wiki/Rijndael_Galois_field
	(2) https://en.wikipedia.org/wiki/Multiplication_algorithm
	*/
	/*
	8    4 3 10
	X    X X XX
	n = 1 0001 1011 = 0x11B
	*/
	unsigned int p = 0; /* the product of the multiplication */
	int i;
	for (i = 0; i < 8; i++)
	{
		if (y & 1) /* if y is odd, then add the corresponding y to p */
			p ^= x; /* since we're in GF(2^m), addition is an XOR */
		y >>= 1;   /* equivalent to y/2 */
		x <<= 1;   /* equivalent to x*2 */
		if (x & 0x100) /* GF modulo: if x >= 256, then apply modular reduction */
			x ^= 0x11B; /* XOR with the primitive polynomial x^8 + x^4 + x^3 + x + 1 */

	}
	return p;
}

unsigned int get_msb_mask(unsigned int x)
{
	/* ����x���λ(���5λ)������:
	��x=0x0A, �������λ����=0x08;
	��x=0x1D, �������λ����=0x10;
	aes_8bit_inverse()��Ҫ���ñ�����
	*/
	unsigned int mask = 0x100;
	while (mask != 0 && (mask & x) == 0)
	{
		mask >>= 1;
	}
	return mask;
}

unsigned int aes_8bit_inverse(unsigned int x)
{
	/*      -1      8   4    3
	����x   mod X + X  + X  + X + 1
	����x��������õ�����չŷ������㷨(extended Euclidian algorithm);
	��չŷ������㷨��֤����������ο��̲�p.93��p.94;
	*/
	/*      8    4 3 10
	X    X X XX
	n = 1 0001 1011 = 0x11B
	*/
	unsigned int a1 = 1, b1 = 0, a2 = 0, b2 = 1;
	unsigned int q, r, t, n, nmask, xmask, shift;
	n = 0x11B; /* ��nΪ������ */
	r = x;     /* xΪ���� */
	while (r != 0)
	{
		q = 0;
		nmask = get_msb_mask(n);
		xmask = get_msb_mask(x);
		do
		{
			shift = 0;
			while (xmask < nmask)
			{
				xmask <<= 1;
				shift++;
			}
			if (xmask == nmask)
			{
				q |= 1 << shift; /* qΪ�� */
				n ^= x << shift; /* n = n - (x << shift);
								 ����������GF(2^8)�ļ�������,
								 ����ļ��������Բ�����λ, ������������
								 */
			}
			nmask = get_msb_mask(n);
			xmask = get_msb_mask(x);
		} while (n != 0 && nmask >= xmask);
		r = n;

		t = a1;
		a1 = a2;
		a2 = t ^ aes_8bit_mul_mod_0x11B(q, a2); /* a2 = a1 - q*a2; */
		t = b1;
		b1 = b2;
		b2 = t ^ aes_8bit_mul_mod_0x11B(q, b2); /* b2 = b1 - q*b2; */
		n = x;
		x = r;
	}
	return b1;
}


void build_sbox(void)
{
	/* ����sbox:
	��a��[0,255], ��
	-1                8
	sbox[a] = ((a   *  0x1F) mod (X + 1)) ^ 0x63;

	-1
	����a * a   = 1 mod (X^8+X^4+X^3+X+1);
	*/
	int i;
	for (i = 0; i<256; i++)
	{
		sbox[i] = aes_8bit_mul_mod_0x101(aes_8bit_inverse(i), 0x1F) ^ 0x63;
	}
}

void build_sbox_inverse(void)
{
	/* ����sbox����sbox����� */
	int i, j;
	for (i = 0; i<256; i++)
	{
		for (j = 0; j<256; j++)
		{
			if (sbox[j] == i)
				break;
		}
		sbox_inverse[i] = j;
	}
}

void aes_init(void)
{
	/* ����sbox��sbox����� */
	build_sbox();
	build_sbox_inverse();
}

int aes_set_key(unsigned char *seed_key, int bits, unsigned char *key)
{
	/* ����������Կ����������Կ:
	128bit������Կ(16�ֽ�): ����(1+10)*4��long32, step=4, loop=10
	192bit������Կ(24�ֽ�): ����(1+12)*4��long32, step=6, loop=8
	256bit������Կ(32�ֽ�): ����(1+14)*4��long32, step=8, loop=7
	*/
	int i, j, step, loop;
	ulong32 *pk;
	if (seed_key == NULL || key == NULL)
		return 0;
	if (bits == 128)      /* 16�ֽ�������Կ */
	{
		key_rounds = 10;  /* ���ܻ������Ҫ��10��ѭ�� */
		step = 4;         /* ������Կ��ѭ������Ϊ4��long32 */
		loop = 10;        /* ������Կ��Ҫ��10��ѭ�� */
	}
	else if (bits == 192) /* 24�ֽ�������Կ */
	{
		key_rounds = 12;  /* ���ܻ������Ҫ��12��ѭ�� */
		step = 6;
		loop = 8;
	}
	else if (bits == 256) /* 32�ֽ�������Կ */
	{
		key_rounds = 14;  /* ���ܻ������Ҫ��14��ѭ�� */
		step = 8;
		loop = 7;
	}
	else
	{
		key_rounds = 0;
		step = 0;
		return 0;
	}
	memcpy(key, seed_key, bits / 8);
	pk = (ulong32 *)(key + 4 * step);
	for (i = step; i<step + step*loop; i += step)
	{
		unsigned int r;
		/* �ٶ����ɵ���Կk��long32���͵�����, i�����±�,
		��i!=0 && i%step==0ʱ, k[i]�ڼ���ʱ��������
		�����⴦��:
		*/
		pk[0] = pk[-1];
		rol_a_row(key + i * 4, 1);
		ByteSub(key + i * 4, 4);
		r = 1 << ((i - step) / step);
		if (r <= 0x80)
			r = aes_8bit_mul_mod_0x11B(r, 1);
		else
			r = aes_8bit_mul_mod_0x11B(r / 4, 4);
		key[i * 4] ^= r;
		pk[0] ^= pk[-step];

		for (j = 1; j<step; j++) /* i+j����Կk���±�, ��(i+j)%step != 0ʱ, */
		{                     /* k[i+j]ֻ�����򵥵������ */
			if (step == 6 && i == step*loop && j >= step - 2 ||
				step == 8 && i == step*loop && j >= step - 4)
				break; /* 128-bit key does not need to discard any steps:
					   4 + 4*10 - 0= 4+40 = 44 == 4+4*10
					   192-bit key should discard last 2 steps:
					   6 + 6*8 - 2 = 4+48 = 52 == 4+4*12
					   256-bit key should discard last 4 steps:
					   8 + 8*7 - 4 = 4+56 = 60 == 4+4*14
					   */
			if (step == 8 && j == 4) /* ����256bit��Կ, ��(i+j)%4==0ʱ�������⴦�� */
			{
				ulong32 k;
				k = pk[3];
				ByteSub((unsigned char *)&k, 4); /* k = scrambled pk[3] */
				pk[4] = k ^ pk[4 - 8];
			}
			else /* ��(i+j)%step != 0ʱ, k[i+j]ֻ������������� */
				pk[j] = pk[j - step] ^ pk[j - 1];
		}
		pk += step;
	} /* for(i=step; i<step+step*loop; i+=step) */
	return 1;
}


void AddRoundKey(unsigned char *p, unsigned char *k)
{
	/* ��p��ָ���4*4���ľ�����k��ָ���4*4��Կ�������������:
	p -> 4byte * 4byte matrix
	k -> 4byte * 4byte key
	*/
	ulong32 *pp, *kk;
	int i;
	pp = (ulong32 *)p;
	kk = (ulong32 *)k;
	for (i = 0; i<4; i++)
	{
		pp[i] ^= kk[i];
	}
}

void ByteSub(unsigned char *p, int n)
{
	/* ��p��ָ���n�ֽ��滻��sbox�е�ֵ */
	int i;
	for (i = 0; i<n; i++)
	{
		p[i] = sbox[p[i]];
	}
}



void rol_a_row(unsigned char *p, int n)
{
	/* ��p��ָ���4���ֽ�ѭ������n���ֽ� */
	int i;
	unsigned char t;
	for (i = 0; i<n; i++)
	{
		t = p[0];
		memcpy(p, p + 1, 3);
		p[3] = t;
	}
}

void shr_a_row(unsigned char *p, int n)
{
	/* ��p��ָ���4���ֽ�����n���ֽ� */
	int i, j;
	for (i = 0; i<n; i++)
	{
		for (j = 3; j >= 1; j--)
		{
			p[j] = p[j - 1];
		}
		p[0] = 0;
	}
}

void ror_a_row(unsigned char *p, int n)
{
	/* ��p��ָ���4���ֽ�ѭ������n���ֽ� */
	int i, j;
	unsigned char t;
	for (i = 0; i<n; i++)
	{
		t = p[3];
		for (j = 3; j >= 1; j--)
		{
			p[j] = p[j - 1];
		}
		p[0] = t;
	}
}


void ShiftRow(unsigned char *p)
{
	/* ��p��ָ���4*4����������ѭ�����Ʋ���:
	��0��: ���ƶ�;
	��1��: ����1�ֽ�
	��2��: ����2�ֽ�
	��3��: ����3�ֽ�
	*/
	int i;
	for (i = 1; i<4; i++)
	{
		rol_a_row(p + i * 4, i);
	}
}



void get_column(unsigned char *p, int j, int r, unsigned char *q)
{
	/* ��p��ָ���4*4����m����ȡ��j��, �����4��unsigned char, ���浽q��;
	����rָ����j��4��Ԫ���е���Ԫ�������к�, ����get_column(p, 3, 3, q)
	��ȡ����: m[3][3], m[0][3], m[1][3], m[2][3]
	*/
	int i;
	for (i = 0; i<4; i++)
	{
		q[i] = p[(r + i) % 4 * 4 + j];
	}
}

void put_column(unsigned char *p, unsigned char *q, int j, int r)
{
	/* ��p��ָ��4��unsigned char���浽q��ָ����m�еĵ�j��;
	����rָ����j��4��Ԫ���е���Ԫ�������к�, ����put_column(p, q, 2, 2)
	�������: m[2][2], m[3][2], m[0][2], m[1][2]
	*/
	int i;
	for (i = 0; i<4; i++)
	{
		q[(r + i) % 4 * 4 + j] = p[i];
	}
}

void MixColumn(unsigned char *p, unsigned char a[4], int do_mul)
{
	/* (1) ��pָ���4*4����m�е�4�����˷�����;
	(2) ����ĳ˷���ָ������GF(2^8)����ʽģ(X^4+1)�˷�,���岽����ο��̲�p.61��p.62;
	(3) aes����ʱ���õı�����aΪ����ʽ3*X^3 + X^2 + X + 2, �������ʾΪ
	unsigned char a[4]={0x03, 0x01, 0x01, 0x02};
	(4) aes����ʱ���õı�����aΪ�������ö���ʽ����, ��{0x0B, 0x0D, 0x09, 0x0E};
	(5) ����m�е�4�а�����˳��ֱ���a���˷�����:
	��0��: ��m[0][0], m[1][0], m[2][0], m[3][0]����
	��3��: ��m[1][3], m[2][3], m[3][3], m[0][3]����
	��2��: ��m[2][2], m[3][2], m[0][2], m[1][2]����
	��1��: ��m[3][1], m[0][1], m[1][1], m[2][1]����
	(6) �˷�����4��ת��4��, ���浽p��, �滻��p��ԭ�еľ���;
	(7) do_mul���������Ƿ�Ҫ���˷�����, �������һ�ּ����ܵ�һ��do_mul=0;
	*/
	unsigned char b[4];
	unsigned char t[4][4];
	int j;
	for (j = 0; j<4; j++)
	{
		get_column(p, (4 - j) % 4, j, b); /* ��p��ָ����m����ȡ��j��, ���浽����b��;
										  ��j����Ԫ�ص��к�Ϊj:
										  column 0: 0 1 2 3
										  column 3: 1 2 3 0
										  column 2: 2 3 0 1
										  column 1: 3 0 1 2
										  */
		if (do_mul) /* �ڼ������һ���Լ����ܵ�һ�ֵ�MixColumn�����в���Ҫ���˷�; */
			aes_polynomial_mul(a, b, b); /* �����ֶ�Ҫ���˷�: b = a*b mod (X^4+1); */
		memcpy(t[j], b, 4); /* �ѳ˷����ý�����Ƶ�t�е�j�� */
	}
	memcpy(p, t, 4 * 4); /* ����t�о���p, �滻��p��ԭ�о��� */
}


void aes_encrypt(unsigned char *bufin, unsigned char *bufout, unsigned char *key)
{
	int i;
	unsigned char a[4] = { 0x03, 0x01, 0x01, 0x02 }; /* �������ʽ3*X^3+X^2+X+2 */
	unsigned char matrix[4][4];
	memcpy(matrix, bufin, 4 * 4); /* ��������16�ֽڵ�matrix */
	AddRoundKey((unsigned char *)matrix, key); /* ��0��ֻ��AddRoundKey() */
	for (i = 1; i <= key_rounds; i++)
	{  /* ��1��key_rounds��, �����²���: ByteSub, ShiftRow, MixColumn, AddRoundKey */
		ByteSub((unsigned char *)matrix, 16);
		ShiftRow((unsigned char *)matrix);
		if (i != key_rounds)
			MixColumn((unsigned char *)matrix, a, 1); /* do mul */
		else
			MixColumn((unsigned char *)matrix, a, 0); /* don't mul */
		AddRoundKey((unsigned char *)matrix, key + i*(4 * 4));
	}
	memcpy(bufout, matrix, 4 * 4); /* ���ĸ��Ƶ�bufout */
}

main()
{
	unsigned char seed_key[3][100] =
	{
		"0123456789ABCDEF", /* 16 bytes */
		"0123456789ABCDEF012345678", /* 24 bytes */
		"0123456789ABCDEF0123456789ABCDEF" /* 32 bytes */
	};
	int key_size[3] = { 128, 192, 256 };
	unsigned char key[(56 + 4) * 4]; /* 128bit: 40+4, 192bit: 48+4, 256bit: 56+4 */
	unsigned char plaintext[100] = "A Quick BrownFox";
	unsigned char ciphertext[100];
	int i, k;
	char buf[100];
	aes_init(); /* ����sbox��sbox����� */
	for (k = 0; k<3; k++)
	{
		printf("Encrypting by using %d-bit seed_key...\n", key_size[k]);
		printf("plaintext=%s\n", plaintext);
		printf("seed_key=%s\n", seed_key[k]);
		aes_set_key(seed_key[k], key_size[k], key);
		/* ��128λ��16�ֽ�������Կת����176�ֽ���Կ */
		/* 192λ(24�ֽ�)��256λ(32�ֽ�)������Կ����:
		aes_set_key(seed_key, 192, key);
		aes_set_key(seed_key, 256, key);
		*/
		aes_encrypt(plaintext, ciphertext, key); /* aes���� */
		for (i = 0; i<16; i++)
		{
			sprintf(buf + i * 2, "%02X", ciphertext[i]);
		}
		printf("ciphertext=%s\n", buf);
		/* 128-bit��Կ����������: C037F19D4AEA8E7172B176B9D4CE1D62
		192-bit��Կ����������: F51C8DD5F7052264650126DC70D402F5
		256-bit��Կ����������: 2687C0D61E803718F6D11A0384C81229
		*/
		puts("Decrypting...");
		aes_decrypt(ciphertext, plaintext, key); /* aes���� */
		printf("plaintext=%s\n", plaintext);
		puts("==========================================");
	}
	return 1;
}