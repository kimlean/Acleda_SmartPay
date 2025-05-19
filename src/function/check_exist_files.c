#include <string.h>
#include "def.h"
#include <coredef.h>
#include <struct.h>
#include <poslib.h>
#include "httpDownload.h"
#include <stdio.h>

#define SERVER_IP "103.25.92.104"
// #define SERVER_IP "103.83.163.69"
#define SERVER_PORT "3000"
#define ACCESS_TOKEN "IQ5IwuSo15TavbCNd6473IL6kaQ89i72NAFuMulzXfs" //your_api_key
#define TIMEOUT_MS 1000

#define TMS_FILE_DIR  "/ext/tms/"
int exist_or_not = 0;
int mp3_exist_or_not = 0;
void filegetlistcbtestings(const char *pchDirName, uint32 size, uint8 filetype,
		void *arg) {

	char *pChar;
	int ret, num;

	// MAINLOG_L1( "**** [%s] -%s- Line=%d: size = %d,filetype=%d,pchDirName:%s, arg=%s\r\n", filename(__FILE__), __FUNCTION__, __LINE__, size, filetype,pchDirName, arg);

	if (fileFilter == 0) {
		pChar = strstr(pchDirName, ".jpg"); // Delete via Serial number  //G_sys_param.sn
		if (pChar != NULL) {
			pChar = strstr(pchDirName, G_sys_param.sn);
			if (pChar != NULL) {
				// MAINLOG_L1("**** Existing Images");
//				DelFile_Api(pchDirName);
				exist_or_not = 1;
			} else {
				// MAINLOG_L1("**** No Existing Images");
				exist_or_not = 0;
			}

		}
	} else if (fileFilter == 9) {
		pChar = strstr(pchDirName, ".mp3"); // Delete via Serial number  //G_sys_param.sn
		if (pChar != NULL) {
//			pChar = strstr(pchDirName, G_sys_param.sn);
			if (pChar != NULL) {
				// MAINLOG_L1("**** Existing MP3 QQQQQQQQQQ");
				//				DelFile_Api(pchDirName);
				mp3_exist_or_not = 1;
			} else {
				// MAINLOG_L1("**** No Existing MP3 rrrrrrrrrr");
				mp3_exist_or_not = 0;
			}

		}
	}
}
//*********************************************************
void check_exist_files() {
    int iRet = -1;
//
    uint8 *rP = NULL;
//
    iRet = fileGetFileListCB_lib(TMS_FILE_DIR, filegetlistcbtestings, rP);

    if (exist_or_not == 1) {
        MAINLOG_L1("**** Yes Images");
    }else{

    	// KIMLEAN CLOSING CODE NOT TO DOWNLOAD FIRST
//    	download_qr_image_main();
    }
}

//*******************************************



void filegetlistcbtestings_mp3(const char *pchDirName, uint32 size,
		uint8 filetype, void *arg) {

	char *pChar;
	int ret, num;

	// MAINLOG_L1( "**** [%s] -%s- Line=%d: size = %d,filetype=%d,pchDirName:%s, arg=%s\r\n", filename(__FILE__), __FUNCTION__, __LINE__, size, filetype, pchDirName, arg);

	if (fileFilter == 0) {
		pChar = strstr(pchDirName, ".mp3"); // Delete via Serial number  //G_sys_param.sn
		if (pChar != NULL) {
			pChar = strstr(pchDirName, G_sys_param.sn);
			if (pChar != NULL) {
				// MAINLOG_L1("**** Existing MP3");
//				DelFile_Api(pchDirName);
				mp3_exist_or_not = 1;
			} else {
				// MAINLOG_L1("**** No Existing MP3");
				mp3_exist_or_not = 0;
			}

		}
	}
}

/*
 * download_mp3() - Fast and robust implementation for downloading MP3 files
 * Incorporates robust techniques from download_QR.c with optimizations for speed
 */
#define FILE_COUNT 7

void download_mp3() {

	char *fileNames[FILE_COUNT] = { "Decrease.mp3", "Increase.mp3",
			"Insert sim.mp3", "IoT_Sucess_connect.mp3", "Max_sound.mp3",
			"Min_sound.mp3", "Success_01.mp3" };

	char baseURL[] =
			"http://103.25.92.104/home/spp/upload/file/tms/Vanstone/MP3/";
	char savePath[] = "/ext/tms/";

	char saveLocationMP3[256];
	char urlDownloadMP3[256];
	int r;
	fileFilter = 9;
	// MAINLOG_L1("**** Checking for existing MP3");

	uint8 *rP = NULL;
	int iRet = fileGetFileListCB_lib(TMS_FILE_DIR, filegetlistcbtestings,
			rP);

	if (mp3_exist_or_not == 1) {
		// MAINLOG_L1("**** MP3 exist, skipping download");
	} else {
		for (int i = 0; i < FILE_COUNT; i++) {
			snprintf(saveLocationMP3, sizeof(saveLocationMP3), "%s%s", savePath,
					fileNames[i]);
			snprintf(urlDownloadMP3, sizeof(urlDownloadMP3), "%s%s", baseURL,
					fileNames[i]);

			r = httpDownload(urlDownloadMP3, METHOD_GET, saveLocationMP3);

			if (r < 0) {
				// MAINLOG_L1("**** Failed to download %s, retrying...", fileNames[i]);
				r = httpDownload(urlDownloadMP3, METHOD_GET, saveLocationMP3);
			}

			if (r >= 0) {
				// MAINLOG_L1("**** Successfully downloaded: %s", fileNames[i]);
			} else {
				// MAINLOG_L1("**** Failed to download after retry: %s", fileNames[i]);
			}
		}
		mp3_exist_or_not = 0;
	}

}

