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

#define CONTENT1 "SMART PAY QR"
#define CONTENT2 "MID:qqqqqqqqqqqqq"
// Update code here
// SYS_PARAM G_sys_param;s
// Declare the global instance of the structure
void DispMainFace(void)
{
	ScrCls_Api();
	//    ScrDisp_Api(LINE4, 0, "Welcome to use", CDISP);
	//    ScrDisp_Api(LINE5, 0, "Aisino Q161ffffffffffffffff", CDISP);
}

int WaitEvent(void)
{
	u8 Key;
	int TimerId;

	TimerId = TimerSet_Api();
	while (1)
	{
		if (TimerCheck_Api(TimerId, 30 * 1000) == 1)
		{
			SysPowerStand_Api();
			return 0xfe;
		}
		Key = GetKey_Api();
		if (Key != 0)
			return Key;
	}

	return 0;
}

#define TIMEOUT -2

int ShowMenuItem(char *Title, const char *menu[], u8 ucLines, u8 ucStartKey,
				 u8 ucEndKey, int IsShowX, u8 ucTimeOut)
{
	u8 IsShowTitle, cur_screen, OneScreenLines, Cur_Line, i, t;
	int nkey;
	char dispbuf[50];

	memset(dispbuf, 0, sizeof(dispbuf));

	if (Title != NULL)
	{
		IsShowTitle = 1;
		OneScreenLines = 12;
	}
	else
	{
		IsShowTitle = 0;
		OneScreenLines = 13;
	}
	IsShowX -= 1;
	cur_screen = 0;
	while (1)
	{
		ScrClsRam_Api();
		if (IsShowTitle)
			ScrDisp_Api(LINE1, 0, Title, CDISP);
		Cur_Line = LINE1 + IsShowTitle;
		for (i = 0; i < OneScreenLines; i++)
		{
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

		switch (nkey)
		{
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

void SoundVolumeUp()
{
	G_sys_param.sound_level++;
	if (G_sys_param.sound_level > 5) { G_sys_param.sound_level = 5; }

	saveParam();

	ttsSetVolume_lib(G_sys_param.sound_level);

	if (G_sys_param.sound_level == 5) {
		PlayMP3File("Max_sound.mp3");
	} else {
		PlayMP3File("Increase.mp3");
	}
}

void SoundVolumeDown()
{
	G_sys_param.sound_level--;
	if (G_sys_param.sound_level < 0) { G_sys_param.sound_level = 0; }

	saveParam();

	if (G_sys_param.sound_level == 0) {
		PlayMP3File("Min_sound.mp3");
	}
	else if (G_sys_param.sound_level == 1) {
		ttsSetVolume_lib(G_sys_param.sound_level);
		PlayMP3File("Min_sound.mp3");
	}
	else {
		ttsSetVolume_lib(G_sys_param.sound_level);
		PlayMP3File("Decrease.mp3");
	}
}

void ActionRefresh_Tread(void)
{
	while(1)
	{
		if (g_RefreshQRDisplay)
		{
			MAINLOG_L1("ActionRefresh_Tread Start");
			CustomizedQR2(DISPLAY_QR_TYPE);
			g_RefreshQRDisplay = 0;  // Reset flag
		}
		Delay_Api(1000);
	}
}

void MenuThread(void)
{
	int Result = 0;
	unsigned char key;
	while (1)
	{
		clearSmallScreen();
		LedLightOn_Api(0x01);
		ScrBackLight_Api(0xFFFF);
		CustomizedQR2(DISPLAY_QR_TYPE);

		KBFlush_Api();
		while(1)
		{
			key = GetKey_Api();
			if(key == ENTER)
			{
				Beep_Api(0);

				// CLOSE CODE NOT SHOW DYNAMIC QR
				// ShowMainMenu();
				QRType(1);
				break;
			}
			// CLOSING CODE NOT TO SEE THE DYNAMIC QR
			if(key == FUNCTION)
			{
				Beep_Api(0);
				QRType(2);
				break;
			}
			if(key == MENU)
			{
				Beep_Api(0);
				showFNmenu();
				break;
			}
			// Up
			if (key == PGUP) { SoundVolumeUp(); }
			// Down
			if (key == PGDWON) { SoundVolumeDown(); }
		}
		Delay_Api(50);
	}
}

void CustomizedQR2(int QRType)
{
    ScrCls_Api();
    MAINLOG_L1("CustomizedQR2 => %d", QRType);

    // Check which file to display based on QR type and if it exists
    if (QRType == 1) {
        if (g_khrQrExists) {
            ScrDispImage_Api(KHQRBMP, 0, 0);
        } else {
            NoQRDisplay();
        }
    } else {
        if (g_usdQrExists) {
            ScrDispImage_Api(USDQRBMP, 0, 0);
        } else {
            NoQRDisplay();
        }
    }
}

void NoQRDisplay()
{
    ScrCls_Api();

    ScrFontSet_Api(3);
    ScrDisp_Api(LINE9, 0, "Please upload QR", CDISP);
    ScrDisp_Api(LINE10, 0, "code for display", CDISP);

    // Reset color and font
    ScrSetColor_Api(0x0000, 0xFFFF);
    ScrFontSet_Api(5);
}

/* ========== KIMLEAN DEV ========== */
void QRType(int QRType)
{
	int nSelcItem = 1;

	// HEADER TEXT
	char *pszTitle = "";
	if (QRType == 1)
	{
		pszTitle = "Static Currency";
	};
	if (QRType == 2)
	{
		pszTitle = "Dynamic Currency";
	};

	// ITEM
	const char *pszItems[] = {"1.QR KHR", "2.QR USD"};
	while (1)
	{
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
								 sizeof(pszItems) / sizeof(char *), DIGITAL1, DIGITAL2, 0, 60);
		ScrCls_Api();
		ScrClsRam_Api();
		switch (nSelcItem)
		{
			case DIGITAL1:
				if (QRType == 1)
				{
					DISPLAY_QR_TYPE = 1;
					return;
				}
				if (QRType == 2)
				{
					DynamicQRCode(1);
					return;
				}
				break;
			case DIGITAL2:
				if (QRType == 1)
				{
					DISPLAY_QR_TYPE = 2;
					return;
				}
				if (QRType == 2)
				{
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

	while (1)
	{
		key = GetKey_Api();

		if (key == 0x08 || key == ESC || key == ENTER)
		{
			if (key == ESC)
			{
				Beep_Api(0);
			}
			return;
		}
	}
}

#define QR_BMP "QR.bmp"
#define WIFI_Connect  "/ext/tms/Wi_Fi.jpg"

/// ========== Wifi Config
void ConfigWifi()
{
	int ret;
	char qrContent[256] = {0};
	char qrRouteContent[256] = {0};

	// Clear screen and display title
	ScrCls_Api();
	ScrClsRam_Api();
	ScrDispRam_Api(0, 0, "WiFi Configuration", CDISP);
	ScrDispRam_Api(5, 0, "Run Hotspot Mode", CDISP);
	ScrBrush_Api();

	// Start hotspot mode (AP_WEB_ID = 2, timeout 120 seconds)
	ret = wifiAPConnectType_lib(2, 120);
	Delay_Api(100);

	// Clear screen and display title
	ScrCls_Api();
	ScrClsRam_Api();

	ScrDispImage_Api(WIFI_Connect, 0, 0);

	ScrBrush_Api();
	if (ret != 0)
	{
		char info[32] = {0};
		sprintf(info, "Hotspot Mode Config Failed");

		char ret_buf[16] = {0};
		itoa(ret, ret_buf, 10);

		ScrDispRam_Api(6, 0, info, CDISP);
		ScrDispRam_Api(7, 0, ret_buf, CDISP);
		ScrBrush_Api();
	}
	else
	{
		// Start the web network with the configured SSID and password
		ret = wifiWebNetwork_lib(WIFI_HOTSPOT_CONFIG_SSID, WIFI_HOTSPOT_CONFIG_PWD, 60 * 1000);

		if (ret != 0)
		{
			char info[32] = {0};
			sprintf(info, "Hotspot Mode Config");

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
				if (ret == 4)
				{
					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "Waiting for config...", CDISP);
					ScrBrush_Api();

					// Display scan QR code prompt
					ScrDispRam_Api(8, 0, "Scan QR code to connect", CDISP);
					ScrBrush_Api();

					Delay_Api(3000);
					continue;
				}
				else if (ret == 0)
				{
					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "Preparing for config...", CDISP);
					ScrBrush_Api();

					Delay_Api(3000);
					continue;
				}
				else if (ret == 1)
				{
					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "Configuring...", CDISP);
					ScrBrush_Api();

					Delay_Api(3000);
					continue;
				}
				else if (ret == 3)
				{
					AppPlayTip("Configure time out");

					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "Configuring Timeout", CDISP);
					ScrBrush_Api();

					break;
				}
				else if (ret == 2)
				{
					AppPlayTip("WiFi configured");

					ScrClrLineRam_Api(6, 6);
					ScrDispRam_Api(6, 0, "WiFi Configured", CDISP);
					ScrDispRam_Api(7, 0, "Device Reboot...", CDISP);
					ScrBrush_Api();

					is_success = 1;
					break;
				}
				else
				{
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
	            saveInternetConnectionType(2);
				unsigned char WIFI_SSID[64] = {0};
				unsigned char WIFI_PWD[64] = {0};
				wifiAPCheck_lib(WIFI_SSID, WIFI_PWD);

				// Save configuration to file
				ret = SaveWholeFile_Api(WIFI_CONFIG_FILE, "1", 1);
				if (ret != 0)
				{
					// Log error if saving failed
					// MAINLOG_L1("!!! SaveWholeFile_Api('wifi_config.dat') failed(%d) !!!", ret);
				}

				// Display success message and reboot
				ScrCls_Api();
				ScrDispRam_Api(3, 0, "WiFi Configuration Success!", CDISP);
				ScrDispRam_Api(5, 0, "SSID:", LDISP);
				ScrDispRam_Api(5, 6, WIFI_SSID, LDISP);
				ScrDispRam_Api(6, 0, "Rebooting device...", CDISP);
				ScrBrush_Api();

				Delay_Api(100);
				SysPowerReBoot_Api();
			}
		}
	}

	WaitingForConfirmOrNext();
}

void clearSmallScreen()
{
	secscrCls_lib();
	// =====
	secscrSetAttrib_lib(4, 1);
	secscrPrint_lib(0, 0, 0, " ");
	// =====
}

void showFNmenu()
{
	int nSelcItem = 1, ret;

	char thxMode[20];
	snprintf(thxMode, sizeof(thxMode), "%s%s", "4.Thanks Mode", THANKS_MODE ? "(ON)" : "(OFF)");

	char *pszTitle = "Function";
	const char *pszItems[] =
			{
				"1.Wifi Config 2.4GHz",
				"2.Connection",
				"3.Reset Wifi",
				thxMode
			};

	while (1)
	{
		// DISPLAY THE PARM MENU AND RETURN THE KEY CLICK AND TIME TO CLOSE
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
								 sizeof(pszItems) / sizeof(char *), DIGITAL1, DIGITAL4, 0, 60);
		switch (nSelcItem)
		{
			case DIGITAL1:
				ConfigWifi();
				break;
			case DIGITAL2:
				SelectedNetwork();
				break;
			case DIGITAL3:
				ResetWifi();
				break;
			case DIGITAL4:
				ThanksMode();
				return;
			case ESC:
				return;
			default:
				break;
		}
	}
}

void ThanksMode(void)
{
	THANKS_MODE = (THANKS_MODE == 0) ? 1 : 0;
	// Save the updated preference
	saveThanksModeType(THANKS_MODE);
	ScrCls_Api();
	ScrDisp_Api(LINE1, 0, "Thanks Mode", CDISP);
	ScrDisp_Api(LINE4, 0, THANKS_MODE ? "ON" : "OFF", CDISP);

	WaitAnyKey_Api(3);
}

void ShowMainMenu(void)
{
	int nSelcItem = 1, ret;

	char *pszTitle = "Menu";
	const char *pszItems[] = {"1.Static QR", "2.Dynamic QR"};
	while (1)
	{
		// DISPLAY THE PARM MENU AND RETURN THE KEY CLICK AND TIME TO CLOSE
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
								 sizeof(pszItems) / sizeof(char *), DIGITAL1, DIGITAL2, 0, 60);
		switch (nSelcItem)
		{
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
