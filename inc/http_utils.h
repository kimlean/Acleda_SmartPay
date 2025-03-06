#ifndef __HTTP_UTILS_H__
#define	__HTTP_UTILS_H__

// ========================== DEFINE ==========================
#define MAX_DOMAIN_LEN 128

#define	TIMEOUT	30 * 1000

#define DATA_BUF_SIZE 	 2048
#define	RECEIVE_BUF_SIZE 2048

#define REQUEST_GET  0
#define REQUEST_POST 1

#define HTTP_PROTOCOL  0
#define HTTPS_PROTOCOL 1
#define HTTPS_CRT_PROTOCOL 2
// ========================== DEFINE ==========================

// ========================== STRUCT ==========================
typedef struct {
	char type[6];
	char version[12];
	char request_suffix[MAX_DOMAIN_LEN];
	int protocol;
	char domain[MAX_DOMAIN_LEN];
	char port[4];
} HTTP_UTILS_CONNECT_PARAMS;

typedef struct {
	char accept[64];
	char accept_encoding[42];
	char content_type[64];
	char user_agent[52];
	char connection[16];
} HTTP_UTILS_REQUEST_HEADER;

typedef struct {
	char *version;
	char *code;
	char *desc;
	char *body;
	int bodySize;
} HTTP_UTILS_RESPONSE;
// ========================== STRUCT ==========================

// ========================== FUNCTIONS ==========================
int http_utils_connect(
	HTTP_UTILS_CONNECT_PARAMS *http_utils_connect_params,
	HTTP_UTILS_REQUEST_HEADER *http_utils_request_header,
	char *data,
	char *result,
	int isQueryType,
	char *queries
);

int http_utils_parse_header(char *data, char *result, int *is_body_size_ok);
// ========================== FUNCTIONS ==========================

#endif
