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

	char fileName[] = "param_api.dat";
	char baseURL[] = "http://103.25.92.104/tms/Vanstone/config/";
	char savePath[] = "/ext/tms/";

	char saveLocation[256];
	char urlDownload[256];
	int r;
	// MAINLOG_L1("**** Checking for existing param.dat");

	fileFilter=4;
	deleteAll();

	uint8 *rP = NULL;
	int iRet = fileGetFileListCB_lib(TMS_FILE_DIR,
			filegetlistcbtestings_config_file, rP);

	if (exist_or_not_file == 1) {
		// MAINLOG_L1("**** param.dat exists, skipping download");
		// MAINLOG_L1("PARAM _TMS_HOST_IP_ %s", _TMS_HOST_IP_);
		// MAINLOG_L1("PARAM _TMS_HOST_DOMAIN_ %s", _TMS_HOST_DOMAIN_);
	} else {
		snprintf(saveLocation, sizeof(saveLocation), "%s%s", savePath,
				fileName);
		snprintf(urlDownload, sizeof(urlDownload), "%s%s", baseURL, fileName);

		r = httpDownload(urlDownload, METHOD_GET, saveLocation);

		if (r < 0) {
			// MAINLOG_L1("**** Failed to download param.dat, retrying...");
			r = httpDownload(urlDownload, METHOD_GET, saveLocation);
		}

		if (r >= 0) {
			// MAINLOG_L1("**** Successfully downloaded: param.dat");
		} else {
			// MAINLOG_L1("**** Failed to download param.dat after retry");
			strcpy(_TMS_HOST_IP_, "103.235.231.19");
			strcpy(_TMS_HOST_DOMAIN_, "ipos-os.jiewen.com.cn");
			strcpy(_TMS_HOST_PORT_, "80");
			//#define _TMS_HOST_IP_		"103.235.231.19"
			//#define _TMS_HOST_DOMAIN_	"ipos-os.jiewen.com.cn"
			//#define _TMS_HOST_PORT_		"80"
		}

		exist_or_not_file = 0;
		readWIFIParam();
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

	// just pointers to const char
	const char *baseURL = NULL;
	const char *fileName = NULL;
	// Your device string
	const char *device = G_sys_param.sn;
//	char device[128];                  // Make sure the array is large enough
//	strcpy(device, G_sys_param.sn);    // Copies the runtime string into device1

//	    // Substring to search for
	char substring1[] = "600";
	char substring2[] = "6000";
	char substring3[] = "60000";

	if (is_valid_match(device, substring1)) {
		// MAINLOG_L1("Yes string existed substring1");
		baseURL = "http://103.25.92.104/tms/Vanstone/600/";
		fileName = "600.dat";
	}else if (is_valid_match(device, substring2)) {
		// MAINLOG_L1("Yes string existed substring2");
		baseURL = "http://103.25.92.104/tms/Vanstone/6000/";
		fileName = "6000.dat";
	}else if (is_valid_match(device, substring3)) {
		// MAINLOG_L1("Yes string existed substring3");
		baseURL = "http://103.25.92.104/tms/Vanstone/60000/";
		fileName = "60000.dat";
	}


	char savePath[] = "/ext/tms/";

	char saveLocation[256];
	char urlDownload[256];
	int r;


	// MAINLOG_L1("**** Checking for existing dB.dat");

	uint8 *rP = NULL;
	int iRet = fileGetFileListCB_lib(TMS_FILE_DIR,
			filegetlistcbtestings_config_dB, rP);

	if (dB_file == 1) {
		// MAINLOG_L1("LLLLLLLLLLL **** dB.dat exists, skipping download");
	} else {
		snprintf(saveLocation, sizeof(saveLocation), "%s%s", savePath,
				fileName);
		snprintf(urlDownload, sizeof(urlDownload), "%s%s", baseURL, fileName);
		// MAINLOG_L1("URL **** Failed to download dB.dat, retrying... %s",urlDownload);

		r = httpDownload(urlDownload, METHOD_GET, saveLocation);

		if (r < 0) {
			// MAINLOG_L1("LLLLLLLLLLL **** Failed to download dB.dat, retrying...");
			r = httpDownload(urlDownload, METHOD_GET, saveLocation);
		}

		if (r >= 0) {
			// MAINLOG_L1("LLLLLLLLLLL **** Successfully downloaded: dB.dat");
		} else {
			// MAINLOG_L1("LLLLLLLLLLL **** Failed to download dB.dat after retry");
		}

		int ret, len = 64;
		char buf[64] = "";

		unsigned char *ptr = NULL;

		char name[128];

		snprintf(name, sizeof(name), "%/ext/tms/%s",fileName);
		ret = ReadFile_Api(name, buf, 0, &len);
		if (ret != 0) {
			// MAINLOG_L1("LLLLLLLLLLL Read PARAM Failed(%d) !!!",ret);
			return;
		}

		ptr = strstr(buf, ",");
		if (ptr == NULL) {
			// MAINLOG_L1("LLLLLLLLLLL PARAM ERROR !!!");

			return;
		}

		memcpy(tms_ip, buf,     strlen(buf) - strlen(ptr));
		memcpy(global_dB, ptr + 1, strlen(ptr + 1));




		dB_file = 0;

//		strcpy(_TMS_HOST_IP_,tms_ip);
//		strcpy(_TMS_HOST_DOMAIN_, tms_domain);

//		// MAINLOG_L1("LLLLLLLLLLL PARAM SN %s",_TMS_HOST_IP_);
//		// MAINLOG_L1("LLLLLLLLLLL PARAM DB %s",_TMS_HOST_DOMAIN_);

//		char global_dB[124];
//		strcpy(global_dB, tms_domain);

		// MAINLOG_L1("LLLLLLLLLLL PARAM Global_dB %s",global_dB);



	}

}
