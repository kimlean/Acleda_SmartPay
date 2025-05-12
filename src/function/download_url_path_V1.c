//#include <sys/stat.h>
//#include <unistd.h>
//#include "httpDownload.h"
//#include <cJSON.h>
//#include "def.h"
//
//#define SHORT_TIMEOUT_MS 1000  // A shorter timeout to avoid blocking too long.
//#define SMALL_RECV_BUF_SIZE 2048  // Reduce buffer size for memory efficiency.
//static void* netContext = NULL;
//
///**
// * @brief Send the entire buffer in small chunks, handling partial writes.
// */
//static int send_all(void* context, const unsigned char* buffer, int length) {
//    int totalSent = 0;
//    while (totalSent < length) {
//        int sent = net_write(context, buffer + totalSent, length - totalSent, SHORT_TIMEOUT_MS);
//        if (sent <= 0) return -1;  // Timeout or error
//        totalSent += sent;
//    }
//    return totalSent;
//}
//
///**
// * @brief Read response in smaller chunks until server closes or buffer is full.
// */
//static int read_all_chunks(void* context, unsigned char* buffer, int bufferSize) {
//    int totalRead = 0;
//    while (totalRead < bufferSize - 1) {
//        int recvLen = net_read(context, buffer + totalRead, bufferSize - 1 - totalRead, SHORT_TIMEOUT_MS);
//        if (recvLen < 0) return -1;  // Error or timeout
//        if (recvLen == 0) break;     // Server closed the connection
//        totalRead += recvLen;
//    }
//    buffer[totalRead] = '\0';  // Null-terminate
//    return totalRead;
//}
//
///**
// * @brief Makes a single HTTP GET request and returns the parsed "url_audio_path".
// */
//char* make_http_request(const char* ip, const char* port, const char* amount,
//                        const char* currency, const char* sn, const char* timeStamp,
//                        int volume, const char* token) {
//    unsigned char recvBuf[SMALL_RECV_BUF_SIZE];  // Reduced buffer size for memory efficiency
//    static char urlPath[512];  // Use static buffer to avoid dynamic allocations
//    memset(urlPath, 0, sizeof(urlPath));
//
//    // Close existing context if not reusing
//    if (netContext) {
//        net_close(netContext);
//        netContext = NULL;
//    }
//
//    // Connect to server
//    int errCode;
//    netContext = net_connect(NULL, ip, port, SHORT_TIMEOUT_MS, 0, &errCode);
//    if (!netContext || errCode != 0) return NULL;
//
//    // Prepare HTTP GET request
//    char request[512];
//    snprintf(request, sizeof(request),
//             "GET /generate-url?amount=%s&currency=%s&device_id=%s&speaker_id=2&timestamp=%s"
//             "&volume=%d&access_token=%s HTTP/1.1\r\nHost: %s:%s\r\n"
//             "Accept: application/json\r\nConnection: close\r\n\r\n",
//             amount, currency, sn, timeStamp, volume, token, ip, port);
//
//    // Send request
//    if (send_all(netContext, (const unsigned char*)request, strlen(request)) < 0) {
//        net_close(netContext);
//        return NULL;
//    }
//
//    // Read response
//    if (read_all_chunks(netContext, recvBuf, sizeof(recvBuf)) < 0) {
//        net_close(netContext);
//        return NULL;
//    }
//
//    // Parse HTTP body
//    char* body = strstr((char*)recvBuf, "\r\n\r\n");
//    if (!body) {
//        net_close(netContext);
//        return NULL;
//    }
//    body += 4;
//
//    // Parse JSON
//    cJSON* json = cJSON_Parse(body);
//    if (!json) {
//        net_close(netContext);
//        return NULL;
//    }
//
//    // Extract "url_audio_path"
//    cJSON* urlItem = cJSON_GetObjectItem(json, "url_audio_path");
//    if (urlItem && cJSON_IsString(urlItem)) {
//        snprintf(urlPath, sizeof(urlPath), "%s", urlItem->valuestring);
//    }
//
//    cJSON_Delete(json);
//    net_close(netContext);
//    return urlPath[0] ? urlPath : NULL;
//}
