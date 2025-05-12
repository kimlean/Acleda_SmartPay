/*
 * SmartPay_Service.c
 *
 *  Created on: Jan 23, 2025
 *      Author: KIMLEAN
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
#include <string.h>

int httpRequestSmartPayInfo(
		HTTP_UTILS_CONNECT_PARAMS *params,
		HTTP_UTILS_REQUEST_HEADER *header,
		char *result,
		char *body)
{
	return http_utils_connect(params, header, body, result, 0, NULL);
}

void ParseSmartPayData(const char *jsonString, SmartPay *sp)
{
	char decrypted_data[2048] = {0};
	DecryptJson(jsonString, decrypted_data, PK_DECODE);
	MAINLOG_L1("Decrypted JSON: %s\n", decrypted_data);

    cJSON *json = cJSON_Parse(decrypted_data);
    if (json == NULL) {
    	 // MAINLOG_L1("Error parsing JSON\n");
        return;
    }

    cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (!cJSON_IsObject(data))
    {
    	// MAINLOG_L1("Error: 'data' field not found or invalid\n");
        cJSON_Delete(json);
        return;
    }

    // Extract fields from the "data" object
	cJSON *regId          = cJSON_GetObjectItemCaseSensitive(data, "regId");
	cJSON *accIdKhr       = cJSON_GetObjectItemCaseSensitive(data, "accIdKhr");
	cJSON *accIdUsd       = cJSON_GetObjectItemCaseSensitive(data, "accIdUsd");
	cJSON *mctName        = cJSON_GetObjectItemCaseSensitive(data, "mctName");
	cJSON *mmc            = cJSON_GetObjectItemCaseSensitive(data, "mmc");
	cJSON *mbNo           = cJSON_GetObjectItemCaseSensitive(data, "mbNo");
	cJSON *counCode       = cJSON_GetObjectItemCaseSensitive(data, "counCode");
	cJSON *cityName       = cJSON_GetObjectItemCaseSensitive(data, "cityName");
	cJSON *mctId          = cJSON_GetObjectItemCaseSensitive(data, "mctId");
	cJSON *adDataTCGUID   = cJSON_GetObjectItemCaseSensitive(data, "adDataTCGUID");

	// Copy values into the SmartPay struct
	strncpy(sp->regId, regId ? regId->valuestring : "", sizeof(sp->regId) - 1);
	strncpy(sp->accIdKhr, accIdKhr ? accIdKhr->valuestring : "", sizeof(sp->accIdKhr) - 1);
	strncpy(sp->accIdUsd, accIdUsd ? accIdUsd->valuestring : "", sizeof(sp->accIdUsd) - 1);
	strncpy(sp->mctName, mctName ? mctName->valuestring : "", sizeof(sp->mctName) - 1);
	strncpy(sp->mmc, mmc ? mmc->valuestring : "", sizeof(sp->mmc) - 1);
	strncpy(sp->mbNo, mbNo ? mbNo->valuestring : "", sizeof(sp->mbNo) - 1);
	strncpy(sp->counCode, counCode ? counCode->valuestring : "", sizeof(sp->counCode) - 1);
	strncpy(sp->cityName, cityName ? cityName->valuestring : "", sizeof(sp->cityName) - 1);
	strncpy(sp->mctId, mctId ? mctId->valuestring : "", sizeof(sp->mctId) - 1);
	strncpy(sp->adDataTCGUID, adDataTCGUID ? adDataTCGUID->valuestring : "", sizeof(sp->adDataTCGUID) - 1);

	// Ensure null termination
	sp->regId[sizeof(sp->regId) - 1] = '\0';
	sp->accIdKhr[sizeof(sp->accIdKhr) - 1] = '\0';
	sp->accIdUsd[sizeof(sp->accIdUsd) - 1] = '\0';
	sp->mctName[sizeof(sp->mctName) - 1] = '\0';
	sp->mmc[sizeof(sp->mmc) - 1] = '\0';
	sp->mbNo[sizeof(sp->mbNo) - 1] = '\0';
	sp->counCode[sizeof(sp->counCode) - 1] = '\0';
	sp->cityName[sizeof(sp->cityName) - 1] = '\0';
	sp->mctId[sizeof(sp->mctId) - 1] = '\0';
	sp->adDataTCGUID[sizeof(sp->adDataTCGUID) - 1] = '\0';

    // Clean up
    cJSON_Delete(json);
}

int GetSmartPayInfo_Service()
{
	int ret = 0;

	// PREAPREA RQUEST VALUE
	cJSON *root = NULL;
	cJSON *obj  = NULL;
	root = cJSON_CreateObject();
	if (root == NULL) {
		// MAINLOG_L1("!!! cJSON_CreateObject() failed(root=NULL) !!!");
		return -1;
	}
	obj = cJSON_AddStringToObject(root, "device_sn", G_sys_param.sn);
	if (obj == NULL) {
		 MAINLOG_L1("!!! cJSON_AddStringToObject() failed('device_sn') !!!");
		return -1;
	}
	obj = cJSON_AddStringToObject(root, "key", PK_ENCODE_AES_IV);
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
	char encryptedPar[512] = {0};
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
	sprintf(params.request_suffix, "%s", "/NFC/CU01SER2LZ");
	sprintf(params.version,        "%s", "HTTP/1.1");

	// HEADER
	HTTP_UTILS_REQUEST_HEADER header;
	memset(&header, 0, sizeof(HTTP_UTILS_REQUEST_HEADER));

	sprintf(header.accept,	        "%s", "*/*");
	sprintf(header.accept_encoding, "%s", "gzip, deflate, br, chunked");
	sprintf(header.content_type,    "%s", "application/json; charset=UTF-8");
	sprintf(header.user_agent,      "%s", "Mozilla/4.0(compatible; MSIE 5.5; Windows 98)");
	sprintf(header.connection,      "%s", "keep-alive");

	char *result = malloc(1024);
	if (result == NULL) {
		MAINLOG_L1("Error: Failed to allocate memory for result\n");
		return -1;
	}

	ret = httpRequestSmartPayInfo(&params, &header, result, body);
	if (ret == 0)
	{
		ParseSmartPayData(result, &SmartPay_Info);
	}

	free(result);
	return ret;
}
