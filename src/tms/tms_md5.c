/* MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm 
 */
  
/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All 
rights reserved. 
  
License to copy and use this software is granted provided that it 
is identified as the "RSA Data Security, Inc. MD5 Message-Digest 
Algorithm" in all material mentioning or referencing this software 
or this function. 
  
License is also granted to make and use derivative works provided 
that such works are identified as "derived from the RSA Data 
Security, Inc. MD5 Message-Digest Algorithm" in all material 
mentioning or referencing the derived work. 
  
RSA Data Security, Inc. makes no representations concerning either 
the merchantability of this software or the suitability of this 
software for any particular purpose. It is provided "as is" 
without express or implied warranty of any kind. 
  
These notices must be retained in any copies of any part of this 
documentation and/or software. 
 */

#include <string.h> 
#include <stdio.h> 

#include "../inc/tms_md5.h"
  
/* Constants for MD5Transform routine. 
*/
  
  
#define _TMS_S11 7 
#define _TMS_S12 12 
#define _TMS_S13 17 
#define _TMS_S14 22 
#define _TMS_S21 5 
#define _TMS_S22 9 
#define _TMS_S23 14 
#define _TMS_S24 20 
#define _TMS_S31 4 
#define _TMS_S32 11 
#define _TMS_S33 16 
#define _TMS_S34 23 
#define _TMS_S41 6 
#define _TMS_S42 10 
#define _TMS_S43 15 
#define _TMS_S44 21 
  
static void _tms_MD5_memcpy (POINTER output, POINTER input, unsigned int len); 
static void _tms_MD5Transform (UINT4 state[4], unsigned char block[64]); 
static void _tms_Encode (unsigned char *output, UINT4 *input, unsigned int len); 
static void _tms_MD5_memset (POINTER output, int value, unsigned int len); 
static void _tms_Decode (UINT4 *output, unsigned char *input, unsigned int len); 
  
static unsigned char _TMS_PADDING[64] = { 
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
}; 
  
/* F, G, H and I are basic MD5 functions. 
*/
#define _TMS_F(x, y, z) (((x) & (y)) | ((~x) & (z))) 
#define _TMS_G(x, y, z) (((x) & (z)) | ((y) & (~z))) 
#define _TMS_H(x, y, z) ((x) ^ (y) ^ (z)) 
#define _TMS_I(x, y, z) ((y) ^ ((x) | (~z))) 
  
/* ROTATE_LEFT rotates x left n bits. 
*/
#define _TMS_ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n)))) 
  
/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4. 
Rotation is separate from addition to prevent recomputation. 
*/

#define _TMS_FF(a, b, c, d, x, s, ac) { \
 (a) += _TMS_F ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = _TMS_ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
 } 
#define _TMS_GG(a, b, c, d, x, s, ac) { \
 (a) += _TMS_G ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = _TMS_ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
 } 
#define _TMS_HH(a, b, c, d, x, s, ac) { \
 (a) += _TMS_H ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = _TMS_ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
 } 
#define _TMS_II(a, b, c, d, x, s, ac) { \
 (a) += _TMS_I ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = _TMS_ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
 } 
  
/* MD5 initialization. Begins an MD5 operation, writing a new context. 
 */
void _tms_MD5Init (_tms_MD5_CTX *context)          /* context */
{ 
 context->count[0] = context->count[1] = 0; 
 /* Load magic initialization constants. 
 */
 context->state[0] = 0x67452301; 
 context->state[1] = 0xefcdab89; 
 context->state[2] = 0x98badcfe; 
 context->state[3] = 0x10325476; 
} 
  
/* MD5 block update operation. Continues an MD5 message-digest 
 operation, processing another message block, and updating the 
 context. 
 */
void _tms_MD5Update(_tms_MD5_CTX *context, unsigned char *input, unsigned int inputLen)
  
{ 
 unsigned int i, index, partLen; 
  
 /* Compute number of bytes mod 64 */
 index = (unsigned int)((context->count[0] >> 3) & 0x3F); 
  
 /* Update number of bits */
 if ((context->count[0] += ((UINT4)inputLen << 3)) 
  < ((UINT4)inputLen << 3)) 
  context->count[1]++; 
 context->count[1] += ((UINT4)inputLen >> 29); 
  
 partLen = 64 - index; 
  
 /* Transform as many times as possible. 
 */
 if (inputLen >= partLen) { 
  _tms_MD5_memcpy((POINTER)&context->buffer[index], (POINTER)input, partLen); 
  _tms_MD5Transform (context->state, context->buffer); 
  
  for (i = partLen; i + 63 < inputLen; i += 64) 
   _tms_MD5Transform (context->state, &input[i]); 
  
  index = 0; 
 } 
 else
  i = 0; 
  
 /* Buffer remaining input */
 _tms_MD5_memcpy((POINTER)&context->buffer[index], (POINTER)&input[i],inputLen-i); 
} 
  
/* MD5 finalization. Ends an MD5 message-digest operation, writing the 
 the message digest and zeroizing the context. 
 */
void _tms_MD5Final(unsigned char digest[16], _tms_MD5_CTX *context)
{ 
 unsigned char bits[8]; 
 unsigned int index, padLen; 
  
 /* Save number of bits */
 _tms_Encode (bits, context->count, 8); 
  
 /* Pad out to 56 mod 64. 
 */
 index = (unsigned int)((context->count[0] >> 3) & 0x3f); 
 padLen = (index < 56) ? (56 - index) : (120 - index); 
 _tms_MD5Update (context, _TMS_PADDING, padLen); 
  
 /* Append length (before padding) */
 _tms_MD5Update (context, bits, 8); 
  
 /* Store state in digest */
 _tms_Encode (digest, context->state, 16); 
  
 /* Zeroize sensitive information. 
 */
 _tms_MD5_memset ((POINTER)context, 0, sizeof (*context)); 
} 
  
/* MD5 basic transformation. Transforms state based on block. 
 */
static void _tms_MD5Transform (UINT4 state[4], unsigned char block[64]) 
{ 
 UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16]; 
  
 _tms_Decode (x, block, 64); 
  
 /* Round 1 */
 _TMS_FF (a, b, c, d, x[ 0], _TMS_S11, 0xd76aa478); /* 1 */
 _TMS_FF (d, a, b, c, x[ 1], _TMS_S12, 0xe8c7b756); /* 2 */
 _TMS_FF (c, d, a, b, x[ 2], _TMS_S13, 0x242070db); /* 3 */
 _TMS_FF (b, c, d, a, x[ 3], _TMS_S14, 0xc1bdceee); /* 4 */
 _TMS_FF (a, b, c, d, x[ 4], _TMS_S11, 0xf57c0faf); /* 5 */
 _TMS_FF (d, a, b, c, x[ 5], _TMS_S12, 0x4787c62a); /* 6 */
 _TMS_FF (c, d, a, b, x[ 6], _TMS_S13, 0xa8304613); /* 7 */
 _TMS_FF (b, c, d, a, x[ 7], _TMS_S14, 0xfd469501); /* 8 */
 _TMS_FF (a, b, c, d, x[ 8], _TMS_S11, 0x698098d8); /* 9 */
 _TMS_FF (d, a, b, c, x[ 9], _TMS_S12, 0x8b44f7af); /* 10 */
 _TMS_FF (c, d, a, b, x[10], _TMS_S13, 0xffff5bb1); /* 11 */
 _TMS_FF (b, c, d, a, x[11], _TMS_S14, 0x895cd7be); /* 12 */
 _TMS_FF (a, b, c, d, x[12], _TMS_S11, 0x6b901122); /* 13 */
 _TMS_FF (d, a, b, c, x[13], _TMS_S12, 0xfd987193); /* 14 */
 _TMS_FF (c, d, a, b, x[14], _TMS_S13, 0xa679438e); /* 15 */
 _TMS_FF (b, c, d, a, x[15], _TMS_S14, 0x49b40821); /* 16 */
  
 /* Round 2 */
 _TMS_GG (a, b, c, d, x[ 1], _TMS_S21, 0xf61e2562); /* 17 */
 _TMS_GG (d, a, b, c, x[ 6], _TMS_S22, 0xc040b340); /* 18 */
 _TMS_GG (c, d, a, b, x[11], _TMS_S23, 0x265e5a51); /* 19 */
 _TMS_GG (b, c, d, a, x[ 0], _TMS_S24, 0xe9b6c7aa); /* 20 */
 _TMS_GG (a, b, c, d, x[ 5], _TMS_S21, 0xd62f105d); /* 21 */
 _TMS_GG (d, a, b, c, x[10], _TMS_S22, 0x2441453); /* 22 */
 _TMS_GG (c, d, a, b, x[15], _TMS_S23, 0xd8a1e681); /* 23 */
 _TMS_GG (b, c, d, a, x[ 4], _TMS_S24, 0xe7d3fbc8); /* 24 */
 _TMS_GG (a, b, c, d, x[ 9], _TMS_S21, 0x21e1cde6); /* 25 */
 _TMS_GG (d, a, b, c, x[14], _TMS_S22, 0xc33707d6); /* 26 */
 _TMS_GG (c, d, a, b, x[ 3], _TMS_S23, 0xf4d50d87); /* 27 */
 _TMS_GG (b, c, d, a, x[ 8], _TMS_S24, 0x455a14ed); /* 28 */
 _TMS_GG (a, b, c, d, x[13], _TMS_S21, 0xa9e3e905); /* 29 */
 _TMS_GG (d, a, b, c, x[ 2], _TMS_S22, 0xfcefa3f8); /* 30 */
 _TMS_GG (c, d, a, b, x[ 7], _TMS_S23, 0x676f02d9); /* 31 */
 _TMS_GG (b, c, d, a, x[12], _TMS_S24, 0x8d2a4c8a); /* 32 */
  
 /* Round 3 */
 _TMS_HH (a, b, c, d, x[ 5], _TMS_S31, 0xfffa3942); /* 33 */
 _TMS_HH (d, a, b, c, x[ 8], _TMS_S32, 0x8771f681); /* 34 */
 _TMS_HH (c, d, a, b, x[11], _TMS_S33, 0x6d9d6122); /* 35 */
 _TMS_HH (b, c, d, a, x[14], _TMS_S34, 0xfde5380c); /* 36 */
 _TMS_HH (a, b, c, d, x[ 1], _TMS_S31, 0xa4beea44); /* 37 */
 _TMS_HH (d, a, b, c, x[ 4], _TMS_S32, 0x4bdecfa9); /* 38 */
 _TMS_HH (c, d, a, b, x[ 7], _TMS_S33, 0xf6bb4b60); /* 39 */
 _TMS_HH (b, c, d, a, x[10], _TMS_S34, 0xbebfbc70); /* 40 */
 _TMS_HH (a, b, c, d, x[13], _TMS_S31, 0x289b7ec6); /* 41 */
 _TMS_HH (d, a, b, c, x[ 0], _TMS_S32, 0xeaa127fa); /* 42 */
 _TMS_HH (c, d, a, b, x[ 3], _TMS_S33, 0xd4ef3085); /* 43 */
 _TMS_HH (b, c, d, a, x[ 6], _TMS_S34, 0x4881d05); /* 44 */
 _TMS_HH (a, b, c, d, x[ 9], _TMS_S31, 0xd9d4d039); /* 45 */
 _TMS_HH (d, a, b, c, x[12], _TMS_S32, 0xe6db99e5); /* 46 */
 _TMS_HH (c, d, a, b, x[15], _TMS_S33, 0x1fa27cf8); /* 47 */
 _TMS_HH (b, c, d, a, x[ 2], _TMS_S34, 0xc4ac5665); /* 48 */
  
 /* Round 4 */
 _TMS_II (a, b, c, d, x[ 0], _TMS_S41, 0xf4292244); /* 49 */
 _TMS_II (d, a, b, c, x[ 7], _TMS_S42, 0x432aff97); /* 50 */
 _TMS_II (c, d, a, b, x[14], _TMS_S43, 0xab9423a7); /* 51 */
 _TMS_II (b, c, d, a, x[ 5], _TMS_S44, 0xfc93a039); /* 52 */
 _TMS_II (a, b, c, d, x[12], _TMS_S41, 0x655b59c3); /* 53 */
 _TMS_II (d, a, b, c, x[ 3], _TMS_S42, 0x8f0ccc92); /* 54 */
 _TMS_II (c, d, a, b, x[10], _TMS_S43, 0xffeff47d); /* 55 */
 _TMS_II (b, c, d, a, x[ 1], _TMS_S44, 0x85845dd1); /* 56 */
 _TMS_II (a, b, c, d, x[ 8], _TMS_S41, 0x6fa87e4f); /* 57 */
 _TMS_II (d, a, b, c, x[15], _TMS_S42, 0xfe2ce6e0); /* 58 */
 _TMS_II (c, d, a, b, x[ 6], _TMS_S43, 0xa3014314); /* 59 */
 _TMS_II (b, c, d, a, x[13], _TMS_S44, 0x4e0811a1); /* 60 */
 _TMS_II (a, b, c, d, x[ 4], _TMS_S41, 0xf7537e82); /* 61 */
 _TMS_II (d, a, b, c, x[11], _TMS_S42, 0xbd3af235); /* 62 */
 _TMS_II (c, d, a, b, x[ 2], _TMS_S43, 0x2ad7d2bb); /* 63 */
 _TMS_II (b, c, d, a, x[ 9], _TMS_S44, 0xeb86d391); /* 64 */
  
 state[0] += a; 
 state[1] += b; 
 state[2] += c; 
 state[3] += d; 
  
 /* Zeroize sensitive information. 
 */
 _tms_MD5_memset ((POINTER)x, 0, sizeof (x)); 
} 
  
/* Encodes input (UINT4) into output (unsigned char). Assumes len is 
 a multiple of 4. 
 */
static void _tms_Encode (unsigned char *output, UINT4 *input, unsigned int len) 
{ 
 unsigned int i, j; 
  
 for (i = 0, j = 0; j < len; i++, j += 4) { 
  output[j] = (unsigned char)(input[i] & 0xff); 
  output[j+1] = (unsigned char)((input[i] >> 8) & 0xff); 
  output[j+2] = (unsigned char)((input[i] >> 16) & 0xff); 
  output[j+3] = (unsigned char)((input[i] >> 24) & 0xff); 
 } 
} 
  
/* Decodes input (unsigned char) into output (UINT4). Assumes len is 
 a multiple of 4. 
 */
static void _tms_Decode (UINT4 *output, unsigned char *input, unsigned int len) 
{ 
 unsigned int i, j; 
  
 for (i = 0, j = 0; j < len; i++, j += 4) 
  output[i] = ((UINT4)input[j]) | (((UINT4)input[j+1]) << 8) | 
  (((UINT4)input[j+2]) << 16) | (((UINT4)input[j+3]) << 24); 
} 
  
/* Note: Replace "for loop" with standard memcpy if possible. 
 */
  
static void _tms_MD5_memcpy (POINTER output, POINTER input, unsigned int len) 
{ 
 unsigned int i; 
  
 for (i = 0; i < len; i++) 
  output[i] = input[i]; 
} 
  
/* Note: Replace "for loop" with standard memset if possible. 
 */
static void _tms_MD5_memset (POINTER output, int value, unsigned int len) 
{ 
 unsigned int i; 
  
 for (i = 0; i < len; i++) 
  ((char *)output)[i] = (char)value; 
} 
/* Digests a string and prints the result. 
 */
void _tms_MDString (char *string,unsigned char digest[16]) 
{ 
 _tms_MD5_CTX context; 
 unsigned int len = strlen (string); 
  
 _tms_MD5Init (&context); 
 _tms_MD5Update (&context, (unsigned char *)string, len); 
 _tms_MD5Final (digest, &context); 
} 
/* Digests a file and prints the result. 
 */
int _tms_MD5File (char *filename,unsigned char digest[16]) 
{ 
 /*FILE *file;   //V36H not support FILE
 _tms_MD5_CTX context; 
 int len; 
 unsigned char buffer[1024]; 
  
 if ((file = fopen (filename, "rb")) == NULL) 
  return -1; 
 else { 
  _tms_MD5Init (&context); 
  while (len = fread (buffer, 1, 1024, file)) 
   _tms_MD5Update (&context, buffer, len); 
  _tms_MD5Final (digest, &context); 
  
  fclose (file); 
 } */
 return 0; 
} 
void _tms_MD5UpdaterString(_tms_MD5_CTX *context,const char *string) 
{ 
 unsigned int len = strlen (string); 
 _tms_MD5Update (context, (unsigned char *)string, len); 
} 
int _tms_MD5FileUpdateFile (_tms_MD5_CTX *context,char *filename) 
{ 
	/* v36h not support FILE
 FILE *file; 
 int len; 
 unsigned char buffer[1024]; 
  
 if ((file = fopen (filename, "rb")) == NULL) 
  return -1; 
 else { 
  while (len = fread (buffer, 1, 1024, file)) 
   _tms_MD5Update (context, buffer, len); 
  fclose (file); 
 } */
 return 0; 
} 
/*
void main(void)
{ 
	unsigned char digest[16]; //存放结果 

	//第一种用法: 

	_tms_MD5_CTX _tms_MD5c; 
	_tms_MD5Init(&_tms_MD5c); //初始化 
	_tms_MD5UpdaterString(&_tms_MD5c,"你要测试的字符串"); 
	_tms_MD5FileUpdateFile(&_tms_MD5c,"你要测试的文件路径"); 
	_tms_MD5Final(digest,&_tms_MD5c); 

	//第二种用法: 
	_tms_MDString("你要测试的字符串",digest); //直接输入字符串并得出结果 

	//第三种用法: 
	_tms_MD5File("你要测试的文件路径",digest); //直接输入文件路径并得出结果 
} */
