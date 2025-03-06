/*
 * sound.c
 *
 *  Created on: 2021Äê10ÔÂ15ÈÕ
 *      Author: vanstone
 */
#include "def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

void AppPlayTip(char *tip) {
//	PlaySound_Api((unsigned char*) tip, G_sys_param.sound_level, 0);
	if (G_sys_param.sound_level == 5) {
		G_sys_param.sound_level = 1;
		ttsSetVolume_lib(G_sys_param.sound_level);
		PlaySound_Api((unsigned char*) tip, G_sys_param.sound_level, 0);
	} else {
		G_sys_param.sound_level = 1;
		ttsSetVolume_lib(G_sys_param.sound_level);
		PlaySound_Api((unsigned char*) tip, G_sys_param.sound_level, 0);
	}
	initParam();
}

void PlayMP3File(char *audioFileName) {
	int ret;
	char filePath[128];

	memset(filePath, 0, sizeof(filePath));

//	sprintf(filePath,"/app/ufs/%s",audioFileName);
	sprintf(filePath, "/ext/tms/%s", audioFileName);

	// MAINLOG_L1("MP3 Path is %s", filePath);

	while (1) {

		ret = audioFilePlayPath_lib(filePath);

		if (ret == 0) {
			break;
		} else if (ret == -1) {
			// MAINLOG_L1("Play failed, please check if the file is damaged");
			break;
		} else if (ret == -2) {
			// MAINLOG_L1("File is not present");
			break;
		} else if (ret == -3) {
			// MAINLOG_L1("TSS is occupied");
		}
	}
}

void stopAllAudios()
{
	ttsQueueClear_Api();
	audioStop_lib();
}
