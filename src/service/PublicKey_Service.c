/*
 * PublicKey_Service.c
 *
 *  Created on: Feb 17, 2025
 *      Author: Kimlean
 */

#include "def.h"
#include "http_utils.h"
#include "EnvAcleda.h"
#include "encrypt.h"
#include "SmartPayInfo.h"

#include <stdlib.h>
#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#include <cJSON.h>

int httpRequestPublicKey(
		HTTP_UTILS_CONNECT_PARAMS *params,
		HTTP_UTILS_REQUEST_HEADER *header,
		char *result,
		char *body)
{
	return http_utils_connect(params, header, body, result, 0, NULL);
}

int ParsePublicKey(const char *result)
{
    char decrypted_data[2048] = {0};
    DecryptJson(result, decrypted_data, PK_DEFAULT);

    cJSON *json = cJSON_Parse(decrypted_data);
    if (json == NULL) {
        MAINLOG_L1("Error parsing JSON\n");
        return -1;
    }

    cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (!cJSON_IsObject(data)) {
        MAINLOG_L1("Error: 'data' field not found or invalid\n");
        cJSON_Delete(json);
        return -1;
    }

    // Extract fields from the "data" object
    cJSON *public_key = cJSON_GetObjectItemCaseSensitive(data, "generatedKey");
    if (!cJSON_IsString(public_key) || (public_key->valuestring == NULL)) {
        MAINLOG_L1("Error: 'generatedKey' field not found or invalid\n");
        cJSON_Delete(json);
        return -1;
    }

    // Copy the extracted public key into PUBLICKEY_AES_IV
    strncpy((char *)PK_ENCODE_AES_IV, public_key->valuestring, AES_KEY_LEN);
    PK_ENCODE_AES_IV[AES_KEY_LEN] = '\0';  // Ensure null termination

    // DECODE PUBLIC KEY
    char temp[AES_KEY_LEN + 1];

	memcpy(temp, PK_ENCODE_AES_IV, AES_KEY_LEN);
	temp[AES_KEY_LEN] = '\0'; // Null terminate to use string functions safely

	// Arrays for replacement strings
	const char *to_replace[] = {"kh", "le", "se", "tc"};
	char replace_with[4][3]; // Each replacement is 2 chars + '\0'

	for (int i = 0; i < 4; i++) {
//		strncpy(replace_with[i], G_sys_param.sn + i * 2, 2);
		strncpy(replace_with[i], "00060000279" + i * 2, 2);
		replace_with[i][2] = '\0'; // Null-terminate each segment
	}

	for (int i = 0; i < 4; i++) {
		char *pos;
		while ((pos = strstr(temp, to_replace[i])) != NULL) {
			memmove(pos + 2, pos + strlen(to_replace[i]), strlen(pos + strlen(to_replace[i])) + 1);
			memcpy(pos, replace_with[i], 2);
		}
	}

	memcpy(PK_DECODE_AES_IV, temp, AES_KEY_LEN);

	// Clean up
    cJSON_Delete(json);
    return 0;
}

int GetPublicKey_Service()
{
	int ret = 0;

	// PREAPREA RQUEST VALUE
	cJSON *root = NULL;
	cJSON *obj  = NULL;
	root = cJSON_CreateObject();
	if (root == NULL) {
		MAINLOG_L1("!!! cJSON_CreateObject() failed(root=NULL) !!!");
		return -1;
	}
//	obj = cJSON_AddStringToObject(root, "device_sn", G_sys_param.sn);
	obj = cJSON_AddStringToObject(root, "device_sn", "00060000279");
	if (obj == NULL) {
		MAINLOG_L1("!!! cJSON_AddStringToObject() failed('device_sn') !!!");
		return -1;
	}
	char *ParseJson = cJSON_PrintUnformatted(root);
	if (ParseJson == NULL) {
		MAINLOG_L1("!!! cJSON_PrintUnformatted() failed(result=NULL) !!!");
		return -1;
	}
	cJSON_Delete(root);

	// ENCRYPTION
	char encryptedPar[2048] = {0};
	EncryptJson(ParseJson, encryptedPar, PK_DEFAULT);

	// PREPARE BODY TESTING CODE
	cJSON *root2 = NULL;
	root2 = cJSON_CreateObject();
	cJSON_AddStringToObject(root2, "par", encryptedPar);
	char *body = cJSON_PrintUnformatted(root2);

	// Params
	HTTP_UTILS_CONNECT_PARAMS params;
	memset(&params, 0, sizeof(HTTP_UTILS_CONNECT_PARAMS));
	params.protocol = HTTPS_PROTOCOL;
	sprintf(params.domain, 		   "%s", ENV_AC_APIHOST);
	sprintf(params.port,   		   "%s", ENV_AC_APIPORT);
	sprintf(params.type, 	       "%s", "GET");
	sprintf(params.request_suffix, "%s", "/NFC/CU06SER2LZ");
	sprintf(params.version,        "%s", "HTTP/1.1");

	// HEADER
	HTTP_UTILS_REQUEST_HEADER header;
	memset(&header, 0, sizeof(HTTP_UTILS_REQUEST_HEADER));

	sprintf(header.accept,	        "%s", "*/*");
	sprintf(header.accept_encoding, "%s", "gzip, deflate, br");
	sprintf(header.content_type,    "%s", "application/json; charset=UTF-8");
	sprintf(header.user_agent,      "%s", "Mozilla/4.0(compatible; MSIE 5.5; Windows 98)");
	sprintf(header.connection,      "%s", "keep-alive");

	char *result = malloc(224);
	if (result == NULL) {
		MAINLOG_L1("Error: Failed to allocate memory for result\n");
		return -1;
	}

	ret = httpRequestPublicKey(&params, &header, result, body);
	MAINLOG_L1("result %s", result);
	if (ret == 0)
	{
		ret = ParsePublicKey(result);
		if(ret == 0)
		{
			free(result);

			ret = GetSmartPayInfo_Service();
			return ret;
		}
	}

	return ret;
}

