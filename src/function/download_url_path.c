/*
 * downlaod_url_path.c
 *
 *  Created on: 2021-10-14
 *  SY KIMLEAN: Modified on: 2025-03-03
 */
#include "httpDownload.h"
#include "def.h"
#include "http_utils.h"

#include <stdlib.h>
#include <coredef.h>
#include <struct.h>
#include <poslib.h>
#include <stdio.h>
#include <string.h>

#include <cJSON.h>

#define SHORT_TIMEOUT_MS 1000  // A shorter timeout to avoid blocking too long.
#define SMALL_RECV_BUF_SIZE 2048  // Buffer size for memory efficiency.

char* make_http_request(const char* ip, const char* port, const char* amount,
                        const char* currency, const char* sn, const char* timeStamp,
                        int volume, const char* token) {
    static char urlPath[512];  // Use static buffer to avoid dynamic allocations
    char responseBuf[SMALL_RECV_BUF_SIZE];

    memset(urlPath, 0, sizeof(urlPath));
    memset(responseBuf, 0, sizeof(responseBuf));

    // Prepare query string for GET request
    char *queryString[512] = {0};
    sprintf(queryString, "?amount=%s&currency=%s&device_id=%s&speaker_id=2&timestamp=%s&volume=%d&suffix=%d&access_token=%s",
                 amount, currency, sn, timeStamp, volume, THANKS_MODE, token);

	MAINLOG_L1("queryString => %s", queryString);

    // Set up connection parameters
	HTTP_UTILS_CONNECT_PARAMS connectParams;
	memset(&connectParams, 0, sizeof(HTTP_UTILS_CONNECT_PARAMS));
	connectParams.protocol = HTTP_PROTOCOL;
	sprintf(connectParams.domain, "%s", ip);
	sprintf(connectParams.port, "%s", port);
	sprintf(connectParams.type, "%s", "GET");
	sprintf(connectParams.request_suffix, "%s", "/generate-url");
	sprintf(connectParams.version, "%s", "HTTP/1.1");

	// Set up request header
	HTTP_UTILS_REQUEST_HEADER requestHeader;
	memset(&requestHeader, 0, sizeof(HTTP_UTILS_REQUEST_HEADER));

	sprintf(requestHeader.accept, "%s", "*/*");
	sprintf(requestHeader.accept_encoding, "%s", "gzip, deflate, br");
	sprintf(requestHeader.content_type, "%s", "application/json; charset=UTF-8");
	sprintf(requestHeader.user_agent, "%s", "Mozilla/4.0(compatible; MSIE 5.5; Windows 98)");
	sprintf(requestHeader.connection, "%s", "close");

    // Make the HTTP request
    int ret = http_utils_connect(&connectParams, &requestHeader, NULL, responseBuf, 1, queryString);

    if (ret < 0) {
        MAINLOG_L1("HTTP request failed with error: %d", ret);
        return NULL;
    }

    // Log the response for debugging
    MAINLOG_L1("Response: %.*s", 100, responseBuf); // Show first 100 chars for debugging

    // Parse JSON response
    cJSON* json = cJSON_Parse(responseBuf);
    if (!json) {
        MAINLOG_L1("Failed to parse JSON response");
        return NULL;
    }

    // Extract "url_audio_path"
    cJSON* urlItem = cJSON_GetObjectItem(json, "url_audio_path");
    if (urlItem && cJSON_IsString(urlItem)) {
        snprintf(urlPath, sizeof(urlPath), "%s", urlItem->valuestring);
        MAINLOG_L1("Successfully extracted URL: %s", urlPath);
    } else {
        MAINLOG_L1("URL path not found in response");
        cJSON_Delete(json);
        return NULL;
    }

    cJSON_Delete(json);
    return urlPath[0] ? urlPath : NULL;
}
