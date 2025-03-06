/*
 * encrypt.h
 *
 *  Created on: Jan 23, 2025
 *      Author: kimlean
 */

#ifndef ENCRYPT_H_
#define ENCRYPT_H_

int EncryptJson(const char *jsonString, char *output, int EncodeKey);
void DecryptJson(const char *hexString, char *output, int EncodeKey);
int Sha256(char *input, char *output);

void DecodePublicKey();

// ========= ENCRYPT_TYPE

#define PK_DEFAULT  0
#define PK_ENCODE   1
#define PK_DECODE   2

// =========

// ========= ENCRYPT_KEY
extern unsigned char PK_ENCODE_AES_IV[AES_KEY_LEN];
extern unsigned char PK_DECODE_AES_IV[AES_KEY_LEN];
// =========

#endif /* ENCRYPT_H_ */
