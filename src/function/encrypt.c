/*
 * NFC_Payment.c
 *
 *  Created on: Jan 28, 2025
 *      Author: KIMLEAN
 */
#include "def.h"
#include "http_utils.h"

#include "encrypt.h"

#include <cJSON.h>

#include <stdio.h>
#include <string.h>

/* ============ KIMLEAN DEV ============*/

int EncryptJson(const char *jsonString, char *output, int encodeKey)
{
    if (jsonString == NULL || output == NULL) {
        MAINLOG_L1("Invalid parameters\n");
        return -1;
    }

    // Calculate input length and align to 16-byte boundary for AES
    int inputLen = strlen(jsonString);
    int paddedLen = ((inputLen + 15) / 16) * 16;
    int paddingValue = paddedLen - inputLen;

    // Allocate memory for padded and encrypted data
    char *paddedData = (char *)calloc(paddedLen + 1, sizeof(char));
    char *encryptedData = (char *)calloc(paddedLen + 1, sizeof(char));

    if (!paddedData || !encryptedData) {
        MAINLOG_L1("Memory allocation failed\n");
        free(paddedData);
        free(encryptedData);
        return -2;
    }

    // Copy JSON string and apply PKCS#7 padding
    memcpy(paddedData, jsonString, inputLen);
    for (int i = 0; i < paddingValue; i++) {
        paddedData[inputLen + i] = paddingValue;
    }

    // Select encryption key based on mode
    const char *aesKey = NULL;
    switch (encodeKey) {
        case PK_DEFAULT:
            aesKey = ACLEDA_AES_KEY;
            break;
        case PK_ENCODE:
            aesKey = PK_ENCODE_AES_IV;
            break;
        case PK_DECODE:
            aesKey = PK_DECODE_AES_IV;
            break;
        default:
            MAINLOG_L1("Invalid encryption key mode: %d\n", encodeKey);
            free(paddedData);
            free(encryptedData);
            return -3;
    }

    // Encrypt the padded data
    int ret = calcAesEnc_lib(paddedData, paddedLen, encryptedData,
                            aesKey, AES_KEY_LEN, NULL, AES_ECB);

    if (ret != 0) {
        MAINLOG_L1("Encryption failed: %d\n", ret);
        free(paddedData);
        free(encryptedData);
        return ret;
    }

    // Convert encrypted data to hex string
    bytes_to_hex(encryptedData, paddedLen, output);
    output[2 * paddedLen] = '\0';

    // Clean up
    free(paddedData);
    free(encryptedData);

    return 0;
}

void DecryptJson(const char *hexString, char *output, int EncodeKey)
{
	MAINLOG_L1("hexString %s", hexString);

    int hexLen = strlen(hexString);
    MAINLOG_L1("hexString hexLen %d", hexLen);
    if (hexLen % 32 != 0)
    {
    	MAINLOG_L1("Invalid hex input length.\n");
        return;
    }

    int dataLen = hexLen / 2;
    char *encryptedData = (char *)calloc(dataLen + 1, sizeof(char));
    char *decryptedData = (char *)calloc(dataLen + 1, sizeof(char));

    if (!encryptedData || !decryptedData)
    {
    	MAINLOG_L1("Memory allocation failed.\n");
        free(encryptedData);
        free(decryptedData);
        return;
    }

    // Convert hex string back to encrypted bytes
    hex_to_bytes(hexString, encryptedData);

    int ret;
	if(EncodeKey == PK_DEFAULT)
	{
		ret = calcAesDec_lib(encryptedData,
					 dataLen,
					 decryptedData,
					 ACLEDA_AES_KEY,
					 AES_KEY_LEN,
					 NULL,
					 AES_ECB);
	}
	if(EncodeKey == PK_ENCODE)
	{
		ret = calcAesDec_lib(encryptedData,
					 dataLen,
					 decryptedData,
					 PK_ENCODE_AES_IV,
					 AES_KEY_LEN,
					 NULL,
					 AES_ECB);
	}
	if(EncodeKey == PK_DECODE)
	{
		ret = calcAesDec_lib(encryptedData,
						 dataLen,
						 decryptedData,
						 PK_DECODE_AES_IV,
						 AES_KEY_LEN,
						 NULL,
						 AES_ECB);
	}
    if (ret != 0)
    {
    	MAINLOG_L1("Decryption failed: %d\n", ret);
        free(encryptedData);
        free(decryptedData);
        return;
    }

    // Remove PKCS#7 padding
    int padding_value = decryptedData[dataLen - 1];
    if (padding_value > 0 && padding_value <= 16)
    {
        decryptedData[dataLen - padding_value] = '\0';
    }

    // Copy decrypted data to output
    strcpy(output, decryptedData);

    free(encryptedData);
    free(decryptedData);
}

int Sha256(char *input, char *output)
{
	MAINLOG_L1("VALUE => %s", G_sys_param.sn);
	int ret = calcSha_lib(G_sys_param.sn, strlen(G_sys_param.sn), output, SHA_TYPE_512);
	if (ret != 0)
	{
		LogPrintWithRet(1, "!!! SHA512 FAILED(%d) !!!", ret);
	}
	else
	{
		int output_len = strlen(output);
		LogPrintWithRet(0, "output_len = ", output_len);

		unsigned char output_asc[output_len];
		memset(output_asc, 0, sizeof(output_asc));
		BcdToAsc_Api(output_asc, output, output_len);

		LogPrintInfo("===== SHA256 RESULT(ASC) =====");
		LogPrintInfo(output_asc);
		LogPrintInfo("==============================");
	}

	return ret;
}

void DecodePublicKey() {
    char result[AES_KEY_LEN + 1];

    memcpy(result, PK_ENCODE_AES_IV, AES_KEY_LEN);
    result[AES_KEY_LEN] = '\0'; // Null terminate to use string functions safely

    MAINLOG_L1("Before Replacement: %s", result);

    // Arrays for replacement strings
    const char *to_replace[] = {"kh", "le", "se", "tc"};
    char replace_with[4][3]; // Each replacement is 2 chars + '\0'

    for (int i = 0; i < 4; i++) {
//        strncpy(replace_with[i], G_sys_param.sn + i * 2, 2);
        strncpy(replace_with[i], "00060000279" + i * 2, 2);
        replace_with[i][2] = '\0'; // Null-terminate each segment
    }

    for (int i = 0; i < 4; i++) {
        char *pos;
        while ((pos = strstr(result, to_replace[i])) != NULL) {
            memmove(pos + 2, pos + strlen(to_replace[i]), strlen(pos + strlen(to_replace[i])) + 1);
            memcpy(pos, replace_with[i], 2);
        }
    }

    MAINLOG_L1("After Replacement: %s", result);

    memcpy(PK_DECODE_AES_IV, result, AES_KEY_LEN);
}
