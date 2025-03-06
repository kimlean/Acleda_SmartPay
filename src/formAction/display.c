/*
 * display.c
 *
 *  Created on: Oct 25, 2021
 *      Author: Administrator
 */

#include <string.h>

#include "def.h"
#include <coredef.h>
#include <struct.h>
#include <poslib.h>
#include "EmvCommon.h"

#define QRBMP "QRBMP.bmp"

// Update code here
#define KHQRBMP "/ext/tms/%s_KHR.jpg"
#define USDQRBMP "/ext/tms/%s_USD.jpg"

#define CONTENT1 "SMART PAY QR"
#define CONTENT2 "MID:qqqqqqqqqqqqq"
// Update code here
//SYS_PARAM G_sys_param;s
// Declare the global instance of the structure

void DispMainFace(void) {
	ScrCls_Api();
//    ScrDisp_Api(LINE4, 0, "Welcome to use", CDISP);
//    ScrDisp_Api(LINE5, 0, "Aisino Q161ffffffffffffffff", CDISP);
}

int WaitEvent(void) {
	u8 Key;
	int TimerId;

	TimerId = TimerSet_Api();
	while (1) {
		if (TimerCheck_Api(TimerId, 30 * 1000) == 1) {
			SysPowerStand_Api();
			return 0xfe;
		}
		Key = GetKey_Api();
		if (Key != 0)
			return Key;
	}

	return 0;
}

#define        TIMEOUT            -2

int ShowMenuItem(char *Title, const char *menu[], u8 ucLines, u8 ucStartKey,
		u8 ucEndKey, int IsShowX, u8 ucTimeOut) {
	u8 IsShowTitle, cur_screen, OneScreenLines, Cur_Line, i, t;
	int nkey;
	char dispbuf[50];

	memset(dispbuf, 0, sizeof(dispbuf));

	if (Title != NULL) {
		IsShowTitle = 1;
		OneScreenLines = 12;
	} else {
		IsShowTitle = 0;
		OneScreenLines = 13;
	}
	IsShowX -= 1;
	cur_screen = 0;
	while (1) {
		ScrClsRam_Api();
		if (IsShowTitle)
			ScrDisp_Api(LINE1, 0, Title, CDISP);
		Cur_Line = LINE1 + IsShowTitle;
		for (i = 0; i < OneScreenLines; i++) {
			t = i + cur_screen * OneScreenLines;
			if (t >= ucLines || menu[t] == NULL) //
			{
				break;
			}
			memset(dispbuf, 0, sizeof(dispbuf));
			strcpy(dispbuf, menu[t]);
			ScrDispRam_Api(Cur_Line++, 0, dispbuf, FDISP);
		}
		ScrBrush_Api();

		nkey = WaitAnyKey_Api(ucTimeOut);

		switch (nkey) {
		case ESC:
		case TIMEOUT:
			return nkey;
		default:
			if ((nkey >= ucStartKey) && (nkey <= ucEndKey))
				return nkey;
			break;
		}
	};
}

void MenuThread(void) {
	//TODO Fix sound

	int Result = 0;
	while (1) {
		LedLightOn_Api(0x01);
		ScrBackLight_Api(0xFFFF);
		CustomizedQR2(1);

		Result = WaitEvent();
		if (Result == 0xfe)
			continue;
		if (Result != 0) {
			switch (Result) {
			case ENTER:
				Beep_Api(0);
				ShowMainMenu();
				break;
			case FUNCTION: // FUNCTION = KEY PORT F1
				Beep_Api(0);
				QRType(2);
				break;
			case MENU: // MENU = KEY PORT FN
				Beep_Api(0);
				showFNmenu();
				break;
			default:
				break;
			}
		}
	}
}

int CustomizedQR2(int QRType) {
	int ret = 1;
	int key = 1;

	ScrCls_Api();

	// ON THIS WE CAN SET IT 1 TIME ON MAIN
	char USDQR_CONCAT[50]; // Ensure this buffer is large enough to hold the formatted string
	sprintf(USDQR_CONCAT, USDQRBMP, G_sys_param.sn);

	char KHQR_CONCAT[50]; // Ensure this buffer is large enough to hold the formatted string
	sprintf(KHQR_CONCAT, KHQRBMP, G_sys_param.sn);

	if (QRType == 1) {
		ret = ScrDispImage_Api(KHQR_CONCAT, 0, 0);
	} else {
		ret = ScrDispImage_Api(USDQR_CONCAT, 0, 0);
	}

	while (1) {
		key = WaitAnyKey_Api(60);
		switch (key) {
		case PGUP:
			G_sys_param.sound_level += 1;

			if (G_sys_param.sound_level >= 1 && G_sys_param.sound_level < 5) {
				ttsSetVolume_lib(G_sys_param.sound_level);
				PlayMP3File("Increase.mp3");
				// MAINLOG_L1( "hort_param MP3 Increase SSSSSSSSSSSSSSSSSSSSSSSSSSS is %d", G_sys_param.sound_level);
				saveParam();

			}
//			else if (G_sys_param.sound_level==3) {
//				ttsSetVolume_lib(G_sys_param.sound_level);
//				PlayMP3File("Max_sound.mp3");
//			}
			else {
				G_sys_param.sound_level = 5; // Make sure it equal 3 (it should not over 3,4,5,6,7,8,9 etc)
				ttsSetVolume_lib(G_sys_param.sound_level);
				PlayMP3File("Max_sound.mp3");
				saveParam();
			}
			continue;
		case PGDWON:

			G_sys_param.sound_level -= 1;
			// MAINLOG_L1("hort_param Decrease %d", G_sys_param.sound_level);

			if (G_sys_param.sound_level > 1 && G_sys_param.sound_level <= 4) {
				ttsSetVolume_lib(G_sys_param.sound_level);
				saveParam();
				PlayMP3File("Decrease.mp3");
			} else if (G_sys_param.sound_level == 1) {
				G_sys_param.sound_level = 1;
				ttsSetVolume_lib(G_sys_param.sound_level);
				saveParam();
				PlayMP3File("Min_sound.mp3");
			} else {
				G_sys_param.sound_level = 1; // Make sure it equal 1 (it should not 0,-1,-2 etc)
				ttsSetVolume_lib(G_sys_param.sound_level);
				PlayMP3File("Min_sound.mp3");
				saveParam();
			}
			continue;
		case ESC:
			return 0;
		default:
			break;
		}
	}

	return 0;
}

/* ========== KIMLEAN DEV ========== */
void QRType(int QRType) {
	int nSelcItem = 1, ret;

	// HEADER TEXT
	char *pszTitle = "";
	if (QRType == 1) {
		pszTitle = "Static Currency";
	};
	if (QRType == 2) {
		pszTitle = "Dynamic Currency";
	};

	// ITEM
	const char *pszItems[] = { "1. QR KHR", "2. QR USD" };
	while (1) {
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
				sizeof(pszItems) / sizeof(char*), DIGITAL1, DIGITAL2, 0, 60);
		switch (nSelcItem) {
		case DIGITAL1:
			if (QRType == 1) {
				CustomizedQR2(1);
			}
			if (QRType == 2) {
				DynamicQRCode(1);
				return;
			}
			break;
		case DIGITAL2:
			if (QRType == 1) {
				CustomizedQR2(2);
			}
			if (QRType == 2) {
				DynamicQRCode(2);
				return;
			}
			break;
		case ESC:
			return;
		default:
			break;
		}
	}
}

void WaitingForConfirmOrNext()
{
	unsigned char key;

	while (1) {
		key = GetKey_Api();

		if (key == 0x08 || key == ESC || key == ENTER) {
			if (key == ESC) { Beep_Api(0); }
			return;
		}
	}
}

// ========== Language Set

/// ========== Wifi Config
void ConfigWifi(int mode)
{
	int ret;

	ScrCls_Api();
	ScrClsRam_Api();

	ScrDispRam_Api(0, 0, "Wifi Config", CDISP);
	ScrBrush_Api();

	if (mode == 2) {
		ScrDispRam_Api(5, 0, "Run Hotspot Mode", CDISP);
	}
	else {
		ScrDispRam_Api(5, 0, "Run Bluetooth Mode", CDISP);
	}
	ScrBrush_Api();

	ret = wifiAPConnectType_lib(mode, 120);
	MAINLOG_L1("INFOMATION WIFI OPEN LIB %s", ret);
	ret = 0;
	if (ret != 0)
	{
		char info[32] = {0};
		sprintf(info, "%s Config Failed", mode == 2 ? "Hotspot Mode" : "Bluetooth Mode");

		char ret_buf[16] = {0};
		itoa(ret, ret_buf, 10);

		ScrDispRam_Api(6, 0, info, CDISP);
		ScrDispRam_Api(7, 0, ret_buf, CDISP);

		ScrBrush_Api();
	}
	else
	{
		char WifiName[256];  // Buffer to hold the combined stringh

		// Combine strings
		strcpy(WifiName, WIFI_HOTSPOT_CONFIG_SSID);  // Copy the first string to WifiName
		strcat(WifiName, G_sys_param.sn);

		ret = wifiWebNetwork_lib(WifiName, WIFI_HOTSPOT_CONFIG_PWD, 60 * 1000);
		if (ret != 0)
		{
			char info[32] = {0};
			sprintf(info, "%s Config", mode == 2 ? "Hotspot Mode" : "Bluetooth Mode");

			char info_[32] = {0};
			sprintf(info_, "Connect Failed(%d)", ret);

			ScrDispRam_Api(6, 0, info, CDISP);
			ScrDispRam_Api(7, 0, info_, CDISP);

			ScrBrush_Api();
		}
		else
		{
			int is_success = 0;

			while (1)
			{
				ret = wifiAPConnectCheck_lib();
				if (ret == 4) {
					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "Waiting for config...", CDISP);
					ScrBrush_Api();

					Delay_Api(3000);
					continue;
				}
				else if (ret == 0) {
					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "Preparing for config...", CDISP);
					ScrBrush_Api();

					Delay_Api(3000);
					continue;
				}
				else if (ret == 1) {
					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "Configuring...", CDISP);
					ScrBrush_Api();

					Delay_Api(3000);
					continue;
				}
				else if (ret == 3) {
					AppPlayTip("Configure time out");

					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "Configuring Timeout", CDISP);
					ScrBrush_Api();

					break;
				}
				else if (ret == 2) {
					AppPlayTip("WiFi configured");

					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "WiFi Configured", CDISP);
					ScrDispRam_Api(7, 0, "Device Reboot...", CDISP);
					ScrBrush_Api();

					is_success = 1;

					break;
				}
				else {
					AppPlayTip("Config failed");

					char ret_buf[16] = {0};
					itoa(ret, ret_buf, 10);

					char info[32] = {0};
					sprintf(info, "Config Failed(%s)", ret_buf);

					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, info, CDISP);
					ScrBrush_Api();

					break;
				}
			}

			if (is_success)
			{
				unsigned char WIFI_SSID[64] = {0};
				unsigned char WIFI_PWD[64]  = {0};
				wifiAPCheck_lib(WIFI_SSID, WIFI_PWD);

				MAINLOG_L1("WIFI_SSID: %s", WIFI_SSID);
				MAINLOG_L1("WIFI_PWD: %s",  WIFI_PWD);

				ret = SaveWholeFile_Api(WIFI_CONFIG_FILE, "1", 1);
				if (ret != 0) { MAINLOG_L1("!!! SaveWholeFile_Api('wifi_config.dat') failed(%d) !!!", ret); }

				SysPowerReBoot_Api();
			}
		}

	}

	WaitingForConfirmOrNext();
}

void WifiConfig()
{
	int nSelcItem = 1;

	char *pszTitle = "Wifi Config";
	const char *pszItems[] =
	{
		"1. Hotspot Mode",
		"2. Bluetooth Mode",
		//"3.Scan QR Code",
	};

	while (1)
	{
		nSelcItem = ShowMenuItem(pszTitle, pszItems, sizeof(pszItems) / sizeof(char *), DIGITAL1, DIGITAL2, 0, 60);

		switch (nSelcItem) {
			case DIGITAL1: ConfigWifi(2); break;
			case DIGITAL2: ConfigWifi(3); break;

			case 0x08:
			case ESC: return;

			default: break;
		}
	}
}

void clearSmallScreen()
{
	secscrOpen_lib();
	secscrCls_lib();

	// =====
	secscrSetAttrib_lib(4, 1);
	secscrPrint_lib(0, 0, 0, " ");
	// =====
}

/// ========== Wifi Config
void showFNmenu()
{
	int nSelcItem = 1, ret;

	char *pszTitle = "Function";
	const char *pszItems[] =
	{
	  "1. Wifi Config",
	  "2. Connection",
	  "3. Reset Wifi",
	  "4. Thanks Mode"
	};

	while (1) {
		// DISPLAY THE PARM MENU AND RETURN THE KEY CLICK AND TIME TO CLOSE
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
				sizeof(pszItems) / sizeof(char*), DIGITAL1, DIGITAL4, 0, 60);
		switch (nSelcItem) {
		case DIGITAL1: WifiConfig(); break;
		case DIGITAL2: SelectedNetwork(); break;
		case DIGITAL3: ResetWifi(); break;
		case DIGITAL4: ThanksMode(); break;
		case ESC:
			return;
		default:
			break;
		}
	}
}

void ThanksMode(){
	// create a globle varible then switch to 0 and 1 make sure defult it is 1 to open
	// then print to tell use
	// ThanksMode
	//   ON || OFF
}

void ShowMainMenu(void) {
	int nSelcItem = 1, ret;

	char *pszTitle = "Menu";
	const char *pszItems[] = { "1. Static QR", "2. Dynamic QR" };
	while (1) {
		// DISPLAY THE PARM MENU AND RETURN THE KEY CLICK AND TIME TO CLOSE
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
				sizeof(pszItems) / sizeof(char*), DIGITAL1, DIGITAL2, 0, 60);
		switch (nSelcItem) {
		case DIGITAL1:
			QRType(1);
			break;
		case DIGITAL2:
			QRType(2);
			break;
		case ESC:
			return;
		default:
			break;
		}
	}
}
