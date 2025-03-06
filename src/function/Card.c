//#include <coredef.h>
//
//#include <stdio.h>
//#include <string.h>
//
//#include "def.h"
//#include "cJSON.h"
//#include "EmvCommon.h"
//
//#include <MQTTClient.h>
//#include <MQTTFreeRTOS.h>
//
//int CARD_MODE_FLAG = 0;
//
//int PAID_WITH_CARD = 0;
//int CARD_TXN_PLAY_FAILED = 0;
//
//extern int GetAmount(u8 *pAmt);
//extern int GetEmvTrackData(u8 * pTrackBuf);
//
//unsigned char CARD_API_SECURE_KEY[32];
//
//int g_ucKerType = 0;
//int is_card_txn_canceled = 0;
//
///// ========== PICC BLOCK ==========
//int PiccInit(void)
//{
//	if (PiccOpen_Api() != 0x00) { return 1; }
//	else {
//		card_txn_enabled = 0;
//		return 0;
//	}
//}
//int PiccStop(void)
//{
//	card_txn_enabled = 0;
//
//	if (PiccClose_Api() != 0x00) { return 1; }
//	return 0;
//}
//int PiccCheck()
//{
//	u8 ret = -1;
//	u8 CardType[8] = {0}, SerialNo[24] = {0};
//
//	ret = PiccCheck_Api(0, CardType, SerialNo);
//
//	if (ret != 0x00) {
//		return -1;
//	}
//	else {
//		int card_type_len = strlen(CardType);
//		int serial_no_len = strlen(SerialNo);
//
//		char info[3 + card_type_len + serial_no_len];
//		memset(info, 0, sizeof(info));
//
//		char serial_no_asc[24] = "";
//		BcdToAsc_Api(serial_no_asc, SerialNo, 24);
//
//		sprintf(info, "%s | %s", CardType, serial_no_asc);
//
//		LogPrintInfo("========== CARD INFO ==========");
//		LogPrintInfo(info);
//		LogPrintInfo("===============================");
//
//		return 0;
//	}
//}
///// ========== PICC BLOCK ==========
//
//static void *ctran_connect = NULL; // card transaction connection
//
//void EmvSaveDataOnline(void)
//{
//    int iLength = 0, ret;
//	u8 Buf[100], trackData[256];
//
//	ret = Common_GetTLV_Api(0x5f20, (u8*)PosCom.stTrans.HoldCardName, &iLength);
//	if (ret != 0) LogPrintWithRet(1, "!!! Common_GetTLV_Api(0x5f20) failed(%d) !!!", ret);
//
//	LogPrintWithRet(0, "iLength = ", iLength);
//
//	PosCom.stTrans.HoldCardName[iLength] = 0;
//
//	memset(trackData, 0, sizeof(trackData));
//
//	ret = GetEmvTrackData(trackData);
//	if (ret != 0) LogPrintWithRet(1, "!!! GetEmvTrackData() failed(%d) !!!", ret);
//
//	memset(Buf, 0, sizeof(Buf));
//
//    ret = Common_GetTLV_Api(0x5A, Buf, &iLength);
//    if (ret == 0) {
//    	BcdToAsc_Api(PosCom.stTrans.MainAcc, Buf, iLength * 2);
//
//    	if (PosCom.stTrans.MainAcc[iLength * 2 - 1] == 'F') {
//    		PosCom.stTrans.MainAcc[iLength * 2 - 1] = 0;
//    	}
//    }
//    else {
//    	LogPrintWithRet(1, "!!! Common_GetTLV_Api(0x5A) failed(%d) !!!", ret);
//
//    	ret = Common_GetTLV_Api(0x57, Buf, &iLength);
//    	if (ret != 0) LogPrintWithRet(1, "!!! Common_GetTLV_Api(0x57) failed(%d) !!!", ret);
//    }
//
//    memset(Buf, 0, sizeof(Buf));
//
//	ret = Common_GetTLV_Api(0x5F24, Buf, &iLength);
//	if (ret != 0) LogPrintWithRet(1, "!!! Common_GetTLV_Api(0x5F24) failed(%d) !!!", ret);
//
//	// =============== EXPIRE DATE MOCK
//	AscToBcd_Api(PosCom.stTrans.ExpDate, "1225", 4);
//	/*
//	if (ret == EMV_OK) {
//		memcpy(PosCom.stTrans.ExpDate, Buf, 3);
//	}
//	*/
//	// =============== EXPIRE DATE MOCK
//
//	memset(Buf, 0, sizeof(Buf));
//
//	ret = Common_GetTLV_Api(0x4F, Buf, &iLength);
//	if (ret != 0) LogPrintWithRet(1, "!!! Common_GetTLV_Api(0x4F) failed(%d) !!!", ret);
//
//	BcdToAsc_Api(PosCom.stTrans.szAID, Buf, 2 * iLength);
//
//	ret = Common_GetTLV_Api(0x95, PosCom.stTrans.sTVR, &iLength);
//	if (ret != 0) LogPrintWithRet(1, "!!! Common_GetTLV_Api(0x95) failed(%d) !!!", ret);
//
//	ret = Common_GetTLV_Api(0x9B, PosCom.stTrans.sTSI, &iLength);
//	if (ret != 0) LogPrintWithRet(1, "!!! Common_GetTLV_Api(0x9B) failed(%d) !!!", ret);
//}
//
//int BuildSendRecvParse()
//{
//	cJSON *root = 0;
//	int amt, errCode, ret, i, tlen, tret, inDatalen;
//
//	char *out = 0, *inData = 0, *outData = 0;
//	char sendBuf[2048], recvBuf[2048], tmp[256], tval[128], data[1024];
//
//	char tagList1[][5] = {
//		"57", 	"84",   "5F2A", "4F",   "5F34", "5F20", "82",   "9F39", "95",   "9C",   "9F02", "9F03", "9F09", "9F10",
//		"9F1A", "9F1E", "9F26", "9F27", "9F33", "9F34", "9F6C", "9F6E", "9F35", "9F36", "9F37", "9F12", "5F24", "9F06",
//		"9F41", "9B",   "9A",	"8E",	"5F25",	"5F30",	"9F07",	"9F0D",	"9F0E",	"9F0F", "9F16",	"9F21",	"9F04", "94"
//	};
//	int tagList2[] = {
//		0x57, 	0x84, 	0x5F2A, 0x4F,   0x5F34, 0x5F20, 0x82,   0x9F39, 0x95,   0x9C, 	0x9F02, 0x9F03, 0x9F09, 0x9F10,
//		0x9F1A, 0x9F1E, 0x9F26, 0x9F27, 0x9F33, 0x9F34, 0x9F6C, 0x9F6E, 0x9F35, 0x9F36, 0x9F37, 0x9F12, 0x5F24, 0x9F06,
//		0x9F41, 0x9B,   0x9A,   0x8E,   0x5F25, 0x5F30, 0x9F07, 0x9F0D, 0x9F0E, 0x9F0F, 0x9F16, 0x9F21, 0x9F04, 0x94
//	};
//
//	EmvSaveDataOnline();
//
//	// Build Card Json
//	root = NULL;
//	root = cJSON_CreateObject();
//	if (root == NULL) {
//		LogPrintInfo("!!! root == NULL, Error !!!");
//	}
//	cJSON_AddStringToObject((cJSON *)root, "cardhdlrname", PosCom.stTrans.HoldCardName);
//
//	memset(tmp, 0, sizeof(tmp));
//	memcpy(tmp, PosCom.stTrans.MainAcc, 4);
//
//	cJSON_AddStringToObject((cJSON *)root, "cardlstfourdigits", tmp);
//
//	memset(tmp, 0, sizeof(tmp));
//	memcpy(tmp, PosCom.stTrans.MainAcc, 6);
//
//	cJSON_AddStringToObject((cJSON *)root, "cardlstfirstsixdigits", tmp);
//
//	memset(tmp, 0, sizeof(tmp));
//	BcdToAsc_Api(tmp, PosCom.stTrans.ExpDate, 4);
//	cJSON_AddStringToObject((cJSON *)root, "cardexpiry", tmp);
//
//	memset(tmp, 0, sizeof(tmp));
//	amt = BcdToLong_Api(PosCom.stTrans.TradeAmount, 6);
//	sprintf(tmp, "%d.%02d", amt / 100, amt % 100);
//	cJSON_AddStringToObject((cJSON *)root, "amount", tmp);
//
//	cJSON_AddStringToObject((cJSON *)root, "applicationidentifier", PosCom.stTrans.szAID);
//
//	memset(tmp, 0, sizeof(tmp));
//	BcdToAsc_Api(tmp, PosCom.stTrans.sTVR, 10);
//	cJSON_AddStringToObject((cJSON *)root, "tvr", tmp);
//
//	memset(tmp, 0, sizeof(tmp));
//	BcdToAsc_Api(tmp, PosCom.stTrans.sTSI, 4);
//	cJSON_AddStringToObject((cJSON *)root, "tsi", tmp);
//
//	cJSON_AddStringToObject((cJSON *)root, "devicesrno", G_sys_param.sn);
//
//	out = cJSON_PrintUnformatted(root);
//	LogPrintInfo("\nJSON BEFORE ENCRYPT:");
//	LogPrintInfo(out);
//
//	inDatalen = strlen(out);
//
//	if (inDatalen % 16) {
//		inDatalen = ((inDatalen + 15) / 16) * 16;
//	}
//
//	inData = (char *)malloc(inDatalen + 1);
//	if (inData == NULL)
//	{
//		//out = NULL;
//		free(out);
//
//		return -1;
//	}
//
//	outData = (char *)malloc(inDatalen + 1);
//	if (outData == NULL)
//	{
//		//out = NULL;
//		free(out);
//
//		//inData = NULL;
//		free(inData);
//
//		return -1;
//	}
//
//	memset(inData, 0, 	inDatalen + 1);
//	memcpy(inData, out, strlen(out));
//
//	int outLen = strlen(out);
//
//	if (outLen % 16) {
//		int abc = 16 - outLen % 16;
//
//		for (int i = 0; i < abc; ++i) {
//			inData[inDatalen - 1 - i] = abc;
//		}
//	}
//
//	//out = NULL;
//	free(out);
//
//	cJSON_Delete(root);
//
//	memset(outData, 0, inDatalen + 1);
//
//	ret = calcAesEnc_lib(inData, inDatalen, outData, CARD_API_SECURE_KEY, AES_KEY_LEN, NULL, AES_ECB);
//	if (ret != 0) {
//		LogPrintWithRet(1, "!!! calcAesEnc_lib() failed(%d) !!!", ret);
//		return ret;
//	}
//
//	int outData_len = strlen(outData);
//
//	if (LOG_FLAG)
//	{
//		char output[inDatalen + 1];
//		memset(output, 0, sizeof(output));
//
//		int result = -1;
//		result = calcAesDec_lib(outData, outData_len, output, CARD_API_SECURE_KEY, AES_KEY_LEN, NULL, AES_ECB);
//
//		if (result == 0) {
//			LogPrintInfo("========== AES DEC ==========");
//			LogPrintInfo(output);
//			LogPrintInfo("=============================");
//		} else {
//			LogPrintWithRet(1, "!!! calcaAesDec_lib() failed(%d) !!!", result);
//		}
//	}
//
//	//inData = NULL;
//	free(inData);
//
//	// Base64
//	memset(data, 0, sizeof(data));
//	int len = base64Encode(outData, data, inDatalen);
//
//	if (len <= 0) {
//		LogPrintWithRet(1, "!!! base64Encode() failed(%d) !!!", len);
//	}
//	else
//	{
//		if (LOG_FLAG)
//		{
//			LogPrintInfo("\nBASE64:");
//			LogPrintInfo(data);
//		}
//	}
//
//	//outData = NULL;
//	free(outData);
//
//	memset(sendBuf, 0, sizeof(sendBuf));
//
//	for (i = 0; i < sizeof(tagList1) / 5; ++i) {
//		memset(tval, 0, sizeof(tval));
//
//		tret = Common_GetTLV_Api(tagList2[i], (unsigned char *)tval, &tlen);
//
//		sprintf(sendBuf + strlen((char *)sendBuf), tagList1[i], strlen(tagList1[i]));
//
//		if (tret != 0)
//		{
//			char info[32] = "";
//			sprintf(info, "tagList2[%02x] = %d, failed", tagList2[i], tret);
//
//			LogPrintInfo(info);
//
//			#ifdef _TEST_DATA_
//				if (tagList2[i] == 0x5F24) {
//					sprintf(sendBuf + strlen((char *)sendBuf), "%s", "021225");
//				} else {
//					sprintf(sendBuf + strlen((char *)sendBuf), "%02x", 0);
//				}
//			#else
//				// Failed-Tag Set Value of 0
//				sprintf(sendBuf + strlen((char *)sendBuf), "%02x", 0);
//			#endif
//		}
//		else
//		{
//			if (tagList2[i] == 0x57)
//			{
//				char pucIV[8]   = {0};
//				char ksnOut[10] = {0};
//
//				tlen = ((tlen + 7) / 8) * 8;
//
//				memset(tmp, 0, sizeof(tmp));
//				BcdToAsc_Api(tmp, tval, 2 * tlen);
//
//				char info[156] = "";
//				sprintf(info, "TVAL(asc) = %s", tmp);
//				LogPrintInfo(info);
//
//				#ifdef _TEST_DATA_
//					memset(tval, 0, sizeof(tval));
//					sprintf(tval, "%s", "4514617637440464D251220112143148");
//
//					char tval_bcd[64] = "";
//					AscToBcd_Api(tval_bcd, tval, sizeof(tval_bcd));
//				#endif
//
//				memset(tmp, 0, sizeof(tmp));
//
//				#ifdef _TEST_DATA_
//					ret = PedDukptTdes_Api(DUKPT_DAT_KEY_INJECTION_GROUP_INDEX, 2, 0, pucIV, tval_bcd, tlen, DES_ENCRYPTION_CBC_MODE, tmp, ksnOut);
//				#else
//					ret = PedDukptTdes_Api(DUKPT_DAT_KEY_INJECTION_GROUP_INDEX, 2, 0, pucIV, tval, tlen, DES_ENCRYPTION_CBC_MODE, tmp, ksnOut);
//				#endif
//
//				if (ret != 0) {
//					LogPrintWithRet(1, "!!! PedDukptTdes_Api() failed(%d) !!!", ret);
//				}
//				else
//				{
//					if (LOG_FLAG)
//					{
//						char tmp_asc[64] = "";
//						BcdToAsc_Api(tmp_asc, tmp, 32);
//
//						LogPrintInfo("========== TDES ENC ==========");
//						LogPrintInfo(tmp_asc);
//						LogPrintInfo("=============================");
//
//						char output[256] = "";
//						int result = PedDukptTdes_Api(DUKPT_DAT_KEY_INJECTION_GROUP_INDEX, 2, 0, pucIV, tmp, strlen(tmp), DES_DECRYPTION_CBC_MODE, output, ksnOut);
//
//						if (result == 0)
//						{
//							char output_asc[64] = "";
//							BcdToAsc_Api(output_asc, output, 32);
//
//							LogPrintInfo("========== TDES DEC ==========");
//							LogPrintInfo(output_asc);
//							LogPrintInfo("=============================");
//						}
//						else {
//							LogPrintWithRet(1, "!!! PedDukptTdes_Api() failed(%d) !!!", result);
//						}
//					}
//				}
//
//				sprintf(sendBuf + strlen((char *)sendBuf), "%02x", tlen);
//				BcdToAsc_Api(sendBuf + strlen((char *)sendBuf), (unsigned char *)tmp, 2 * tlen);
//
//				if (LOG_FLAG)
//				{
//					memset(tmp, 0, sizeof(tmp));
//					BcdToAsc_Api(tmp, ksnOut, 20);
//
//					memset(info, 0, sizeof(info));
//					sprintf(info, "KSN = %s", tmp);
//
//					LogPrintInfo(info);
//				}
//			}
//			else {
//				sprintf(sendBuf + strlen((char *)sendBuf), "%02x", tlen);
//				BcdToAsc_Api(sendBuf + strlen((char *)sendBuf), (unsigned char *)tval, 2 * tlen);
//			}
//		}
//	}
//
//	LogPrintWithRet(0, "ST_TRANS_ENTRY_MODE = ", (int)PosCom.stTrans.EntryMode[1]);
//
//	if (PosCom.stTrans.EntryMode[1] == PIN_NOT_INPUT)
//	{
//		//sprintf(sendBuf + strlen((char *)sendBuf), "%s", "9900");
//		// do nothing.
//	}
//	else {
//		sprintf(sendBuf + strlen((char *)sendBuf), "%s", "9908");
//		BcdToAsc_Api(sendBuf + strlen((char *)sendBuf), PosCom.sPIN, 16);
//	}
//
//	memset(recvBuf, 0, sizeof(recvBuf));
//	sprintf(recvBuf + strlen((char *)recvBuf), "%s", "C00A");
//
//	memset(tmp, 0, sizeof(tmp));
//	ret = PedGetDukptKSN_Api(DUKPT_DAT_KEY_INJECTION_GROUP_INDEX, tmp);
//
//	if (ret != 0) {
//		LogPrintWithRet(1, "!!! PedGetDukptKSN_Api() failed(%d) !!!", ret);
//		return ret;
//	}
//
//	BcdToAsc_Api(recvBuf + strlen((char *)recvBuf), tmp, 20);
//
//	if (PosCom.stTrans.EntryMode[1] == PIN_NOT_INPUT)
//	{
//		sprintf(recvBuf + strlen((char *)recvBuf), "%s", "C100");
//	}
//	else
//	{
//		sprintf(recvBuf + strlen((char *)recvBuf), "%s", "C10A");
//
//		memset(tmp, 0, sizeof(tmp));
//		ret = PedGetDukptKSN_Api(DUKPT_PIN_KEY_INJECTION_GROUP_INDEX, tmp);
//
//		if (ret != 0) {
//			LogPrintWithRet(1, "!!! PedGetDukptKSN_Api() failed(%d) !!!", ret);
//			return ret;
//		}
//
//		BcdToAsc_Api(recvBuf + strlen((char *)recvBuf), tmp, 20);
//	}
//
//	sprintf(recvBuf + strlen((char *)recvBuf), "%s",   "C282");
//	sprintf(recvBuf + strlen((char *)recvBuf), "%04x", strlen(sendBuf) / 2);
//
//	sprintf(recvBuf + strlen((char *)recvBuf), "%s", sendBuf);
//
//	if (LOG_FLAG)
//	{
//		LogPrintInfo("==========TLVDate==========");
//		LogPrintInfo(recvBuf);
//		LogPrintInfo("===========================");
//	}
//
//	// Build Json Data
//	root = NULL;
//	root = cJSON_CreateObject();
//	if (root == NULL) {
//		LogPrintInfo("!!! root == NULL, Error !!!");
//	}
//
//	cJSON_AddStringToObject((cJSON *)root, "Data",     data);
//	cJSON_AddStringToObject((cJSON *)root, "app_name", "AcledaBank Q161 App");
//	cJSON_AddStringToObject((cJSON *)root, "TLVDate",  recvBuf);
//
//	out = cJSON_PrintUnformatted(root);
//	LogPrintInfo("\nJSON AFTER ENCRYPT:");
//	LogPrintInfo(out);
//
//	memset(sendBuf, 0, sizeof(sendBuf));
//
//	strcat((char *)sendBuf, "POST /SaleApi6/api/Sale/SoundBoxTxnProcessor HTTP/1.1\r\n");
//	strcat((char *)sendBuf, "Host: \r\n");
//	strcat((char *)sendBuf, "Accept: */*\r\n");
//	strcat((char *)sendBuf, "Content-Type:application/json-patch+json\r\n");
//
//	sprintf((char *)(sendBuf + strlen(sendBuf)), "Content-Length: %d\r\n", strlen(out));
//
//	strcat((char *)sendBuf, "User-Agent: Mozilla/4.0(compatible; MSIE 5.5; Windows 98)\r\n");
//	strcat((char *)sendBuf, "Connection: Keep-Alive\r\n\r\n");
//	strcat((char *)sendBuf, out);
//
//	//out = NULL;
//	free(out);
//
//	cJSON_Delete(root);
//
//	#ifdef OFFLINE_TRAN
//		ret = 0;
//	#else
//
//		ScrClrLine_Api(6, 6);
//		ScrBrush_Api();
//		ScrDisp_Api(6, 0, "Processing...", CDISP);
//
//
//		SecScrDisplay("Processing...");
//		PlayMP3("Processing", "Processing");
//
//		ScrFontSet_Api(5);
//
//		// CONNECT
//		int reconnect_count = 0;
//		while (ctran_connect == NULL)
//		{
//			ctran_connect = net_connect(NULL, CARDHOST_IP, CARDHOST_PORT, 60000, 1, &errCode);
//			LogPrintWithRet(1, "!!! TRY CONNECT TO CARD HOST(%d) !!!", reconnect_count);
//
//			reconnect_count++;
//
//			if (reconnect_count >= 3) {
//				LogPrintWithRet(1, "!!! ERROR CAN NOT CONNECT TO CARD HOST(%d) !!!", errCode);
//				return E_ERR_CONNECT;
//			}
//		}
//
//		// SEND
//		int resend_count = 0;
//
//		NET_WRITE:
//			ret = net_write(ctran_connect, (unsigned char *)sendBuf, strlen(sendBuf), 10000);
//
//		while (ret < 0) {
//			LogPrintWithRet(1, "!!! TRY SEND DATA TO CARD HOST(%d) !!!", resend_count);
//			resend_count++;
//
//			if (resend_count >= 3) {
//				LogPrintWithRet(1, "!!! ERROR SEND DATA TO CARD HOST(%d) !!!", ret);
//				return E_SEND_PACKET;
//			} else {
//				goto NET_WRITE;
//			}
//		}
//
//		// RECEIVE
//		int receive_count = 0;
//
//		NET_READ:
//			memset(recvBuf, 0, sizeof(recvBuf));
//
//			ret = net_read(ctran_connect, (unsigned char *)recvBuf, 2000, 10000);
//
//		while (ret < 0 || ret == 0) {
//			LogPrintWithRet(1, "!!! TRY RECEIVE DATA FROM CARD HOST(%d) !!!", receive_count);
//			receive_count++;
//
//			if (receive_count >= 3) {
//				LogPrintWithRet(1, "!!! ERROR RECEIVE DATA FROM CARD HOST(%d) !!!", ret);
//				return E_RECV_PACKET;
//			} else {
//				goto NET_READ;
//			}
//		}
//	#endif
//
//	#ifdef OFFLINE_TRAN
//		//data = { "responsecode": "00", "msg": "Unexpected character encountered while parsing
//		//value: T. Path '', line 0, position 0."}
//		//resolve data .
//		memset(recvBuf, 0, sizeof(recvBuf));
//		strcpy(recvBuf, "HTTP/1.1 200 OK\r\nContent-Length: 119\r\n\r\n{ \"responsecode\": \"00\", \"msg\": \"Unexpected character encountered while parsing value: T. Path '', line 0, position 0.\"}");
//	#endif
//
//	ret = ResolveHTTPPacket((unsigned char*)recvBuf);
//	if (ret != 0) LogPrintWithRet(1, "!!! ResolveHTTPPacket() failed(%d) !!!", ret);
//
//	return ret;
//}
//
//int DetectCardEvent(u8 *CardData, u8 timeoutS)
//{
//	u8 key;
//	unsigned int timerid;
//
//	timerid = TimerSet_Api();
//	while (!TimerCheck_Api(timerid, timeoutS * 1000))
//	{
//		key = GetKey_Api();
//
//		if (key == ESC)
//		{
//			LogPrintInfo("!!! CARD TRANSACTION CANCELLED !!!");
//			return ESC;
//		}
//
//		if (PiccCheck() == 0x00)
//		{
//			scrSetBackLightMode_Api(1, 60 * 1000);
//			return 0;
//		}
//		else {
//			continue;
//		}
//	}
//
//	return TIMEOUT;
//}
//
//void showCardDetectInfo()
//{
//	ScrClrLine_Api(6, 6);
//	ScrBrush_Api();
//	ScrDisp_Api(6, 0, "Card Detected", CDISP);
//
//	AppPlayTip("Card detected");
//}
//
//void showCardType(int cardType)
//{
//	char cardTypeName[16] = "";
//
//	switch (cardType) {
//		case TYPE_KER_PAYWAVE: sprintf(cardTypeName, "%s", "PayWave"); break;
//		case TYPE_KER_PAYPASS: sprintf(cardTypeName, "%s", "PayPass"); break;
//		case TYPE_KER_RUPAY:   sprintf(cardTypeName, "%s", "RuPay");   break;
//
//		default: sprintf(cardTypeName, "%s", "Unknown"); break;
//	}
//
//	ScrClrLine_Api(6, 6);
//	ScrBrush_Api();
//
//	strcat(cardTypeName, " Card");
//	ScrDisp_Api(6, 0, cardTypeName, CDISP);
//}
//
//int GetCard()
//{
//	u8 CardData[256] = {0};
//	int ret = -1, event;
//
//	initCTLSConfig(0x00);
//	CTLPreProcess();
//
//	PedDukptIncreaseKsn_Api(DUKPT_DAT_KEY_INJECTION_GROUP_INDEX);
//	PedDukptIncreaseKsn_Api(DUKPT_PIN_KEY_INJECTION_GROUP_INDEX);
//
//	ScrDisp_Api(6, 0, "Tap Card...", FDISP | CDISP);
//	AppPlayTip("Please tap card");
//
//	while (1)
//	{
//		event = DetectCardEvent(CardData, 90);
//		LogPrintWithRet(0, "DetectCardEvent(): ", event);
//
//		switch (event)
//		{
//			case 0:
//			{
//				ret = Common_SetIcCardType_Api(PEDPICCCARD, 0);
//				if (ret != 0) { LogPrintWithRet(1, "!!! Common_SetIcCardType_Api() failed(%d) !!!", ret); }
//
//				g_ucKerType = App_CommonSelKernel();
//				LogPrintWithRet(0, "App_CommonSelKernel(): g_ucKerType = ", g_ucKerType);
//
//				PosCom.TranKernelType = g_ucKerType;
//
//				showCardDetectInfo();
//				Delay_Api(500); // Just for Show message
//
//				int result = -1;
//				if (g_ucKerType == TYPE_KER_PAYWAVE) // 3
//				{
//					result = App_PaywaveTrans(g_ucKerType);
//					LogPrintWithRet(0, "App_PaywaveTrans(): ", result);
//				}
//				else if (g_ucKerType == TYPE_KER_PAYPASS) // 7
//				{
//					result = App_PaypassTrans(g_ucKerType);
//					LogPrintWithRet(0, "App_PaypassTrans(): ", result);
//				}
//				else if (g_ucKerType == TYPE_KER_RUPAY) // 8
//				{
//					result = App_RupayTrans(g_ucKerType);
//					LogPrintWithRet(0, "App_RupayTrans(): ", result);
//				}
//				else
//				{
//					Beep_Api(1);
//					showCardType(g_ucKerType);
//
//					PlayMP3("No_app_match", "No App Match");
//					result = -1;
//				}
//
//				if (result != 0) {
//					sprintf(PosCom.stTrans.szRespCode, "%d", result);
//				}
//
//				//result = 0;
//				return result;
//			}
//
//			case ESC:
//			{
//				Beep_Api(1);
//
//				ret = -1000;
//				return ret;
//			}
//
//			case TIMEOUT:
//			{
//				Beep_Api(1);
//
//				ret = -2000;
//				return ret;
//			}
//
//			default:
//			{
//				LogPrintInfo("!!! DETECT CARD EVENT DEFAULT CASE !!!");
//
//				ret = -1;
//				return ret;
//			}
//		}
//	}
//
//	//ret = 0;
//	return ret;
//}
//
//void TransTapCard()
//{
//	int ret;
//	unsigned int amt;
//	unsigned char buf[12];
//
//	memset(PosCom.stTrans.szRespMsg,  0, sizeof(PosCom.stTrans.szRespMsg));
//	memset(PosCom.stTrans.szRespCode, 0, sizeof(PosCom.stTrans.szRespCode));
//
//	ret = GetCard();
//	LogPrintWithRet(0, "GetCard(): ", ret);
//
//	PiccStop();
//
//	// amt 2 buf
//	amt = BcdToLong_Api(PosCom.stTrans.TradeAmount, 6);
//
//	memset(buf, 0, sizeof(buf));
//	sprintf((char *)buf, "%d.%02d", amt / 100, amt % 100);
//
//	if (ret != 0)
//	//if (0) // DEBUG
//	{
//		if (ret == -1000)
//		{
//			ScrClrLine_Api(6, 6);
//			ScrBrush_Api();
//
//			ScrDisp_Api(6, 0, "Payment Canceled", CDISP);
//
//			return;
//		}
//		else if (ret == -2000) // 1m30s
//		{
//			ScrClrLine_Api(6, 6);
//			ScrBrush_Api();
//
//			ScrDisp_Api(6, 0, "Timeout", CDISP);
//
//			return;
//		}
//		else
//		{
//			SecScrDisplay("Card Transaction Failed");
//			AppPlayTip("Card transaction failed");
//
//			ScrFontSet_Api(5);
//
//			Delay_Api(3000);
//
//			ScrCls_Api();
//			ScrDisp_Api(0, 0, "Card Payment", CDISP);
//			ScrDisp_Api(5, 0, CURRENCY_AMT_INFO, FDISP | CDISP);
//
//			int resp_code_len = strlen(PosCom.stTrans.szRespCode);
//			LogPrintWithRet(0, "resp_code_len = ", resp_code_len);
//
//			if (resp_code_len <= 2)
//			{
//				char info[24] = {0};
//				sprintf(info, "Paid Failed: %d", ret);
//				ScrDisp_Api(6, 0, info, CDISP);
//			}
//			else
//			{
//				char info[14 + resp_code_len];
//				memset(info, 0, sizeof(info));
//				sprintf(info, "Paid Failed: %s", PosCom.stTrans.szRespCode);
//
//				ScrDisp_Api(6, 0, info, CDISP);
//			}
//
//			int respMsgLen = 0;
//			respMsgLen = strlen(PosCom.stTrans.szRespMsg);
//			LogPrintWithRet(0, "respMsgLen = ", respMsgLen);
//
//			if (respMsgLen != 0)
//			{
//				char info[256] = {0};
//				sprintf(info, "Error: %s", PosCom.stTrans.szRespMsg);
//				LogPrintInfo(info);
//
//				showCardTxnRespMsg(respMsgLen);
//			}
//
//			Delay_Api(3000);
//
//			return;
//		}
//	}
//	else
//	{
//		Delay_Api(2 * 1000); // Just for show message
//		Beep_Api(0);
//
//		ScrClrLine_Api(6, 6);
//		ScrBrush_Api();
//
//		ScrDisp_Api(6, 0, "Paid Success", CDISP);
//		SecScrDisplay("Card Paid Success");
//
//		ScrFontSet_Api(5);
//	}
//}
//
//void closeCardTrans()
//{
//	is_close_detect_card = 1;
//
//	PiccStop();
//	card_txn_enabled = 0;
//}
//
///// ========== CARD INIT ==========
//int CommonCardInit()
//{
//	int ret = 0, ret_sum = 0;
//
//	// Common
//	ret = Common_Init_Api();
//	if (ret != 0) { CardInitErrorPrint("Common", ret); }
//	ret_sum += ret;
//
//	Common_DbgEN_Api(1); // Enable DUKPT Log of "/ext/app/data/ComLog.dat" (PS: Must invoke after 'Common_Init_Api')
//
//	// ========================== CARDS ==========================
//	//ret = Amex_Init_Api();    if (ret != 0) { CardInitErrorPrint("Amex",    ret); }
//	//ret_sum += ret;
//
//	//ret = DPAS_Init_Api();    if (ret != 0) { CardInitErrorPrint("DPAS",    ret); }
//	//ret_sum += ret;
//
//	//ret = Mir_Init_Api();     if (ret != 0) { CardInitErrorPrint("Mir",     ret); }
//	//ret_sum += ret;
//
//	ret = PayPass_Init_Api(); if (ret != 0) { CardInitErrorPrint("PayPass", ret); }
//	ret_sum += ret;
//
//	ret = PayWave_Init_Api(); if (ret != 0) { CardInitErrorPrint("PayWave", ret); }
//	ret_sum += ret;
//
//	//ret = Pure_Init_Api();    if (ret != 0) { CardInitErrorPrint("Pure",    ret); }
//	//ret_sum += ret;
//
//	//ret = qUICS_Init_Api();   if (ret != 0) { CardInitErrorPrint("qUICS",   ret); }
//	//ret_sum += ret;
//
//	ret = RUPAY_Init_Api();   if (ret != 0) { CardInitErrorPrint("RuPay",   ret); }
//	ret_sum += ret;
//	// ========================== CARDS ==========================
//
//	LogPrintInfo("==================== Add App/Capk Start ====================");
//	PayPassAddAppExp(0);
//	PayWaveAddAppExp();
//	RuPayAddAppExp();
//
//	AddCapkExample();
//	LogPrintInfo("===================== Add App/Capk End =====================");
//
//	return ret_sum;
//}
//void CardInitErrorPrint(char *type, int errorRet)
//{
//	char tmp[48] = {0};
//	sprintf(tmp, "!!! %s INIT FAILED(%d) !!!", type, errorRet);
//
//	LogPrintInfo(tmp);
//}
///// ========== CARD INIT ==========
//
//// 0: card detected, ESC: TXN cancelled, -1: timeout
//int detectCard(u8 timeoutS)
//{
//	u8 key;
//	unsigned int timerid;
//	int is_timeout = 0;
//
//	timerid = TimerSet_Api();
//
//	while (!is_timeout)
//	{
//		key = GetKey_Api();
//
//		is_timeout = TimerCheck_Api(timerid, timeoutS * 1000);
//
//		if (key == ESC) { return ESC; }
//
//		if (PiccCheck() == 0x00)
//		{
//			ScrBackLight_Api(30);
//			return 0;
//		}
//		else { continue; }
//	}
//
//	return CARD_TXN_TIMEOUT;
//}
//
//void CardOnlyTxnProcess()
//{
//	int ret = -1, event;
//
//	initCTLSConfig(0x00);
//	CTLPreProcess();
//
//	PedDukptIncreaseKsn_Api(DUKPT_DAT_KEY_INJECTION_GROUP_INDEX);
//	PedDukptIncreaseKsn_Api(DUKPT_PIN_KEY_INJECTION_GROUP_INDEX);
//
//	while (1)
//	{
//		event = detectCard(90);
//		LogPrintWithRet(0, "detectCard(): ", event);
//
//		switch (event)
//		{
//			case 0:
//				ret = Common_SetIcCardType_Api(PEDPICCCARD, 0);
//				if (ret != 0) LogPrintWithRet(1, "!!! Common_SetIcCardType_Api() failed(%d) !!!", ret);
//
//				g_ucKerType = App_CommonSelKernel();
//				LogPrintWithRet(0, "App_CommonSelKernel(): g_ucKerType = ", g_ucKerType);
//
//				PosCom.TranKernelType = g_ucKerType;
//
//				if (g_ucKerType == TYPE_KER_PAYWAVE) // 3
//				{
//					showCardDetectInfo();
//
//					ret = App_PaywaveTrans(g_ucKerType);
//					LogPrintWithRet(0, "App_PaywaveTrans(): ", ret);
//				}
//				else if (g_ucKerType == TYPE_KER_PAYPASS) // 7
//				{
//					showCardDetectInfo();
//
//					ret = App_PaypassTrans(g_ucKerType);
//					LogPrintWithRet(0, "App_PaypassTrans(): ", ret);
//				}
//				else if (g_ucKerType == TYPE_KER_RUPAY) // 8
//				{
//					showCardDetectInfo();
//
//					ret = App_RupayTrans(g_ucKerType);
//					LogPrintWithRet(0, "App_RupayTrans(): ", ret);
//				}
//				else {
//					showCardType(g_ucKerType);
//
//					AppPlayTip("No app match");
//					ret = CARD_TXN_NO_APP_MATCH;
//				}
//
//				if (ret != 0) {
//					sprintf(PosCom.stTrans.szRespCode, "%d", ret);
//				}
//
//				break;
//
//			case ESC: AppPlayTip("Card transaction cancelled"); break;
//
//			case CARD_TXN_TIMEOUT:  LogPrintInfo("Card Transaction Timeout"); break;
//		}
//
//		if (ret == CARD_TXN_NO_APP_MATCH) {
//			Beep_Api(1);
//
//			ScrClrLine_Api(6, 6);
//			ScrBrush_Api();
//			ScrDisp_Api(6, 0, "No App Match", CDISP);
//		}
//		else if (ret == 0) {
//			Delay_Api(2000);
//
//			Beep_Api(0);
//			AppPlayTip("Transaction Approved");
//
//			ScrClrLine_Api(6, 6);
//			ScrBrush_Api();
//
//			ScrDisp_Api(6, 0, "Approved", CDISP);
//			SecScrDisplay("Card Txn Approved");
//		}
//
//		if (event == ESC || event == CARD_TXN_TIMEOUT) {
//			Beep_Api(1);
//
//			ScrClrLine_Api(6, 6);
//			ScrBrush_Api();
//
//			char info[16] = "";
//			strcat(info, "TXN ");
//			strcat(info, event == ESC ? "Cancelled" : "Timeout");
//
//			ScrDisp_Api(6, 0, info, CDISP);
//		}
//		PiccStop();
//
//		break;
//	}
//}
//
//void PayWithCard(void)
//{
//	int ret;
//	long amt;
//	unsigned char key, tmp[128] = {0}, buf[24] = {0};
//
//	scrSetBackLightMode_Api(1, 60 * 1000);
//	KBFlush_Api();
//
//	while (1)
//	{
//		if (getActionMode() == CARD_ACTION_MODE)
//		{
//			LogPrintInfo("================== CARD MODE ==================");
//
//			ScrCls_Api();
//			ScrDisp_Api(0, 0, "Card Payment", CDISP);
//			ScrDisp_Api(5, 0, CURRENCY_AMT_INFO, CDISP);
//
//			ret = PiccInit();
//			if (ret == 0)
//			{
//				TransTapCard();
//			}
//			else
//			{
//				Beep_Api(1);
//				LogPrintWithRet(1, "!!! PiccInit() failed(%d) !!!", ret);
//
//				ScrDisp_Api(6, 0, "PICC Open Failed", CDISP);
//				AppPlayTip("Module open failed");
//			}
//
//			goMainMenu();
//		}
//
//		Delay_Api(1 * 1000);
//	}
//}
//
//void CardTxnThread(void)
//{
//	while (1)
//	{
//		PayWithCard();
//		Delay_Api(1000);
//	}
//}
