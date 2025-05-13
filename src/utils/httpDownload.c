#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#include "httpDownload.h"

#define	PROTOCOL_HTTP	0
#define	PROTOCOL_HTTPS	1

typedef struct {
	int protocol;
	char domain[MAX_DOMAIN_LENGTH];
	char path[MAX_PATH_LENGTH];
	char port[8];
} URL_ENTRY;

static int parseURL(char *url, URL_ENTRY *entry)
{
	char *p, *p2;

	memset(entry, 0, sizeof(URL_ENTRY));

	// Protocol analysis
	p = strstr(url, "://");
	if (p == NULL) {
		entry->protocol = PROTOCOL_HTTP;
		p = url;
	}
	else {
		if (((p - url) == 5) && (memcmp(url, "https", 5) == 0))
			entry->protocol = PROTOCOL_HTTPS;
		else if (((p - url) == 4) && (memcmp(url, "http", 4) == 0))
			entry->protocol = PROTOCOL_HTTP;
		else
			return UNSUPPORTED_URL;

		p += 3;
	}

	// Domain analysis (now p points to the beginning of domain name)
	p2 = strchr(p, '/');
	if (p2 == NULL) {
		// Format as http://domain
		if (strlen(p) >= MAX_DOMAIN_LENGTH)
			return URL_DOMAIN_TOO_LONG;

		strcpy(entry->domain, p);
	}
	else {
		if ((p2 - p) >= MAX_DOMAIN_LENGTH)
			return URL_DOMAIN_TOO_LONG;

		memcpy(entry->domain, p, p2 - p);
		entry->domain[p2 - p] = 0;
	}

	// Path analysis (now p2 is either NULL or pointing to the begining of path)
	if (p2 == NULL)
		strcpy(entry->path, "/");
	else {
		if (strlen(p2) >= MAX_PATH_LENGTH)
			return URL_PATH_TOO_LONG;

		strcpy(entry->path, p2);
	}

	// Port analysis (may be part of domain)
	p2 = strchr(entry->domain, ':');
	if (p2 == NULL) {
		if (entry->protocol == PROTOCOL_HTTP)
			strcpy(entry->port, "80");
		else if (entry->protocol == PROTOCOL_HTTPS)
			strcpy(entry->port, "443");
	}
	else {
		p2[0] = 0;	// cut the ":port" from domain

		strcpy(entry->port, p2 + 1);
	}

	return 0;
}

// Remove the space in the begining and end of the str
static char *trim(char *str)
{
	int length, start, end;

	length = strlen(str);

	for (start = 0; start < length; start++) {
		if (str[start] != ' ')
			break;
	}

	for (end = length - 1; end > start; end--) {
		if (str[end] != ' ')
			break;
	}

	str[end + 1] = 0;

	return str + start;
}


// Use a smaller buffer size to reduce memory usage
#define OPTIMIZED_BUF_SIZE (1024 * 4)

// Partial download
// Return content length downloaded or error
static int __httpDownload(char *url, int method, char *filename, int start, int end)
{
    URL_ENTRY entry;
    int ret = 0, length = 0, total = 0, offset = 0;
    int payload_flag = 0;
    int chunked_total = 0;
    char *p, *p1, *p2;
    void *netContext = NULL;

    unsigned char *buf = NULL;

    MAINLOG_L1("Starting download: URL=%s, method=%d, file=%s, range=%d-%d",
               url, method, filename, start, end);

    // Allocate buffer for HTTP operations
    buf = (unsigned char*)malloc(OPTIMIZED_BUF_SIZE);
    if (!buf) {
        MAINLOG_L1("Memory allocation failed for buffer");
        return -10; // Memory allocation failure
    }

    // Clear buffer before use
    memset(buf, 0, OPTIMIZED_BUF_SIZE);

    // Parse URL with improved error handling
    ret = parseURL(url, &entry);
    MAINLOG_L1("parseURL result: %d, domain=%s, path=%s",
               ret, entry.domain, entry.path);

    if (ret != 0) {
        MAINLOG_L1("URL parsing failed with error: %d", ret);
        free(buf);
        return ret;
    }

    // Buffer for holding file data - allocate only when needed
    unsigned char *file_buffer = NULL;

relocate_301:
    {
        // Establish connection with timeout
        int errCode = 0;
        int connect_retries = 0;
        const int MAX_CONNECT_RETRIES = 3;

        while (connect_retries < MAX_CONNECT_RETRIES) {
            MAINLOG_L1("Connecting to %s (attempt %d)...", entry.domain, connect_retries + 1);

            netContext = net_connect(NULL, entry.domain, entry.port, CONNECT_TIMEOUT * 1000,
                                    entry.protocol == PROTOCOL_HTTPS ? 1 : 0, &errCode);

            if (netContext) break;

            MAINLOG_L1("Connection attempt failed with error: %d", errCode);
            connect_retries++;

            if (connect_retries < MAX_CONNECT_RETRIES) {
                Delay_Api(1000 * connect_retries); // Progressive backoff
            }
        }

        if (!netContext) {
            MAINLOG_L1("All connection attempts failed");
            free(buf);
            if (file_buffer) free(file_buffer);
            return CONNECT_ERROR;
        }
    }

    MAINLOG_L1("Connection established successfully");

    // Prepare HTTP request directly into buffer
    if (end < 0) {
        length = snprintf((char *)buf, OPTIMIZED_BUF_SIZE,
                      "%s %s HTTP/1.1\r\nHost:%s\r\nConnection:keep-alive\r\nRange:bytes=%d-\r\n\r\n",
                      (method == METHOD_GET) ? "GET" : "POST",
                      entry.path,
                      entry.domain,
                      start);
    } else {
        length = snprintf((char *)buf, OPTIMIZED_BUF_SIZE,
                      "%s %s HTTP/1.1\r\nHost:%s\r\nConnection:keep-alive\r\nRange:bytes=%d-%d\r\n\r\n",
                      (method == METHOD_GET) ? "GET" : "POST",
                      entry.path,
                      entry.domain,
                      start, end);
    }

    // Ensure buffer wasn't truncated
    if (length >= OPTIMIZED_BUF_SIZE) {
        MAINLOG_L1("Request too large for buffer (%d bytes needed)", length);
        net_close(netContext);
        free(buf);
        return -10; // Request too large
    }

    MAINLOG_L1("Sending request: %s", buf);

    // Send the HTTP request with retry logic
    int send_retries = 0;
    const int MAX_SEND_RETRIES = 2;

    while (send_retries <= MAX_SEND_RETRIES) {
        ret = net_write(netContext, buf, length, CONNECT_TIMEOUT);
        if (ret >= 0) break;

        MAINLOG_L1("Send failed with error %d, retry %d/%d",
                  ret, send_retries+1, MAX_SEND_RETRIES);
        send_retries++;

        if (send_retries <= MAX_SEND_RETRIES) {
            Delay_Api(300 * send_retries);
        }
    }

    if (ret < 0) {
        MAINLOG_L1("Failed to send request after retries, error: %d", ret);
        net_close(netContext);
        free(buf);
        return SEND_ERROR;
    }

    MAINLOG_L1("Request sent successfully");

    // Receive and process HTTP response header
    length = 0;
    MAINLOG_L1("Waiting for response...");

    // Read response headers chunk by chunk
    while (1) {
        // Reset buffer for receiving if it's full
        if (length >= OPTIMIZED_BUF_SIZE - 1) {
            MAINLOG_L1("Response header too large for buffer");
            net_close(netContext);
            free(buf);
            return RECEIVE_ERROR;
        }

        int read_size = OPTIMIZED_BUF_SIZE - 1 - length;
        ret = net_read(netContext, buf + length, read_size, RECEIVE_TIMEOUT * 1000);

        if (ret < 0) {
            MAINLOG_L1("Failed to receive response header, error: %d", ret);
            net_close(netContext);
            free(buf);
            return RECEIVE_ERROR;
        }

        if (ret == 0) {
            MAINLOG_L1("Server closed connection before sending complete headers");
            net_close(netContext);
            free(buf);
            return RECEIVE_ERROR;
        }

        length += ret;
        buf[length] = '\0'; // Null-terminate for string operations
        MAINLOG_L1("Received %d bytes of response data", ret);

        // Find end of status line
        p = strstr((char *)buf, "\r\n");
        if (!p) {
            MAINLOG_L1("Status line incomplete, continuing to receive...");
            Delay_Api(500);
            continue; // Not complete, continue receiving
        }

        // Terminate status line and move p to header start
        *p = 0;
        MAINLOG_L1("Status line: %s", buf);
        p += 2;
        length -= (p - (char *)buf);

        // Parse status code efficiently
        p1 = strchr((char *)buf, ' ');
        if (!p1) {
            MAINLOG_L1("Malformed status line!");
            net_close(netContext);
            free(buf);
            return RECEIVE_ERROR;
        }

        p1[4] = 0; // Limit to 3 digits
        ret = atoi(p1 + 1);
        MAINLOG_L1("HTTP status code: %d", ret);

        // Accept 200 (OK), 206 (Partial Content), 301 (Redirect)
        if ((ret != 200) && (ret != 206) && (ret != 301)) {
            MAINLOG_L1("Unexpected status code: %d", ret);
            net_close(netContext);
            free(buf);
            return -ret; // Return negative status code
        }

        break; // Status line processed successfully
    }

    // Process HTTP headers efficiently
    MAINLOG_L1("Processing headers...");
    while (1) {
        p1 = strstr(p, "\r\n");

        if (!p1) {
            MAINLOG_L1("Headers incomplete, receiving more data...");
            // Move remaining data to beginning of buffer
            memmove(buf, p, length);
            p = (char *)buf;

            // Receive more header data
            ret = net_read(netContext, buf + length, OPTIMIZED_BUF_SIZE - 1 - length,
                          RECEIVE_TIMEOUT * 1000);
            if (ret <= 0) {
                MAINLOG_L1("Failed to receive complete headers, error: %d", ret);
                net_close(netContext);
                free(buf);
                return RECEIVE_ERROR;
            }

            length += ret;
            buf[length] = '\0';
            continue;
        }

        // End of headers
        if (p1 == p) {
            MAINLOG_L1("End of headers reached");
            p += 2;
            length -= 2;
            break;
        }

        // Process current header
        *p1 = 0;
        p2 = p;         // Current header
        p = p1 + 2;     // Next header
        length -= (p - p2);

        // Find header name/value separator
        p1 = strchr(p2, ':');
        if (!p1) {
            MAINLOG_L1("Malformed header: %s", p2);
            net_close(netContext);
            free(buf);
            return RECEIVE_ERROR;
        }

        *p1 = 0;
        char* key = trim(p2);
        char* value = trim(p1 + 1);

        MAINLOG_L1("Header: %s: %s", key, value);

        // Handle important headers
        if (strcasecmp(key, "Location") == 0) {
            // Handle redirect
            MAINLOG_L1("Redirecting to: %s", value);
            URL_ENTRY oldEntry = entry;
            ret = parseURL(value, &entry);

            // If only the path changed, preserve the existing domain
            if (strlen(entry.domain) == 0) {
                strcpy(entry.domain, oldEntry.domain);
                strcpy(entry.port, oldEntry.port);
            }

            net_close(netContext);
            netContext = NULL;
            goto relocate_301;
        }
        else if (strcasecmp(key, "Content-Length") == 0) {
            payload_flag = 1;
            total = atoi(value);
            MAINLOG_L1("Content-Length: %d bytes", total);
        }
        else if (strcasecmp(key, "Transfer-Encoding") == 0 &&
                strcasecmp(value, "chunked") == 0) {
            payload_flag = 2;
            MAINLOG_L1("Transfer-Encoding: chunked");
        }
    }


    // Handle payload based on transfer mode
    if (payload_flag == 1) {
        // Known content length
        MAINLOG_L1("Starting download with known content length: %d bytes", total);

        // Save any already-received payload data
        if (length > 0) {
            MAINLOG_L1("Saving %d bytes of initial payload data", length);
            WriteFile_Api(filename, (unsigned char *)p, start, length);
        }

        offset = length;
        int retry_count = 0;
        const int MAX_RETRIES = 3;

        // Allocate a file buffer for downloads
        file_buffer = malloc(OPTIMIZED_BUF_SIZE);
        if (!file_buffer) {
            MAINLOG_L1("Failed to allocate file buffer");
            net_close(netContext);
            free(buf);
            return RECEIVE_ERROR;
        }

        // Optimized download loop with fixed-size chunks
        while (offset < total) {
            int chunk_size = total - offset > OPTIMIZED_BUF_SIZE ?
                             OPTIMIZED_BUF_SIZE : total - offset;

            MAINLOG_L1("Downloading chunk (%d bytes), progress: %d/%d (%.1f%%)",
                       chunk_size, offset, total, (float)offset*100/total);

            length = net_read(netContext, file_buffer, chunk_size, RECEIVE_TIMEOUT * 1000);

            // Handle connection closed by server
            if (length == 0 && retry_count < MAX_RETRIES) {
                MAINLOG_L1("Server closed connection, retry %d/%d",
                          retry_count+1, MAX_RETRIES);
                retry_count++;
                Delay_Api(300 * retry_count); // Progressive backoff

                // Try to reestablish connection if needed
                if (_IS_WIFI_ENABLED_) {
                    net_close(netContext);
                    netContext = NULL;

                    // Update the entry path to include a range request for the remaining data
                    char rangePath[256];
                    snprintf(rangePath, sizeof(rangePath), "%s", entry.path);

                    // Store the current offset for resuming the download
                    start += offset;

                    // Use the existing relocate_301 flow to handle reconnection
                    MAINLOG_L1("Reconnecting to resume download at byte %d", start);
                    goto relocate_301;
                }
            }

            // Still handle truly negative return codes as errors
            if (length < 0) {
                MAINLOG_L1("Failed to receive data chunk, error: %d", length);
                net_close(netContext);
                free(buf);
                free(file_buffer);
                return RECEIVE_ERROR;
            }

            // Write data to file if we received something
            if (length > 0) {
                retry_count = 0; // Reset retry counter on successful read
                WriteFile_Api(filename, file_buffer, start + offset, length);
                offset += length;
            }
        }

        MAINLOG_L1("Download complete: %d bytes received", total);
        ret = total; // Return total bytes downloaded
    }
    else if (payload_flag == 2)
    {
        // Chunked transfer encoding
        MAINLOG_L1("Starting chunked download");
        offset = 0;
        chunked_total = 0;

        // Allocate file buffer for chunked downloads
        file_buffer = malloc(OPTIMIZED_BUF_SIZE);
        if (!file_buffer) {
            MAINLOG_L1("Failed to allocate file buffer for chunked transfer");
            net_close(netContext);
            free(buf);
            return RECEIVE_ERROR;
        }

        // Process data already in buffer
        if (length > 0) {
            memcpy(file_buffer, p, length);
        }

        // Flag to track if we need to read more data from socket
        int need_more_data = (length == 0);
        int chunk_buffer_used = length;
        char *current_pos = (char *)file_buffer;

        while (1) {
            // If we need more data from the socket
            if (need_more_data) {
                // Move any remaining data to the beginning of the buffer
                if (chunk_buffer_used > 0 && current_pos > (char*)file_buffer) {
                    int remaining = chunk_buffer_used - (current_pos - (char*)file_buffer);
                    if (remaining > 0) {
                        memmove(file_buffer, current_pos, remaining);
                        current_pos = (char*)file_buffer;
                        chunk_buffer_used = remaining;
                    } else {
                        current_pos = (char*)file_buffer;
                        chunk_buffer_used = 0;
                    }
                } else {
                    current_pos = (char*)file_buffer;
                    chunk_buffer_used = 0;
                }

                // Read more data
                ret = net_read(netContext, file_buffer + chunk_buffer_used,
                               OPTIMIZED_BUF_SIZE - chunk_buffer_used - 1,
                               RECEIVE_TIMEOUT * 1000);
                if (ret <= 0) {
                    if (ret < 0) {
                        MAINLOG_L1("Failed to receive chunk data, error: %d", ret);
                        net_close(netContext);
                        free(buf);
                        free(file_buffer);
                        return RECEIVE_ERROR;
                    }
                    // A return of 0 likely means end of data in chunked transfer
                    break;
                }

                chunk_buffer_used += ret;
                file_buffer[chunk_buffer_used] = '\0'; // Null terminate for string processing
                need_more_data = 0;
            }

            // Parse chunk size
            char *end_ptr;
            int chunk_size = strtol(current_pos, &end_ptr, 16);

            // If we couldn't parse the chunk size, we might need more data
            if (end_ptr == current_pos) {
                need_more_data = 1;
                continue;
            }

            // Check for last chunk (size 0)
            if (chunk_size == 0) {
                MAINLOG_L1("Final chunk received (size 0)");
                break;
            }

            // Look for the CRLF after chunk size
            char *crlf = strstr(current_pos, "\r\n");
            if (!crlf) {
                need_more_data = 1;
                continue;
            }

            // Move to the data portion
            current_pos = crlf + 2;

            // Check if we have the complete chunk in the buffer
            int chunk_header_size = (current_pos - (char*)file_buffer);
            int available_data = chunk_buffer_used - chunk_header_size;

            if (available_data < chunk_size + 2) { // +2 for trailing CRLF
                need_more_data = 1;
                continue;
            }

            // We have a complete chunk, write it to the file
            WriteFile_Api(filename, (unsigned char*)current_pos, start + offset, chunk_size);
            offset += chunk_size;
            chunked_total += chunk_size;

            // Move past this chunk's data and the trailing CRLF
            current_pos += chunk_size + 2;

            // Check if we need more data for the next chunk
            if (current_pos >= (char*)file_buffer + chunk_buffer_used) {
                need_more_data = 1;
            }
        }

        MAINLOG_L1("Chunked download complete: %d bytes total", chunked_total);
        ret = chunked_total; // Return total bytes downloaded
    }
    else {
        // No valid transfer mode detected
        MAINLOG_L1("No valid transfer mode detected in response headers");
        net_close(netContext);
        free(buf);
        if (file_buffer) free(file_buffer);
        return RECEIVE_ERROR;
    }

    // Clean disconnection
    MAINLOG_L1("Disconnecting from server");
    net_close(netContext);

    // Free allocated buffers
    free(buf);
    if (file_buffer) free(file_buffer);

    MAINLOG_L1("Download successful: %d bytes saved to %s", ret, filename);
    return ret;
}

int httpDownload(char *url, int method, char *filename)
{
	return __httpDownload(url, method, filename, 0, -1);
}

