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

int httpRequestNFCPayment(
		HTTP_UTILS_CONNECT_PARAMS *params,
		HTTP_UTILS_REQUEST_HEADER *header,
		char *result,
		char *body)
{
	return http_utils_connect(params, header, body, result, 0, NULL);
}

int NFCPayment_Service(char *parmMB, NFC_PAYMENT entry)
{
    MAINLOG_L1("===== NFCPayment_Service started =====");
    MAINLOG_L1("MOBILE PARAM => %s", parmMB);

    int ret = 0;
    char *ParseJson1 = NULL;
    char *ParseJson2 = NULL;
    char *body = NULL;
    char *result = NULL;
    char *param1a = NULL;
    char *param1b = NULL;

    const char* timeout = getTimeOutASecound();
    MAINLOG_L1("Timeout: %s", timeout);

    // Allocate memory for param1a and param1b
    param1a = (char *)malloc(1024 * sizeof(char));
    if (param1a == NULL) {
        MAINLOG_L1("ERROR: Failed to allocate memory for param1a");
        return -1;
    }
    memset(param1a, 0, 1024 * sizeof(char));

    param1b = (char *)malloc(2048 * sizeof(char));
    if (param1b == NULL) {
        MAINLOG_L1("ERROR: Failed to allocate memory for param1b");
        free(param1a);
        return -1;
    }
    memset(param1b, 0, 2048 * sizeof(char));

    HTTP_UTILS_CONNECT_PARAMS params;
    HTTP_UTILS_REQUEST_HEADER header;

    // Create first JSON object
    MAINLOG_L1("Creating first JSON object...");
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        MAINLOG_L1("ERROR: cJSON_CreateObject() failed (root=NULL)");
        free(param1a);
        free(param1b);
        return -1;
    }

    // Add parameters to JSON object
//    cJSON_AddStringToObject(root, "appVersion", "7.13");
    cJSON_AddStringToObject(root, "reqDateTime", timeout);
    cJSON_AddStringToObject(root, "deviceId", G_sys_param.sn);
//    cJSON_AddStringToObject(root, "ip", "xx.xx.xx.xx");
//    cJSON_AddStringToObject(root, "langCode", "EN");
//    cJSON_AddStringToObject(root, "countryCode", "855");
//    cJSON_AddStringToObject(root, "location", "");
    cJSON_AddStringToObject(root, "platform", "Smart Pay");
    cJSON_AddStringToObject(root, "regId", SmartPay_Info.regId);
    cJSON_AddStringToObject(root, "amount", entry.amount);
    cJSON_AddStringToObject(root, "ccy", entry.ccy);
    cJSON_AddStringToObject(root, "creditAccount", entry.creditAccount);

    // Convert to string
    ParseJson1 = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (ParseJson1 == NULL) {
        MAINLOG_L1("ERROR: cJSON_PrintUnformatted() failed (result=NULL)");
        free(param1a);
        free(param1b);
        return -1;
    }

    MAINLOG_L1("First JSON created: %s", ParseJson1);

    // Encrypt first JSON
    MAINLOG_L1("Encrypting first JSON with PK_DECODE...");
    EncryptJson(ParseJson1, param1a, PK_DECODE);
    MAINLOG_L1("First encryption result: %s", param1a);
    free(ParseJson1);
    ParseJson1 = NULL;

    // Create second JSON with encrypted data
    MAINLOG_L1("Creating second JSON object...");
    root = cJSON_CreateObject();
    if (root == NULL) {
        MAINLOG_L1("ERROR: cJSON_CreateObject() failed (root=NULL)");
        free(param1a);
        free(param1b);
        return -1;
    }

    cJSON_AddStringToObject(root, "data", param1a);
    cJSON_AddStringToObject(root, "key", PK_ENCODE_AES_IV);

    // Convert to string
    ParseJson2 = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (ParseJson2 == NULL) {
        MAINLOG_L1("ERROR: cJSON_PrintUnformatted() failed (result=NULL)");
        free(param1a);
        free(param1b);
        return -1;
    }

    MAINLOG_L1("Second JSON created: %s", ParseJson2);

    // Encrypt second JSON
    MAINLOG_L1("Encrypting second JSON with PK_DEFAULT...");
    EncryptJson(ParseJson2, param1b, PK_DEFAULT);
    MAINLOG_L1("Second encryption result: %s", param1b);
    MAINLOG_L1("Encrypted data length: %d", strlen(param1b));
    free(ParseJson2);
    ParseJson2 = NULL;

    // Create final JSON for request body
    MAINLOG_L1("Creating final JSON for request...");
    root = cJSON_CreateObject();
    if (root == NULL) {
        MAINLOG_L1("ERROR: cJSON_CreateObject() failed (root=NULL)");
        free(param1a);
        free(param1b);
        return -1;
    }

    cJSON_AddStringToObject(root, "par", param1b);
    cJSON_AddStringToObject(root, "parmb", parmMB);

    body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (body == NULL) {
        MAINLOG_L1("ERROR: cJSON_PrintUnformatted() failed (result=NULL)");
        free(param1a);
        free(param1b);
        return -1;
    }

    MAINLOG_L1("Request body created: %s", body);
    MAINLOG_L1("Request body length: %04d bytes", strlen(body));

    // Initialize HTTP parameters
    MAINLOG_L1("Initializing HTTP parameters...");
    memset(&params, 0, sizeof(HTTP_UTILS_CONNECT_PARAMS));
    params.protocol = HTTPS_PROTOCOL;
    snprintf(params.domain, sizeof(params.domain), "%s", ENV_AC_APIHOST);
    snprintf(params.port, sizeof(params.port), "%s", ENV_AC_APIPORT);
    snprintf(params.type, sizeof(params.type), "%s", "POST");
    snprintf(params.request_suffix, sizeof(params.request_suffix), "%s", "/NFC/CU05SER2LZ");
    snprintf(params.version, sizeof(params.version), "%s", "HTTP/1.1");
    MAINLOG_L1("HTTP endpoint: %s:%s%s", params.domain, params.port, params.request_suffix);

    // Initialize HTTP headers
    MAINLOG_L1("Setting up HTTP headers...");
    memset(&header, 0, sizeof(HTTP_UTILS_REQUEST_HEADER));
    snprintf(header.accept, sizeof(header.accept), "%s", "*/*");
    snprintf(header.accept_encoding, sizeof(header.accept_encoding), "%s", "gzip, deflate, br");
    snprintf(header.content_type, sizeof(header.content_type), "%s", "application/json");
    snprintf(header.user_agent, sizeof(header.user_agent), "%s", "Mozilla/4.0(compatible; MSIE 5.5; Windows 98)");
    snprintf(header.connection, sizeof(header.connection), "%s", "close");

    // Allocate memory for result
    MAINLOG_L1("Allocating memory for response...");
    result = malloc(1024);
    if (result == NULL) {
        MAINLOG_L1("ERROR: Failed to allocate memory for result");
        free(param1a);
        free(param1b);
        free(body);
        return -1;
    }
    memset(result, 0, 1024);

    // Make HTTP request
    MAINLOG_L1("Sending HTTP request...");
    ret = httpRequestNFCPayment(&params, &header, result, body);
    if (ret == 0) {
        MAINLOG_L1("HTTP request successful, playing tip");
        AppPlayTip("Transaction processing");
        MAINLOG_L1("Response: %s", result);
    } else {
        MAINLOG_L1("HTTP request failed with code: %d", ret);
    }

    // Free allocated memory
    MAINLOG_L1("Cleaning up resources...");
    free(body);
    free(result);
    free(param1a);
    free(param1b);

    MAINLOG_L1("===== NFCPayment_Service completed with result: %d =====", ret);
    return ret;
}
