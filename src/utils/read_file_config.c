#include "EnvAcleda.h"  // Add this include at the top
#include <string.h>
#include "def.h"
#include <coredef.h>
#include <struct.h>
#include <poslib.h>
#include "httpDownload.h"
#include <stdio.h>

#define SERVER_IP "103.83.163.69"
#define SERVER_PORT "3000"
#define ACCESS_TOKEN "IQ5IwuSo15TavbCNd6473IL6kaQ89i72NAFuMulzXfs" //your_api_key
#define TIMEOUT_MS 1000

char _TMS_HOST_IP_[64];
char _TMS_HOST_DOMAIN_[128];
char _TMS_HOST_PORT_[64];
char global_dB[10];
//int global_dB[64];

#define TMS_FILE_DIR  "/ext/tms/"
int exist_or_not_file = 0;
int dB_file = 0;

void filegetlistcbtestings_config_file(const char *pchDirName, uint32 size,
		uint8 filetype, void *arg) {

	char *pChar;

	// MAINLOG_L1( "**** [%s] -%s- Line=%d: size = %d,filetype=%d,pchDirName:%s, arg=%s\r\n", filename(__FILE__), __FUNCTION__, __LINE__, size, filetype, pchDirName, arg);

	pChar = strstr(pchDirName, "param_api.dat");
	// MAINLOG_L1("PARAM @@@@@@@@@@@@@%s**%s", pchDirName, pChar);
	if (pChar == ".dat") {
		// MAINLOG_L1("**** param_api.dat already exists");
//		strcpy(_TMS_HOST_IP_, "103.235.231.19");
//		strcpy(_TMS_HOST_DOMAIN_, "ipos-os.jiewen.com.cn");
		exist_or_not_file = 1;
	} else {
		// MAINLOG_L1("**** param_api.dat not found");
		exist_or_not_file = 0;
	}
}

void filegetlistcbtestings_config_dB(const char *pchDirName, uint32 size,
		uint8 filetype, void *arg) {

	char *pChar;

	// MAINLOG_L1( "**** [%s] -%s- Line=%d: size = %d,filetype=%d,pchDirName:%s, arg=%s\r\n", filename(__FILE__), __FUNCTION__, __LINE__, size, filetype, pchDirName, arg);

	pChar = strstr(pchDirName, filename(__FILE__));
	// MAINLOG_L1("PARAM @@@@@@@@@@@@@%s**%s", pchDirName, pChar);
	if (pChar == filename(__FILE__)) {
		// MAINLOG_L1("**** %s already exists",filename(__FILE__));
//		strcpy(_TMS_HOST_IP_, "103.235.231.19");
//		strcpy(_TMS_HOST_DOMAIN_, "ipos-os.jiewen.com.cn");
		dB_file = 1;
	} else {
		// MAINLOG_L1("**** %s not found",filename(__FILE__));
		dB_file = 0;
	}
}

void download_config_file() {
    char saveLocation[256];
    char urlDownload[256];
    int r;

    // Delete existing config files
    fileFilter = 4;
    deleteAll();

    // Check if config file exists
    uint8 *rP = NULL;
    int iRet = fileGetFileListCB_lib(TMS_FILE_DIR,
            filegetlistcbtestings_config_file, rP);

    if (exist_or_not_file == 1) {
        // Config file exists, skipping download
        MAINLOG_L1("**** param.dat exists, skipping download");

        // Even if file exists, we need to read its contents!
        readTMSParam();

        MAINLOG_L1("Config loaded - IP: %s, Domain: %s, Port: %s",
                  _TMS_HOST_IP_, _TMS_HOST_DOMAIN_, _TMS_HOST_PORT_);
    } else {
        // Build paths for download
        snprintf(saveLocation, sizeof(saveLocation), "%s%s", CONFIG_SAVE_PATH,
                CONFIG_FILE_NAME);
        snprintf(urlDownload, sizeof(urlDownload), "%s%s", CONFIG_BASE_URL,
                CONFIG_FILE_NAME);

        // Download the configuration file
        r = httpDownload(urlDownload, METHOD_GET, saveLocation);

        // Retry if download failed
        if (r < 0) {
             MAINLOG_L1("**** tms Failed to download param.dat, retrying...");
            r = httpDownload(urlDownload, METHOD_GET, saveLocation);
        }

        if (r >= 0) {
             MAINLOG_L1("**** tms Successfully downloaded: param.dat");

            // Now read the newly downloaded file
            readTMSParam();

            MAINLOG_L1("Config loaded - IP: %s, Domain: %s, Port: %s",
                      _TMS_HOST_IP_, _TMS_HOST_DOMAIN_, _TMS_HOST_PORT_);
        }
        else if(r<0) {
            // Download failed, use default values
            MAINLOG_L1("**** tms Failed to download param.dat after retry");
            strcpy(_TMS_HOST_IP_, DEFAULT_TMS_HOST_IP);
            strcpy(_TMS_HOST_DOMAIN_, DEFAULT_TMS_HOST_DOMAIN);
            strcpy(_TMS_HOST_PORT_, DEFAULT_TMS_HOST_PORT);


            MAINLOG_L1("Using default settings - IP: %s, Domain: %s, Port: %s",
                       _TMS_HOST_IP_, _TMS_HOST_DOMAIN_, _TMS_HOST_PORT_);
        }

        exist_or_not_file = 0;
    }
}

int is_valid_match(const char *str, const char *match) {
	// Find the first occurrence of `match` in `str`.
	const char *pos = strstr(str, match);
	if (!pos) {
		// If 'match' isn't found at all, return 0.
		return 0;
	}

	// Calculate length of the match for indexing the next character.
	size_t match_len = strlen(match);
	// The next character after the found substring:
	char after = *(pos + match_len);

	// If there is no character after the substring (i.e., '\0'), that's okay.
	// If the next character exists and is '0', we consider it a "bigger" substring like "6000" → no match.
	// Otherwise, we accept it.
	if (after == '\0') {
		return 1;   // End of string → valid match
	} else if (after == '0') {
		return 0;   // Next char is '0' → consider it "6000" → no match
	}
	return 1;       // Next char is non-zero → valid match
}

void download_config_dB() {
    // Device string for matching
    const char *device = G_sys_param.sn;
    const char *baseURL = NULL;
    const char *fileName = NULL;

    // Comparison strings
    char substring1[] = "600";
    char substring2[] = "6000";
    char substring3[] = "60000";

    // Match device SN and select appropriate URL
    if (is_valid_match(device, substring1)) {
        baseURL = DB_BASE_URL_600;
        fileName = DB_FILENAME_600;
    } else if (is_valid_match(device, substring2)) {
        baseURL = DB_BASE_URL_6000;
        fileName = DB_FILENAME_6000;
    } else if (is_valid_match(device, substring3)) {
        baseURL = DB_BASE_URL_60000;
        fileName = DB_FILENAME_60000;
    }

    // Check for existing file
    char saveLocation[256];
    char urlDownload[256];
    int r;

    uint8 *rP = NULL;
    int iRet = fileGetFileListCB_lib(TMS_FILE_DIR,
            filegetlistcbtestings_config_dB, rP);

    if (dB_file == 1) {
        // DB file exists, skipping download
    } else {
        // Build download paths
        snprintf(saveLocation, sizeof(saveLocation), "%s%s", DB_SAVE_PATH, fileName);
        snprintf(urlDownload, sizeof(urlDownload), "%s%s", baseURL, fileName);

        // Download the configuration file
        r = httpDownload(urlDownload, METHOD_GET, saveLocation);

        // Retry if download failed
        if (r < 0) {
            r = httpDownload(urlDownload, METHOD_GET, saveLocation);
        }

        // Process the downloaded file
        if (r >= 0) {
            int ret, len = 64;
            char buf[64] = {0};
            unsigned char *ptr = NULL;
            char name[128];

            snprintf(name, sizeof(name), "/ext/tms/%s", fileName);
            ret = ReadFile_Api(name, buf, 0, &len);
            if (ret != 0) {
                return;
            }

            ptr = strstr(buf, ",");
            if (ptr == NULL) {
                return;
            }

            memcpy(tms_ip, buf, strlen(buf) - strlen(ptr));
            memcpy(global_dB, ptr + 1, strlen(ptr + 1));

            dB_file = 0;
        }
    }
}
