#include <stdio.h>
#include <string.h>

#include "def.h"
#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#define ZIPPATH "/ext/"

u8* filename(u8 *file) {
	return file;
}

void filegetlistcbtesting(const char *pchDirName, uint32 size, uint8 filetype,
		void *arg) {
	char *pChar;
	int ret, num;

//	MAINLOG_L1("@@@ [%s] -%s- Line=%d: size = %d,filetype=%d,pchDirName:%s, arg=%s\r\n", filename(__FILE__), __FUNCTION__, __LINE__,size, filetype, pchDirName, arg);

	if (fileFilter == 0) {
		pChar = strstr(pchDirName, ".mp3"); // Delete via Serial number  //G_sys_param.sn
		if (pChar != NULL) {
			pChar = strstr(pchDirName, G_sys_param.sn);
			if (pChar != NULL) {
				DelFile_Api(pchDirName);
			}
		}
	}

	//delete all mp3 files
	if (fileFilter == 1) {
		pChar = strstr(pchDirName, ".mp3");
		if (pChar != NULL) {
			DelFile_Api(pchDirName);
		}
	}

	//delete all .img files
	if (fileFilter == 2) {
		pChar = strstr(pchDirName, ".jpg");
		if (pChar != NULL) {
			DelFile_Api(pchDirName);
		}
	}

	//search app to update
	if (fileFilter == 3) {
		pChar = strstr(pchDirName, ".img");
		if (pChar != NULL) {
			num = 9;
			memset(updateAppName, 0, sizeof(updateAppName));
			memcpy(updateAppName, pchDirName, strlen(pchDirName));
			// MAINLOG_L1("pchDirName = %s", pchDirName);
			needUpdate = 1;
		}
	}

	//delete all mp3 files
	if (fileFilter == 4) {
		pChar = strstr(pchDirName, "param_api.dat");
		if (pChar != NULL) {
			// MAINLOG_L1("PARAM = %s", pchDirName);
			DelFile_Api(pchDirName);
		}
	}
	if (fileFilter == 5) {
		DelFile_Api("/ext/app/data/sys_param.dat");
	}

}

void folderFileDisplay(unsigned char *filePath) {
	int iRet = -1;

	uint8 *rP = NULL;

	iRet = fileGetFileListCB_lib(filePath, filegetlistcbtesting, rP);

}

int unzipDownFile(unsigned char *fileName) {
	int ret;

	ret = fileunZip_lib(fileName, ZIPPATH);
	if (ret != 0) {
		return -1;
	}
	DelFile_Api(fileName); //delete zip after unzip it

	return 0;
}

void CheckAppFile()
{
	int ret;
	unsigned char buf[32];

	ret = GetFileSize_Api(SAVE_UPDATE_FILE_NAME);
	// MAINLOG_L1("GetFileSize_Api = %d", ret);
	if (ret > 0)
	{
		memset(buf, 0, sizeof(buf));
		ReadFile_Api(SAVE_UPDATE_FILE_NAME, buf, 0, (unsigned int*) &ret);
		// MAINLOG_L1("buf = %s", buf);
		DelFile_Api(buf);
		DelFile_Api(SAVE_UPDATE_FILE_NAME);
	}
}

int saveInternetConnectionType(int connectionType) {
    char buf[2] = {0};

    // Convert connection type to character
    buf[0] = '0' + connectionType;

    // Save to file
    int ret = SaveWholeFile_Api(INTERNET_CONFIG_FILE, buf, 1);

    MAINLOG_L1("Saved internet connection type %d, result: %d", connectionType, ret);

    return ret;
}

int getInternetConnectionType() {
    unsigned int size = 2;
    char buf[2] = {0};
    int connectionType = 0;

    // Check if the file exists
    int fileSize = GetFileSize_Api(INTERNET_CONFIG_FILE);
    if (fileSize <= 0) {
        MAINLOG_L1("Internet configuration file not found");
        return 0;
    }

    // Read the file
    int ret = ReadFile_Api(INTERNET_CONFIG_FILE, buf, 0, &size);
    if (ret == 0 && size > 0) {
        connectionType = buf[0] - '0';
        MAINLOG_L1("Read internet connection type: %d", connectionType);

        // Validate the connection type
        if (connectionType != 1 && connectionType != 2) {
            MAINLOG_L1("Invalid connection type value: %d", connectionType);
            connectionType = 0;
        }
    } else {
        MAINLOG_L1("Failed to read internet configuration file, ret: %d, size: %d", ret, size);
    }

    return connectionType;
}

int saveThanksModeType(int mode) {
    char buf[2] = {0};

    // Ensure mode is either 0 or 1
    mode = (mode != 0) ? 1 : 0;

    // Convert mode to character
    buf[0] = '0' + mode;

    // Save to file
    int ret = SaveWholeFile_Api(THANKS_MODE_CONFIG_FILE, buf, 1);

    MAINLOG_L1("Saved Thanks Mode type %d, result: %d", mode, ret);

    return ret;
}

int getThanksModeType() {
    unsigned int size = 2;
    char buf[2] = {0};
    int mode = 0; // Default to OFF

    // Check if the file exists
    int fileSize = GetFileSize_Api(THANKS_MODE_CONFIG_FILE);
    if (fileSize <= 0) {
        MAINLOG_L1("Thanks Mode configuration file not found, using default: %d", mode);
        return mode;
    }

    // Read the file
    int ret = ReadFile_Api(THANKS_MODE_CONFIG_FILE, buf, 0, &size);
    if (ret == 0 && size > 0) {
        mode = buf[0] - '0';
        MAINLOG_L1("Read Thanks Mode type: %d", mode);

        // Validate the mode
        if (mode != 0 && mode != 1) {
            MAINLOG_L1("Invalid Thanks Mode value: %d, using default: 0", mode);
            mode = 0;
        }
    } else {
        MAINLOG_L1("Failed to read Thanks Mode configuration file, ret: %d, size: %d", ret, size);
    }

    return mode;
}
