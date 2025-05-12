#include "def.h"
#include "http_utils.h"

#include <stdlib.h>
#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#include <cJSON.h>
#include <string.h>

static void *HTTP_CONNECT = NULL;

unsigned char HTTP_RESP_DATA_SMA_PACKET[DATA_PACKET_SMA_LEN];
unsigned char HTTP_RESP_DATA_MID_PACKET[DATA_PACKET_MID_LEN];
unsigned char HTTP_RESP_DATA_BIG_PACKET[DATA_PACKET_BIG_LEN];
unsigned char HTTP_RESP_DATA_SUP_PACKET[DATA_PACKET_BIG_LEN * 2];

unsigned char AES_ENCRYPTED_DATA[512];
unsigned char AES_KEY_DATA[512];
unsigned char AES_DECRYPTED_DATA[512];

int http_utils_parse_resp_result(char *result, char *parse_tag)
{
	cJSON *root = NULL;
	cJSON *obj  = NULL;

	root = cJSON_Parse(result);
	if (root == NULL)
	{
		// MAINLOG_L1("!!! cJSON_Parse() failed !!!");
		return -1;
	}

	obj = cJSON_GetObjectItem(root, parse_tag);
	if (obj == NULL)
	{
		char info[64] = "";
		sprintf(info, "!!! cJSON_GetObjectItem('%s') failed !!!", parse_tag);
		// MAINLOG_L1(info);

		cJSON_Delete(root);
		return -1;
	}

	int data_len = strlen(obj->valuestring);
	// MAINLOG_L1("data_len = ", data_len);

	if (data_len <= 0)
	{
		// MAINLOG_L1("!!! RESP DATA ERROR(data_len = %d) !!!", data_len);
		cJSON_Delete(root);
		return -1;
	}

	if (data_len <= DATA_PACKET_SMA_LEN)
	{
		memset(HTTP_RESP_DATA_SMA_PACKET, 0, 				sizeof(HTTP_RESP_DATA_SMA_PACKET));
		memcpy(HTTP_RESP_DATA_SMA_PACKET, obj->valuestring, data_len);

		// MAINLOG_L1(HTTP_RESP_DATA_SMA_PACKET);
	}
	else if (DATA_PACKET_SMA_LEN < data_len <= DATA_PACKET_MID_LEN)
	{
		memset(HTTP_RESP_DATA_MID_PACKET, 0, 			    sizeof(HTTP_RESP_DATA_MID_PACKET));
		memcpy(HTTP_RESP_DATA_MID_PACKET, obj->valuestring, data_len);

		// MAINLOG_L1(HTTP_RESP_DATA_MID_PACKET);
	}
	else if (DATA_PACKET_MID_LEN < data_len <= DATA_PACKET_BIG_LEN)
	{
		memset(HTTP_RESP_DATA_BIG_PACKET, 0, 				sizeof(HTTP_RESP_DATA_BIG_PACKET));
		memcpy(HTTP_RESP_DATA_BIG_PACKET, obj->valuestring, data_len);

		// MAINLOG_L1(HTTP_RESP_DATA_BIG_PACKET);
	}
	else {
		memset(HTTP_RESP_DATA_SUP_PACKET, 0, 			    sizeof(HTTP_RESP_DATA_SUP_PACKET));
		memcpy(HTTP_RESP_DATA_SUP_PACKET, obj->valuestring, data_len);

		// MAINLOG_L1(HTTP_RESP_DATA_SUP_PACKET);
	}

	cJSON_Delete(root);

	return 0;
}

// ===================== AES Verify
int http_utils_parse_aesDe_resp_result(char *result)
{
	cJSON *root = NULL;
	cJSON *obj  = NULL;

	root = cJSON_Parse(result);
	if (root == NULL) {
		// MAINLOG_L1("!!! cJSON_Parse() failed !!!");
		return -1;
	}

	// decrypted data
	obj = cJSON_GetObjectItem(root, "decrypted_data");
	if (obj == NULL) {
		// MAINLOG_L1("!!! cJSON_GetObjectItem('decrypted_data') failed !!!");
		cJSON_Delete(root);
		return -1;
	}

	char *data = NULL;
	data = cJSON_PrintUnformatted(obj);
	if (data == NULL) {
		// MAINLOG_L1("!!! cJSON_PrintUnformatted() failed(data = NULL) !!!");
		cJSON_Delete(root);
		return -1;
	}

	int data_len = strlen(data);
	// MAINLOG_L1("data_len = ", data_len);

	if (data_len <= 0) {
		// MAINLOG_L1("!!! RESP DATA ERROR(decrypted_data_len = %d) !!!", data_len);
		cJSON_Delete(root);
		return -1;
	}

	memset(AES_DECRYPTED_DATA, 0,    sizeof(AES_DECRYPTED_DATA));
	memcpy(AES_DECRYPTED_DATA, data, data_len);

	// MAINLOG_L1("AES_DECRYPTED_DATA:\n", AES_DECRYPTED_DATA);
	cJSON_Delete(root);

	return 0;
}

int http_utils_parse_aesEn_resp_result(char *result)
{
	cJSON *root = NULL;
	cJSON *obj1  = NULL, *obj2 = NULL;

	root = cJSON_Parse(result);
	if (root == NULL) {
		// MAINLOG_L1("!!! cJSON_Parse() failed !!!");
		return -1;
	}

	// encrypted data
	obj1 = cJSON_GetObjectItem(root, "encrypted_data");
	if (obj1 == NULL) {
		// MAINLOG_L1("!!! cJSON_GetObjectItem('encrypted_data') failed !!!");
		cJSON_Delete(root);
		return -1;
	}

	int data_len = strlen(obj1->valuestring);
	// MAINLOG_L1("data_len = ", data_len);

	if (data_len <= 0) {
		// MAINLOG_L1("!!! RESP DATA ERROR(encrypted_data_len = %d) !!!", data_len);
		cJSON_Delete(root);
		return -1;
	}

	// Need Transform
	char encrypted_data[data_len + 2];
	memset(encrypted_data, 0, sizeof(encrypted_data));
	memcpy(encrypted_data, obj1->valuestring, data_len);

	// MAINLOG_L1("=============== ORIGINAL ENCRYPTED DATA ===============");
	// MAINLOG_L1(encrypted_data);
	// MAINLOG_L1("=======================================================");

	// ========== transform
	memset(AES_ENCRYPTED_DATA, 0, sizeof(AES_ENCRYPTED_DATA));

	for (int i = 0; i < strlen(encrypted_data); ++i) {
		if (encrypted_data[i] == '+') {
			strcat(AES_ENCRYPTED_DATA, "%2B");
		}
		else if (encrypted_data[i] == '/') {
			strcat(AES_ENCRYPTED_DATA, "%2F");
		}
		else if (encrypted_data[i] == '=') {
			strcat(AES_ENCRYPTED_DATA, "%3D");
		}
		else {
			char tmp[2] = "";
			sprintf(tmp, "%c", encrypted_data[i]);
			strcat(AES_ENCRYPTED_DATA, tmp);
		}
	}
	// ==========

	// MAINLOG_L1("=============== ENCRYPTED DATA AFTER TRANSFORMED ===============");
	// MAINLOG_L1(AES_ENCRYPTED_DATA);
	// MAINLOG_L1("================================================================");

	memset(encrypted_data, 0, sizeof(encrypted_data));
	/*
	memset(AES_ENCRYPTED_DATA, 0, sizeof(AES_ENCRYPTED_DATA));
	memcpy(AES_ENCRYPTED_DATA, obj1->valuestring, data_len);

	// MAINLOG_L1("=============== ENCRYPTED DATA ===============");
	// MAINLOG_L1(AES_ENCRYPTED_DATA);
	// MAINLOG_L1("==============================================");
	*/

	data_len = 0;

	// key
	obj2 = cJSON_GetObjectItem(root, "key");
	if (obj2 == NULL) {
		// MAINLOG_L1("!!! cJSON_GetObjectItem('key') failed !!!");
		cJSON_Delete(root);
		return -1;
	}

	data_len = strlen(obj2->valuestring);
	// MAINLOG_L1("data_len = ", data_len);

	if (data_len <= 0) {
		// MAINLOG_L1("!!! RESP DATA ERROR(key_data_len = %d) !!!", data_len);
		cJSON_Delete(root);
		return -1;
	}

	// Need Transform
	char key_data[data_len + 2];
	memset(key_data, 0, sizeof(key_data));
	memcpy(key_data, obj2->valuestring, data_len);

	// MAINLOG_L1("=============== ORIGINAL KEY DATA ===============");
	// MAINLOG_L1(key_data);
	// MAINLOG_L1("=================================================");

	// ==========
	memset(AES_KEY_DATA, 0, sizeof(AES_KEY_DATA));

	for (int i = 0; i < strlen(key_data); ++i) {
		if (key_data[i] == '+') {
			strcat(AES_KEY_DATA, "%2B");
		}
		else if (key_data[i] == '/') {
			strcat(AES_KEY_DATA, "%2F");
		}
		else if (key_data[i] == '=') {
			strcat(AES_KEY_DATA, "%3D");
		}
		else {
			char tmp[2] = "";
			sprintf(tmp, "%c", key_data[i]);
			strcat(AES_KEY_DATA, tmp);
		}
	}
	// ==========

	// MAINLOG_L1("=============== KEY DATA AFTER TRANSFORMED ===============");
	// MAINLOG_L1(AES_KEY_DATA);
	// MAINLOG_L1("==========================================================");

	memset(key_data, 0, sizeof(key_data));

	/*
	memset(AES_KEY_DATA, 0, sizeof(AES_KEY_DATA));
	memcpy(AES_KEY_DATA, obj2->valuestring, data_len);

	// MAINLOG_L1("=============== KEY DATA ===============");
	// MAINLOG_L1(AES_KEY_DATA);
	// MAINLOG_L1("========================================");
	*/

	cJSON_Delete(root);

	return 0;
}
// =====================

// ===================== CHUNK
char *read_chunk(char *body_data, int size)
{
	char *chunk_end = strstr(body_data, "\r\n");

	// FIXED ME: CHANGE CONDITION
	//if (chunk_end == NULL || (chunk_end - body_data) != size) { return NULL; }

	if (chunk_end == NULL) { return NULL; }

	chunk_end += 2;

	return chunk_end;
}

char *read_chunk_size(char *body_data)
{
	char *size_end = strstr(body_data, "\r\n");
	if (size_end == NULL) {
		// MAINLOG_L1("!!! READ CHUNK SIZE ERROR !!!");
		return NULL;
	}

	*size_end = '\0';
	int size = strtol(body_data, NULL, 16);
	*size_end = '\r';

	return size_end + 2;
}
// =====================

int http_utils_parse_header(char *data, char *result, int *is_body_size_ok)
{
    if (!data || !result || !is_body_size_ok) {
        MAINLOG_L1("Invalid parameters passed to http_utils_parse_header");
        return 0;
    }

    *is_body_size_ok = 0;

    HTTP_UTILS_RESPONSE *resp = (HTTP_UTILS_RESPONSE*)malloc(sizeof(HTTP_UTILS_RESPONSE));
    if (!resp) {
        MAINLOG_L1("Memory allocation failed for response");
        return 0;
    }

    HTTP_UTILS_RESPONSE *resp_tmp = (HTTP_UTILS_RESPONSE*)malloc(sizeof(HTTP_UTILS_RESPONSE));
    if (!resp_tmp) {
        MAINLOG_L1("Memory allocation failed for temp response");
        free(resp);
        return 0;
    }

    memset(resp, 0, sizeof(HTTP_UTILS_RESPONSE));
    memset(resp_tmp, 0, sizeof(HTTP_UTILS_RESPONSE));

    char *start = data;
    MAINLOG_L1("DATA ==> %s", data);

    // Get Response Status
    for (; *start && *start != '\r'; ++start) {
        if (resp_tmp->version == NULL) {
            resp_tmp->version = start;
        }

        if (*start == ' ') {
            if (resp_tmp->code == NULL) {
                resp_tmp->code = start + 1;
            } else {
                resp_tmp->desc = start + 1;
            }
            *start = '\0';
        }
    }

    if (!*start) {
        MAINLOG_L1("Invalid HTTP response format: Missing CRLF after status line");
        free(resp_tmp);
        free(resp);
        return 0;
    }

    *start = '\0';
    start += 2; // Skip '\r\n'

    // Get Response Body Size From Response Header
    char *line = start;
    int is_data_chunk = 0;

    while (*line && *line != '\r') {
        char *key, *value;

        while (*start && *start != ':') start++;
        if (!*start) break; // End of data

        *start = '\0'; // Null-terminate the key
        key = line;
        value = ++start; // Start of value

        // Skip leading spaces in the value
        while (*value == ' ') value++;

        while (*start && *start != '\r') start++;
        if (!*start || *(start+1) != '\n') {
            MAINLOG_L1("Invalid HTTP header format: Missing CRLF");
            free(resp_tmp);
            free(resp);
            return 0;
        }

        *start = '\0'; // Null-terminate the value
        start += 2; // Skip '\r\n'
        line = start;

        if (strcasecmp(key, "Content-Length") == 0) {
            resp_tmp->bodySize = atoi(value);
        } else if (strcasecmp(key, "Transfer-Encoding") == 0 &&
                  strcasecmp(value, "chunked") == 0) {
            is_data_chunk = 1;
        }
    }

    // Get Response Body
    if (*line == '\r' && *(line+1) == '\n') {
        line += 2;
        resp_tmp->body = line;
    }

    // Copy parsed data safely
    if (resp_tmp->version) resp->version = strdup(resp_tmp->version);
    if (resp_tmp->code) resp->code = strdup(resp_tmp->code);
    if (resp_tmp->desc) resp->desc = strdup(resp_tmp->desc);
    if (resp_tmp->body) resp->body = strdup(resp_tmp->body);
    resp->bodySize = resp_tmp->bodySize;

    free(resp_tmp);

    if (!resp->code) {
        MAINLOG_L1("HTTP response parsing failed: Missing status code");
        free(resp);
        return 0;
    }

    MAINLOG_L1("=============== PARSE HTTP RESPONSE ===============");
    char tmp[512] = {0};
    snprintf(tmp, sizeof(tmp), "%s %s %s",
            resp->version ? resp->version : "(NULL)",
            resp->code ? resp->code : "(NULL)",
            resp->desc ? resp->desc : "(NULL)");
    MAINLOG_L1(tmp);

    if (strcmp(resp->code, "200") != 0) {
        char info[64] = {0};
        snprintf(info, sizeof(info), "!!! HTTP REQUEST FAILED(%s) !!!", resp->code);
        MAINLOG_L1(info);

        // Free allocated resources before return
        if (resp->version) free(resp->version);
        if (resp->code) free(resp->code);
        if (resp->desc) free(resp->desc);
        if (resp->body) free(resp->body);
        free(resp);
        return 0;
    }

    if (is_data_chunk) {
        char *result_buffer = malloc(RECEIVE_BUF_SIZE);
        if (!result_buffer) {
            MAINLOG_L1("Memory allocation failed for chunk reassembly");
            // Free resources and return
            return 0;
        }
        memset(result_buffer, 0, RECEIVE_BUF_SIZE);
        size_t result_size = 0;

        char *current_pos = resp->body;

        while (1) {
            // Skip any whitespace
            while (*current_pos && (*current_pos == ' ' || *current_pos == '\r' || *current_pos == '\n'))
                current_pos++;

            // Parse the chunk size (in hex)
            char *end_ptr;
            int chunk_size = strtol(current_pos, &end_ptr, 16);

            // If we couldn't parse the chunk size or it's 0, we're done
            if (end_ptr == current_pos || chunk_size == 0)
                break;

            // Move to the data portion (after the CRLF)
            current_pos = strstr(current_pos, "\r\n");
            if (!current_pos) {
                MAINLOG_L1("Invalid chunk format - missing CRLF after size");
                free(result_buffer);
                return 0;
            }
            current_pos += 2; // Skip CRLF

            // Copy this chunk's data
            memcpy(result_buffer + result_size, current_pos, chunk_size);
            result_size += chunk_size;

            // Move to next chunk
            current_pos += chunk_size + 2; // +2 for CRLF after data
        }

        // Copy assembled data to result buffer
        if (result_size > 0) {
            memcpy(result, result_buffer, result_size);
            result[result_size] = '\0';  // Make sure it's null-terminated
            *is_body_size_ok = 1;
        }

        free(result_buffer);
    } else {
        MAINLOG_L1("RESP_BODY_SIZE = %d", resp->bodySize);
        if (resp->bodySize > 0) {
            if (resp->body)
            {
                memcpy(result, resp->body, resp->bodySize);
                *is_body_size_ok = 1;
            } else
            {
                MAINLOG_L1("Response has body size but no body content");
                *is_body_size_ok = 0;
            }
        } else {
            MAINLOG_L1("Response has no body content (zero length)");
            *is_body_size_ok = 0;
        }
    }

    // Free allocated resources before return
    if (resp->version) free(resp->version);
    if (resp->code) free(resp->code);
    if (resp->desc) free(resp->desc);
    if (resp->body) free(resp->body);
    free(resp);

    return 1;
}

int http_utils_connect(
	HTTP_UTILS_CONNECT_PARAMS *params,
	HTTP_UTILS_REQUEST_HEADER *header,
	char *data,
	char *result,
	int isQueryType,
	char *queries)
{
	int ret, errCode = -1;
	int reConnectCount = 1;
	int reSendDataCount = 0;
	int reReceiveDataCount = 0;
	int timeout = _IS_GPRS_ENABLED_ ? TIMEOUT : 2 * TIMEOUT;

	while (1)
	{
		MAINLOG_L1("Entering main connection loop...");

		// Simplified connection logic with common timeout pattern
		int secureFlag = (params->protocol == HTTP_PROTOCOL) ? 0 :
		                 (params->protocol == HTTPS_PROTOCOL) ? 1 : 2;

		// Check if we already have a valid API socket
		if (g_api_socket != NULL && g_api_socket->valid &&
			strcmp(g_api_socket->host, params->domain) == 0 &&
			strcmp(g_api_socket->port, params->port) == 0) {
			
			HTTP_CONNECT = g_api_socket;
			errCode = 0;
			MAINLOG_L1("Reusing existing API socket connection");
		}
		else {
			// Create a new connection
			HTTP_CONNECT = net_connect(NULL, params->domain, params->port, timeout, secureFlag, &errCode);
			
			// Store in global reference if successful
			if (HTTP_CONNECT != NULL && errCode == 0) {
				g_api_socket = HTTP_CONNECT;
				MAINLOG_L1("Stored new API socket connection");
			}
		}
		
		if (HTTP_CONNECT != NULL && errCode == 0)
		{
			reConnectCount = 0;
			MAINLOG_L1("Connection established. Preparing buffers...");

			// Allocate buffers
			unsigned char *s_packBuf = malloc(DATA_BUF_SIZE);
			if (s_packBuf == NULL) {
				MAINLOG_L1("!!! s_packBuf MALLOC Failed !!!");
				return -1;
			}

			unsigned char *r_packBuf = malloc(RECEIVE_BUF_SIZE);
			if (r_packBuf == NULL) {
				MAINLOG_L1("!!! r_packBuf MALLOC Failed !!!");
				free(s_packBuf);
				return -1;
			}

			memset(s_packBuf, 0, DATA_BUF_SIZE);
			memset(r_packBuf, 0, RECEIVE_BUF_SIZE);

			MAINLOG_L1("Buffers initialized. Preparing request...");

			// Build HTTP request header
			if (!isQueryType) {
				// Non-query request
				int requestLen = snprintf((char *)s_packBuf, DATA_BUF_SIZE,
					"%s %s %s\r\n", params->type, params->request_suffix, params->version);
				if (requestLen < 0 || requestLen >= DATA_BUF_SIZE) {
					MAINLOG_L1("!!! Request buffer overflow !!!");
					free(s_packBuf);
					free(r_packBuf);
					net_close(HTTP_CONNECT);
					HTTP_CONNECT = NULL;
					return -1;
				}
			} else {
				// Query request
				int requestLen = snprintf((char *)s_packBuf, DATA_BUF_SIZE,
					"%s http://%s:%s%s%s %s\r\n",
					params->type, params->domain, params->port,
					params->request_suffix, queries, params->version);
				if (requestLen < 0 || requestLen >= DATA_BUF_SIZE) {
					MAINLOG_L1("!!! Request buffer overflow !!!");
					free(s_packBuf);
					free(r_packBuf);
					net_close(HTTP_CONNECT);
					HTTP_CONNECT = NULL;
					return -1;
				}
			}

			// Append HTTP headers more efficiently using snprintf
			int headerLen = strlen((char *)s_packBuf);
			int remainingSpace = DATA_BUF_SIZE - headerLen;
			char *currentPos = (char *)s_packBuf + headerLen;

			// Add standard headers
			int addLen = snprintf(currentPos, remainingSpace,
				"Accept: %s\r\n"
				"Accept-Encoding: %s\r\n"
				"Host: %s\r\n"
				"Content-Type: %s\r\n"
				"User-Agent: %s\r\n"
				"Cache-Control: no-cache\r\n"
				"Connection: %s\r\n"
				"Content-Length: %d\r\n\r\n",
				header->accept,
				header->accept_encoding,
				params->domain,
				header->content_type,
				header->user_agent,
				header->connection,
				data ? (int)strlen(data) : 0);

			if (addLen < 0 || addLen >= remainingSpace) {
				MAINLOG_L1("!!! Header buffer overflow !!!");
				free(s_packBuf);
				free(r_packBuf);
				net_close(HTTP_CONNECT);
				HTTP_CONNECT = NULL;
				return -1;
			}

			currentPos += addLen;
			remainingSpace -= addLen;

			// Add data if present
			if (data != NULL) {
				int dataLen = strlen(data);
				if (dataLen >= remainingSpace) {
					MAINLOG_L1("!!! Data buffer overflow !!!");
					free(s_packBuf);
					free(r_packBuf);
					net_close(HTTP_CONNECT);
					HTTP_CONNECT = NULL;
					return -1;
				}
				memcpy(currentPos, data, dataLen);
				currentPos += dataLen;
			}

			int packLen = currentPos - (char *)s_packBuf;
			MAINLOG_L1("PACK_LEN = %d", packLen);

			// Debug Print
			MAINLOG_L1("========== HTTP SEND DATA ==========");
			MAINLOG_L1((char *)s_packBuf);
			MAINLOG_L1("====================================");

			// Send request
			ret = net_write(HTTP_CONNECT, s_packBuf, packLen, timeout);
			MAINLOG_L1("net_write(): %d ", ret);

			if (ret < 0) {
				reSendDataCount++;

				MAINLOG_L1("!!! SEND(%d) PACK DATA TO SERVER FAILED(%d) !!!", reSendDataCount, ret);

				if (reSendDataCount >= 3) {
					net_close(HTTP_CONNECT);
					HTTP_CONNECT = NULL;
					free(s_packBuf);
					free(r_packBuf);
					return -1;
				}
			} else {
				// Read response
				ret = net_read(HTTP_CONNECT, r_packBuf, RECEIVE_BUF_SIZE, timeout);

				if (ret <= 0) {
					reReceiveDataCount++;

					MAINLOG_L1("!!! RECEIVE(%d) PACK DATA FROM SERVER FAILED(%d) !!!",
						reReceiveDataCount, ret);

					if (reReceiveDataCount >= 3) {
						net_close(HTTP_CONNECT);
						HTTP_CONNECT = NULL;
						free(s_packBuf);
						free(r_packBuf);
						return -1;
					}
				} else {
					int data_len1 = strlen((char *)r_packBuf);
					MAINLOG_L1("RECEIVE(1) DATA LEN = %d", data_len1);

					if (r_packBuf != NULL && data_len1 > 0) {
						MAINLOG_L1("************* RECEIVE(1) DATA **************");
						MAINLOG_L1((char *)r_packBuf);
						MAINLOG_L1("********************************************");

						int is_body_size_ok = 1;
						ret = http_utils_parse_header(r_packBuf, result, &is_body_size_ok);

						if (!ret) {
							MAINLOG_L1("!!! PARSE DATA ERROR !!!");
							net_close(HTTP_CONNECT);
							HTTP_CONNECT = NULL;
							free(s_packBuf);
							free(r_packBuf);
							return -1;
						} else {
							MAINLOG_L1("is_body_size_ok = %d", is_body_size_ok);

							if (!is_body_size_ok) {
								// Need to receive more data
								free(r_packBuf); // Free first buffer before allocating second

								unsigned char *r_packBuf2 = malloc(RECEIVE_BUF_SIZE);
								if (r_packBuf2 == NULL) {
									MAINLOG_L1("!!! r_packBuf2 MALLOC Failed !!!");
									net_close(HTTP_CONNECT);
									HTTP_CONNECT = NULL;
									free(s_packBuf);
									return -1;
								}
								memset(r_packBuf2, 0, RECEIVE_BUF_SIZE);

								int receive_again_count = 0;

								do {
									ret = net_read(HTTP_CONNECT, r_packBuf2, RECEIVE_BUF_SIZE, timeout);

									if (ret <= 0) {
										receive_again_count++;
										MAINLOG_L1("!!! RECEIVE AGAIN(%d) FAILED(%d) !!!",
											receive_again_count, ret);

										if (receive_again_count >= 3) {
											MAINLOG_L1("!!! RECEIVE AGAIN FAILED !!!");
											net_close(HTTP_CONNECT);
											HTTP_CONNECT = NULL;
											free(s_packBuf);
											free(r_packBuf2);
											return -1;
										}
									}
								} while (ret <= 0 && receive_again_count < 3);

								if (ret > 0) {
									int data_len2 = strlen((char *)r_packBuf2);
									MAINLOG_L1("RECEIVE(2) DATA LEN = %d", data_len2);

									if (r_packBuf2 != NULL && data_len2 >= 0) {
										MAINLOG_L1("************* RECEIVE(2) DATA **************");
										MAINLOG_L1((char *)r_packBuf2);
										MAINLOG_L1("********************************************");

										memcpy(result, r_packBuf2, data_len2);

										net_close(HTTP_CONNECT);
										HTTP_CONNECT = NULL;
										free(s_packBuf);
										free(r_packBuf2);
										return 0;
									} else {
										MAINLOG_L1("!!! RECEIVE AGAIN DATA ERROR !!!");
										net_close(HTTP_CONNECT);
										HTTP_CONNECT = NULL;
										free(s_packBuf);
										free(r_packBuf2);
										return -1;
									}
								}
							} else {
								net_close(HTTP_CONNECT);
								HTTP_CONNECT = NULL;
								free(s_packBuf);
								free(r_packBuf);
								return 0;
							}
						}
					} else {
						MAINLOG_L1("!!! RECEIVE DATA ERROR !!!");
						net_close(HTTP_CONNECT);
						HTTP_CONNECT = NULL;
						free(s_packBuf);
						free(r_packBuf);
						return -1;
					}
				}
			}

			// Clean up in case we get here from a continue statement
			free(s_packBuf);
			free(r_packBuf);
		} else {
			if (reConnectCount > 3) {
				MAINLOG_L1("!!! CAN NOT CONNECT TO SERVER (%d) !!!", errCode);
				net_close(HTTP_CONNECT);
				return -1;
			} else {
				// Retry connection
				MAINLOG_L1("ERR_CODE = %d", errCode);
				MAINLOG_L1("!!! TRY CONNECT TO SERVER (%d) COUNT !!!", reConnectCount);
				reConnectCount++;
			}
		}

		Delay_Api(5 * 1000);
	}
}
