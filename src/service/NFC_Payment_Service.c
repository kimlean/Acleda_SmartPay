/*
 * NFC_Payment.c
 *
 *  Created on: Jan 28, 2025
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
#include <ctype.h>
#include <string.h>

int httpRequestNFCPayment(
		HTTP_UTILS_CONNECT_PARAMS *params,
		HTTP_UTILS_REQUEST_HEADER *header,
		char *result,
		char *body)
{
	return http_utils_connect(params, header, body, result, 0, NULL);
}

int ParseResults(const char *result)
{
    char decrypted_data[2048] = {0};
    DecryptJson(result, decrypted_data, PK_DECODE);
    MAINLOG_L1("ParseResults %s", decrypted_data);

    cJSON *json = cJSON_Parse(decrypted_data);
    if (json == NULL) {
        MAINLOG_L1("Error parsing JSON\n");
        return -1;
    }

    // Extract code field directly from the JSON root
    cJSON *code = cJSON_GetObjectItemCaseSensitive(json, "code");
    if (!code || !cJSON_IsString(code)) {
        MAINLOG_L1("No valid 'code' field found in JSON");
        cJSON_Delete(json);
        return -1;
    }

    // Get the code value
    char codeValue[20] = {0};
    strncpy(codeValue, code->valuestring, sizeof(codeValue) - 1);

    // Log the code we found
    MAINLOG_L1("Found code: %s", codeValue);

    // Check the code value
    if (strcmp(codeValue, "0") == 0) {
        // Code 0 usually means success
        cJSON_Delete(json);
        return 0;
    } else {
        // Any other code is typically an error
        cJSON_Delete(json);
        return 1;
    }
}

int NFCPayment_Service(char *parmMB, NFC_PAYMENT entry)
{
    int ret = 0;
    char *ParseJson1 = NULL;
    char *result = NULL;
    char *param1a = NULL;

    const char* timeout = getCurrentTimeMs();

    // Allocate memory for param1a and param1b
    param1a = (char *)malloc(1024 * sizeof(char));
    if (param1a == NULL) {
         MAINLOG_L1("ERROR: Failed to allocate memory for param1a");
        return -1;
    }
    memset(param1a, 0, 1024 * sizeof(char));

    HTTP_UTILS_CONNECT_PARAMS params;
    HTTP_UTILS_REQUEST_HEADER header;

    // Create first JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
    	MAINLOG_L1("ERROR: cJSON_CreateObject() failed (root=NULL)");
        free(param1a);
        return -1;
    }

    cJSON_AddStringToObject(root, "reqDateTime", timeout);
    cJSON_AddStringToObject(root, "deviceId", G_sys_param.sn);
    cJSON_AddStringToObject(root, "platform", "Smart Pay");
    cJSON_AddStringToObject(root, "regId", SmartPay_Info.regId);
    cJSON_AddStringToObject(root, "amount", entry.amount);
    cJSON_AddStringToObject(root, "ccy", entry.ccy);
    cJSON_AddStringToObject(root, "creditAccount", entry.creditAccount);

    // Convert to string
    ParseJson1 = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (ParseJson1 == NULL) {
        free(param1a);
        return -1;
    }

    MAINLOG_L1("Request value %s", ParseJson1);

    EncryptJson(ParseJson1, param1a, PK_DECODE);
    free(ParseJson1);
    ParseJson1 = NULL;

    char* encodeDeviceID = (char *)malloc(128 * sizeof(char));
    if (encodeDeviceID == NULL) {
		 MAINLOG_L1("ERROR: Failed to allocate memory for param1a");
		return -1;
	}
    memset(encodeDeviceID, 0, 128 * sizeof(char));
    EncryptJson(G_sys_param.sn, encodeDeviceID, PK_DEFAULT);

	// Concatenate strings
	char finalResult[1024];
	snprintf(finalResult, sizeof(finalResult), "%s%s%s", encodeDeviceID, PK_ENCODE_AES_IV, param1a);

	root = cJSON_CreateObject();
	if (root == NULL) {
		 MAINLOG_L1("ERROR: cJSON_CreateObject() failed (root=NULL)");
		free(param1a);
		return -1;
	}
    cJSON_AddStringToObject(root, "par", finalResult);
    cJSON_AddStringToObject(root, "parmb", parmMB);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (body == NULL) {
         MAINLOG_L1("ERROR: cJSON_PrintUnformatted() failed (result=NULL)");
        free(param1a);
        return -1;
    }

    // Initialize HTTP parameters
    memset(&params, 0, sizeof(HTTP_UTILS_CONNECT_PARAMS));
    params.protocol = HTTPS_PROTOCOL;
    snprintf(params.domain, sizeof(params.domain), "%s", ENV_AC_APIHOST);
    snprintf(params.port, sizeof(params.port), "%s", ENV_AC_APIPORT);
    snprintf(params.type, sizeof(params.type), "%s", "POST");
    snprintf(params.request_suffix, sizeof(params.request_suffix), "%s", "/NFC/CU05SER2LZ");
    snprintf(params.version, sizeof(params.version), "%s", "HTTP/1.1");

    // Initialize HTTP headers
    memset(&header, 0, sizeof(HTTP_UTILS_REQUEST_HEADER));
    snprintf(header.accept, sizeof(header.accept), "%s", "*/*");
    snprintf(header.accept_encoding, sizeof(header.accept_encoding), "%s", "gzip, deflate, br, chunked");
    snprintf(header.content_type, sizeof(header.content_type), "%s", "application/json");
    snprintf(header.user_agent, sizeof(header.user_agent), "%s", "Mozilla/4.0(compatible; MSIE 5.5; Windows 98)");
    snprintf(header.connection, sizeof(header.connection), "%s", "keep-alive");

    // Allocate memory for result
    result = malloc(1024);
    if (result == NULL) {
        MAINLOG_L1("ERROR: Failed to allocate memory for result");
        free(param1a);
        free(body);
        return -1;
    }
    memset(result, 0, 1024);

    // Make HTTP request
    ret = httpRequestNFCPayment(&params, &header, result, body);
    if(ret == 0)
    {
    	ret = ParseResults(result);
    }
    free(body);
	free(result);
	free(param1a);
	return ret;
}
