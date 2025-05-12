/*
 * dynamicQR.c
 *
 *  Created on: Dec 16, 2024
 *      Author: kimlean
 */
#include <coredef.h>

#include "def.h"
#include "EmvCommon.h"
#include "EnvAcleda.h"
#include "httpDownload.h"
#include "SmartPayInfo.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <poslib.h>

int g_ucKerType = 0;

int PiccInit(void)
{
	if (PiccOpen_Api() != 0x00)
		return 1;

	return 0;
}

int PiccStop(void)
{
	if (PiccClose_Api() != 0x00)
		return 1;

	return 0;
}

int PiccCheck()
{
	u8 CardType[8], SerialNo[32];
	u8 ret;

	memset(CardType, 0, sizeof(CardType));
	memset(SerialNo, 0, sizeof(SerialNo));

	ret = PiccCheck_Api(0, CardType, SerialNo);
	// MAINLOG_L1("PiccCheck_Api, ret %d, cardType %s, SerialNo %s", ret, CardType, SerialNo);
	if (ret != 0x00)
		return -1;
	else
		return 0;
}

int DetectCardEvent(u8* CardData, u8 timeoutS)
{
	u8 key;
	unsigned int timerid;

	timerid = TimerSet_Api();
	while (!TimerCheck_Api(timerid, timeoutS * 1000))
	{
		key = GetKey_Api();
		if (key == ESC)
		{
			return ESC;
		}

		if (PiccCheck() == 0x00)
		{
			return 0;
		}
		else
			continue;
	}
	return TIMEOUT;
}

int GetCard()
{
	u8 CardData[256];
	int ret, event;

	memset(CardData, 0, sizeof(CardData));
	if (PiccInit() != 0)
		return -1;

	initPayPassWaveConfig(0x00);
	CTLPreProcess();

	while (1)
	{
		event = DetectCardEvent(CardData, 60);
		switch (event)
		{
			case 0:
				Common_SetIcCardType_Api(PEDPICCCARD, 0);
				g_ucKerType = App_CommonSelKernel();

				PosCom.TranKernelType = g_ucKerType;
				if (g_ucKerType == 0)
				{
					Beep_Api(0);
					ret = 10;
				}
//				CLOSING CODE NOT ALLOW TO WORK FIST NOT DEVELOP
//				else if(g_ucKerType == TYPE_KER_PAYWAVE)
//				{
//					Beep_Api(0);
//					ret = App_PaywaveTrans(g_ucKerType);
//				}
				else
				{
					// KIMLEAN : PREPARE A MP3 FOR SAY PLEASE TRY AGAIN
					AppPlayTip("no app match");
					break;
				}
			return ret;
			case ESC:
			case TIMEOUT:
			default:
				ret = -1;
				return ret;
		}
	};

	return ret;
}

int SelectedCurrency()
{
	char input;

	ScrCls_Api(); // Clear the screen
	ScrDisp_Api(LINE1, 0, "Select Currency:", CDISP);
	ScrDisp_Api(LINE3, 0, "1. KHR", LDISP);
	ScrDisp_Api(LINE4, 0, "2. USD", LDISP);

	while (1) {
		// Assuming GetInput_Api() captures user input from keypad
		input = WaitAnyKey_Api(60);
		// MAINLOG_L1("WaitAnyKey_Api aa:%d", input);
		switch (input)
		{
		case ESC:
		case TIMEOUT:
			return -1;
		default:
			if (input == DIGITAL1)
				return 1;
			if (input == DIGITAL2)
				return 2;
			break;
		}
	}
}

#define QR_BMP "QR.bmp"
#define UPI_QRBMP_AMT "UPI_QRBMP_AMT.bmp"

#define KH_LOGO "/ext/tms/KHR.jpg"
#define USD_LOGO "/ext/tms/USD.jpg"

#define KH_GOLD_LOGO "/ext/tms/KHR_Gold.jpg"
#define USD_GOLD_LOGO "/ext/tms/USD_Gold.jpg"

void QRDisp(unsigned char* str, int QrType)
{
	QREncodeString(str, 3, 3, UPI_QRBMP_AMT, 4);
	ScrCls_Api();
//	ScrDispImage_Api(KHQR_CONCAT, 0, 0);
	ScrDispImage_Api(UPI_QRBMP_AMT, 6, 10);
	ScrDispImage_Api(QrType == 1 ? KH_GOLD_LOGO : USD_GOLD_LOGO , 105, 100);
}

// Function to calculate CRC16-CCITT-FALSE
uint16_t CalcCRC16(const char* strInput)
{
	uint16_t crc = 0xFFFF;
	size_t length = strlen(strInput);
	for (size_t i = 0; i < length; i++) {
		crc ^= ((uint16_t)strInput[i] << 8);
		for (int j = 0; j < 8; j++) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
		}
	}
	return crc;
}

// Helper: Convert currency text to numeric code
const char* getNumericCurrency(const char* ccy)
{
    if(strcmp(ccy, "KHR") == 0) return "116";
    if(strcmp(ccy, "USD") == 0) return "840";
    if(strcmp(ccy, "THB") == 0) return "764";
    if(strcmp(ccy, "LAK") == 0) return "418";
    return "";
}

// Function to convert CRC to uppercase hexadecimal string
void CRC16ToHex(uint16_t crc, char* hexStr)
{
	sprintf(hexStr, "%04X", crc);
}

char* PrepareQRContent(long amount, int currency, char* formattedAmount, NFC_PAYMENT* NFCPAYMENTREQUEST) {
    static char qrContent[512];  // Increase size to avoid truncation risk
    // Format amount with or without decimal point based on currency
    MAINLOG_L1("Log amount %ld", amount);
    snprintf(formattedAmount, 24, "%ld.%02ld", amount / 100, amount % 100);

    strncpy(NFCPAYMENTREQUEST->amount, formattedAmount, sizeof(NFCPAYMENTREQUEST->amount) - 1);
    NFCPAYMENTREQUEST->amount[sizeof(NFCPAYMENTREQUEST->amount) - 1] = '\0';

    char amountLength[3];
    snprintf(amountLength, sizeof(amountLength), "%02d", (int)strlen(formattedAmount));
    const char* merchantAccount = (currency == 1) ? SmartPay_Info.accIdKhr : SmartPay_Info.accIdUsd;
    snprintf(NFCPAYMENTREQUEST->creditAccount, sizeof(NFCPAYMENTREQUEST->creditAccount), "%s", merchantAccount);

    char merchanValue[128];
    snprintf(merchanValue, sizeof(merchanValue), "%s%s0206ACLEDA", SmartPay_Info.adDataTCGUID, merchantAccount);
    char merchantInfo[128];
    snprintf(merchantInfo, sizeof(merchantInfo), "30%02d%s", (int)strlen(merchanValue), merchanValue);

    char terminalValue[128];
    snprintf(terminalValue, sizeof(terminalValue), "00%02d%s01014", (int)strlen(SmartPay_Info.mctId), SmartPay_Info.mctId);
    char terminalInfo[128];
    snprintf(terminalInfo, sizeof(terminalInfo), "39%02d%s", (int)strlen(terminalValue), terminalValue);

    char additionalValue[128];
    snprintf(additionalValue, sizeof(additionalValue), "%s0711%s", SmartPay_Info.mbNo, G_sys_param.sn);
    char additionalData[128];
    snprintf(additionalData, sizeof(additionalData), "62%02d%s", (int)strlen(additionalValue), additionalValue);

    char merchantName[128];
    snprintf(merchantName, sizeof(merchantName), "59%02d%s", (int)strlen(SmartPay_Info.mctName), SmartPay_Info.mctName);

    // Combine into qrContent
    snprintf(qrContent, sizeof(qrContent),
        "000201010212%s%s5204%s5303%s54%s%s%s%s%s%s6304",
        merchantInfo,
        terminalInfo,
        SmartPay_Info.mmc,
        (currency == 1) ? "116" : "840",
        amountLength,
        formattedAmount,
        SmartPay_Info.counCode,
        merchantName,
        SmartPay_Info.cityName,
        additionalData);

    return qrContent;
}

void ShowQRCode(long amount, int currency, NFC_PAYMENT *NFCPAYMENTREQUEST) {
    unsigned char qrBuffer[255];
    memset(qrBuffer, 0, sizeof(qrBuffer));

    char formattedAmount[24];
    char* qrContent = PrepareQRContent(amount, currency, formattedAmount, NFCPAYMENTREQUEST);

    uint16_t crc = CalcCRC16(qrContent);
    char crcHex[5];
    CRC16ToHex(crc, crcHex);
    strcat(qrContent, crcHex);

    strncpy((char*)qrBuffer, qrContent, sizeof(qrBuffer) - 1);
    QRDisp(qrBuffer, currency);

    char displayAmount[32];
	sprintf(displayAmount, "%s %s", formattedAmount, (currency == 1) ? "KHR" : "USD");

	ScrFontSet_Api(5);
	ScrSetColor_Api(0x001F, 0xFFFF);
	ScrDisp_Api(11, 0, displayAmount, CDISP);
}

void DynamicQRCode(int currency)
{
	// Check if SmartPay_Info.regId is not empty
	if (SmartPay_Info.regId[0] == '\0')
	{
		MAINLOG_L1("Condition Check");
		NoQRDisplayDynamicQr();
		return;
	}
	int ret = 0, amt;
	unsigned char tmp[32], buf[12];
	memset(&PosCom, 0, sizeof(PosCom));

	// CRAETE A TEMP NFC PAYMENT
	NFC_PAYMENT NFCPAYMENTREQUEST;
	memset(&NFCPAYMENTREQUEST, 0, sizeof(NFCPAYMENTREQUEST));

	char CurrencyStr[20];
	if (currency == 1) {
		snprintf(CurrencyStr, sizeof(CurrencyStr), "KHR");
	}
	if (currency == 2) {
		snprintf(CurrencyStr, sizeof(CurrencyStr), "USD");
	}

	char InputAmountType[100];
	snprintf(InputAmountType, sizeof(InputAmountType), "Amount as %s:", CurrencyStr);

	// NOTE: GET INPUT AMOUNT FROM USER
	ScrCls_Api();
	ScrDisp_Api(LINE1, 0, "Sale", CDISP);
	ScrDisp_Api(LINE3, 0, InputAmountType, LDISP);
	if (GetAmount(PosCom.stTrans.TradeAmount, currency) == 0){

		// SHOW QR CODE BY THE AMOUNT
		amt = BcdToLong_Api(PosCom.stTrans.TradeAmount, 6);

		ScrCls_Api();
		snprintf(NFCPAYMENTREQUEST.ccy, sizeof(NFCPAYMENTREQUEST.ccy), CurrencyStr);
		ShowQRCode(amt, currency, &NFCPAYMENTREQUEST);

		/*
			NOTE: CALL API FOR PROCESS THE TRANSACTION
		    NOW USING ONLY ON SUCCESSS RETURN 10
			WILL SET CONDITION 10 || 3 WHEN FINISH THE NFC WITH CARD
		*/

		// KIMLEAN GETING VALUE OF THE CARDs
		ret = GetCard();
		if(ret != -1)
		{
			ScrCls_Api();
			if(NFC_INFO.EncodeMobile != NULL)
			{
				if(ret == 10)
				{
					// MOBILE NFC
					ret = NFCPayment_Service(&NFC_INFO.EncodeMobile, NFCPAYMENTREQUEST);
					MAINLOG_L1("NFCPayment_Service %d", ret);
					if(ret == 1)
					{
						// SHOW MESSAGE FAIL PROCESSING
						AppPlayTip("Processing Failed");
					}
				}
			}
			else
			{
				AppPlayTip("Please try again");
			}
		}
	}

	ScrCls_Api();
	secscrCls_lib();
	secscrSetBackLightValue_lib(0);  //secscrSetBackLightValue_lib is necessary before secscrSetBackLightMode_lib
	secscrSetBackLightMode_lib(0, 300);

	// RESERT COLOR
	ScrSetColor_Api(0x0000, 0xFFFF);
	ScrFontSet_Api(5);

    // CLEAR NFC VALUE STORING
	memset(&NFC_INFO, 0, sizeof(NFCTAPINFO));
	PiccStop();
	return;
}

void NoQRDisplayDynamicQr()
{
	ScrCls_Api();
	ScrDisp_Api(LINE1, 0, "Dynamic QR", CDISP);

	ScrFontSet_Api(3);
	ScrDisp_Api(LINE3, 0, "Account information", LDISP);
	ScrDisp_Api(LINE4, 0, "loading failed!", LDISP);

	ScrFontSet_Api(4);
	ScrSetColor_Api(0xF800, 0xFFFF);
	ScrDisp_Api(10, 0, "ERROR", CDISP);
	WaitAnyKey_Api(10);

	// RESERT COLOR
	ScrSetColor_Api(0x0000, 0xFFFF);
	ScrFontSet_Api(5);
}

