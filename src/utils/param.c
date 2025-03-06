/*
 * param.c
 *
 *  Created on: 2021��10��14��
 *      Author: vanstone
 */
#include <string.h>

#include "def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

unsigned char tms_ip[32];
unsigned char tms_domain[32];
unsigned char tms_port[32];

unsigned char WIFI_ID[32];
unsigned char WIFI_PD[32];

char _TMS_HOST_IP_[64] = "";
char _TMS_HOST_DOMAIN_[128] = "";
char _TMS_HOST_PORT_[64] = "";

SYS_PARAM G_sys_param;
int Device_Type;

static void readSN(char *sn) {
	int ret;
	unsigned char buf[40];

	ret = PEDReadPinPadSn_Api(buf);
	if (ret != 0) {
		// Serial number lost, warning and poweroff.
		AppPlayTip("Serial number lost, power off now");

		WaitAnyKey_Api(10);
		SysPowerOff_Api();

		while (1)
			;
	}

	ret = AscToLong_Api(buf, 2);
	buf[ret + 2] = 0;

	strcpy(sn, (char*) (buf + 2));
}

void createDefaultSysParam()
{
//	LogPrintInfo("===== CREATE DEFAULT SYS PARAM =====");

	G_sys_param.mqtt_ssl = 0;

	readSN(G_sys_param.sn);
//	LogPrintNoRet("SN: ", G_sys_param.sn);

	sprintf(G_sys_param.mqtt_topic,     "%s", "103.83.163.69");
	// sprintf(G_sys_param.mqtt_topic,     "%s", "103.25.92.104");
	sprintf(G_sys_param.mqtt_client_id, "%s",      "1883");

	G_sys_param.mqtt_qos 	   = 0;
	G_sys_param.mqtt_keepalive = 60;
	G_sys_param.sound_level    = 3;

	saveParam();
//	printSysParam();
}

void initLib(void){
	CheckTmsStatus();

	// Initialize card readers
	Common_Init_Api();
	PayPass_Init_Api();
	PayWave_Init_Api();
	PayPassAddAppExp(0);
	PayWaveAddAppExp();
}

void initParam(void) {
//	int ret, len = 0;
//
//		len = GetFileSize_Api(SYS_PARAM_NAME);
////		LogPrintWithRet(0, "GetFileSize_Api('sys_param.dat'): ", len);
//
//		memset(&G_sys_param, 0, sizeof(SYS_PARAM));
//
//		if (len != 0)
//		//if (0)
//		{
//			ret = ReadFile_Api(SYS_PARAM_NAME, (unsigned char *)&G_sys_param, 0, &len);
////			LogPrintWithRet(0, "ReadFile_Api('sys_param.dat'): ", ret);
//
//			if (ret == 0)
//			{
////				LogPrintInfo("!!! READ SYS PARAM FROM FILE !!!");
//
//				// ===== NOTE: TEST
//				/*
//				sprintf(G_sys_param.mqtt_server, "%s", "broker.hivemq.com");
//				sprintf(G_sys_param.mqtt_port,   "%s", "1883");
//				G_sys_param.mqtt_ssl = 0;
//				*/
//				// MAINLOG_L1("hort_param %d", G_sys_param.sound_level);
//
//				sprintf(G_sys_param.mqtt_server, "%s", "103.83.163.69");
//				sprintf(G_sys_param.mqtt_port,   "%s", "1883");
//				G_sys_param.mqtt_ssl = 0;
//				G_sys_param.mqtt_qos 	   = 0;
//				G_sys_param.mqtt_keepalive = 60;
//
//				saveParam();
//
//				// ===== NOTE: TEST
//
////				printSysParam();
//			}
//			else {
////				LogPrintWithRet(1, "!!! Read File('sys_param.dat') Failed(%d) !!!", ret);
//				createDefaultSysParam();
//			}
//		}
//		else {
//			createDefaultSysParam();
//		}
	int ret;
	unsigned int len;

	ret = GetFileSize_Api(SYS_PARAM_NAME);

	if (ret == sizeof(SYS_PARAM)) {
		len = ret;
		ret = ReadFile_Api(SYS_PARAM_NAME, (unsigned char*) &G_sys_param, 0,
				&len);
		if ((ret == 0) && (len == sizeof(SYS_PARAM))) {
			readSN(G_sys_param.sn);

			return;
		}
	}


	// Create default param
	memset(&G_sys_param, 0, sizeof(SYS_PARAM));

	readSN(G_sys_param.sn);
	// MAINLOG_L1("tms 11111111111111111 %d", G_sys_param.sound_level);
	// MAINLOG_L1("tms 22222222222222222 %s", G_sys_param.sound_level);
	strcpy(G_sys_param.mqtt_server, "103.83.163.69"); //3.82.39.163  //103.83.163.69    //103.25.92.104
	strcpy(G_sys_param.mqtt_port, "1883");
	G_sys_param.mqtt_ssl = 0;
	G_sys_param.mqtt_qos = 0;
	G_sys_param.mqtt_keepalive = 60;
	strcpy(G_sys_param.mqtt_topic, G_sys_param.sn);
	strcpy(G_sys_param.mqtt_client_id, G_sys_param.sn); //mqttx_78978d08

	//	strcpy(G_sys_param.mqtt_server, "broker.hivemq.com");
	//	strcpy(G_sys_param.mqtt_port, "1883");
	//	G_sys_param.mqtt_ssl = 0;
//		sprintf(G_sys_param.mqtt_topic, "topic-%s", G_sys_param.sn);
//		sprintf(G_sys_param.mqtt_client_id, "clientId-%s", G_sys_param.sn);

	// MAINLOG_L1("G_sys_param.mqtt_topic:%s", G_sys_param.mqtt_topic);
	// MAINLOG_L1("G_sys_param.mqtt_client_id:%s", G_sys_param.mqtt_client_id);
	G_sys_param.mqtt_qos = 0;
	G_sys_param.mqtt_keepalive = 60;

	//G_sys_param.sound_level = 1;

	saveParam();
}

void saveParam(void) {
	int ret = SaveWholeFile_Api(SYS_PARAM_NAME, (unsigned char*) &G_sys_param,
			sizeof(SYS_PARAM));
	// MAINLOG_L1("tms saveParam %d", ret);

}

void initDeviceType() {
	char machineType[26];
	int Result = 0, ret;

	memset(machineType, 0, sizeof(machineType));
	Result = sysGetTermType_lib(&machineType);
	if (Result >= 0) {
		// MAINLOG_L1("device type : %s", machineType);

		ret = memcmp(machineType, "Q180D", 5);
		if (ret == 0) {
			// MAINLOG_L1("define Q180D");
			Device_Type = 2;
		}

		ret = memcmp(machineType, "Q190", 4);
		if (ret == 0) {
			// MAINLOG_L1("define Q190");
			Device_Type = 3;
//			QRCodeDisp();
		}
	}

}

int checkingWifiActivate(){
	int ret, len = 64;
	char buf[2] = {0};
	int file_size = GetFileSize_Api(WIFI_CONFIG_FILE);
	ret = ReadFile_Api(WIFI_CONFIG_FILE, buf, 0, &file_size);
	if (ret != 0)
	{
		MAINLOG_L1("!!! ReadFile_Api('wifi_config.dat') failed(%d) !!!", ret);
		return ret;
	}

	return 0;
}

int readWIFIParam()
{
	int ret, len = 64;
	char buf[64] = "";

	unsigned char *ptr = NULL;

	ret = ReadFile_Api(WIFI_PARAM_FILE, buf, 0, &len);
	if (ret != 0) {
		MAINLOG_L1("!!! Read WIFI-PARAM Failed(%d) !!!", ret);
		return ret;
	}

	ptr = strstr(buf, ",");
	if (ptr == NULL) {
		MAINLOG_L1("!!! WIFI-PARAM ERROR !!!");
		return -1;
	}

	memcpy(WIFI_ID, buf,     strlen(buf) - strlen(ptr));
	memcpy(WIFI_PD, ptr + 1, strlen(ptr + 1));

//	MAINLOG_L1("===== WIFI_PARAM =====");
//	MAINLOG_L1(WIFI_ID);
//	MAINLOG_L1(WIFI_PD);
//	MAINLOG_L1("======================");


	char *ptr1 = NULL, *ptr2 = NULL;

	ret = ReadFile_Api("/ext/tms/param_api.dat", buf, 0, &len);
	if (ret != 0) {
		// MAINLOG_L1("!!! Read PARAM Failed(%d) !!!", ret);
		return ret;
	}

	// Find the first comma
	ptr1 = strstr(buf, ",");
	if (ptr1 == NULL) {
		// MAINLOG_L1("PARAM ERROR: Missing first comma!");
		return -1;
	}

	// Find the second comma
	ptr2 = strstr(ptr1 + 1, ",");
	if (ptr2 == NULL) {
		// MAINLOG_L1("PARAM ERROR: Missing second comma!");
		return -1;
	}

	// Extract IP
	memcpy(tms_ip, buf, ptr1 - buf);
	tms_ip[ptr1 - buf] = '\0'; // Null-terminate the string

	// Extract Domain
	memcpy(tms_domain, ptr1 + 1, ptr2 - (ptr1 + 1));
	tms_domain[ptr2 - (ptr1 + 1)] = '\0'; // Null-terminate the string

	// Extract Port
	strcpy(tms_port, ptr2 + 1); // The rest is the port

	strcpy(_TMS_HOST_IP_, tms_ip);
	strcpy(_TMS_HOST_DOMAIN_, tms_domain);
	strcpy(_TMS_HOST_PORT_, tms_port);

	return 0;
}

//int readWIFIParam() {
//	int ret, len = 64;
//	char buf[64] = "";
//	char *ptr1 = NULL, *ptr2 = NULL;
//
//	ret = ReadFile_Api("/ext/tms/param_api.dat", buf, 0, &len);
//	if (ret != 0) {
//		// MAINLOG_L1("!!! Read PARAM Failed(%d) !!!", ret);
//		return ret;
//	}
//
//	// Find the first comma
//	ptr1 = strstr(buf, ",");
//	if (ptr1 == NULL) {
//		// MAINLOG_L1("PARAM ERROR: Missing first comma!");
//		return -1;
//	}
//
//	// Find the second comma
//	ptr2 = strstr(ptr1 + 1, ",");
//	if (ptr2 == NULL) {
//		// MAINLOG_L1("PARAM ERROR: Missing second comma!");
//		return -1;
//	}
//
//	// Extract IP
//	memcpy(tms_ip, buf, ptr1 - buf);
//	tms_ip[ptr1 - buf] = '\0'; // Null-terminate the string
//
//	// Extract Domain
//	memcpy(tms_domain, ptr1 + 1, ptr2 - (ptr1 + 1));
//	tms_domain[ptr2 - (ptr1 + 1)] = '\0'; // Null-terminate the string
//
//	// Extract Port
//	strcpy(tms_port, ptr2 + 1); // The rest is the port
//
//	// MAINLOG_L1("PARAM IP: %s", tms_ip);
//	// MAINLOG_L1("PARAM DOMAIN: %s", tms_domain);
//	// MAINLOG_L1("PARAM PORT: %s", tms_port);
//
//	strcpy(_TMS_HOST_IP_, tms_ip);
//	strcpy(_TMS_HOST_DOMAIN_, tms_domain);
//	strcpy(_TMS_HOST_PORT_, tms_port);
//
//	// MAINLOG_L1("PARAM _TMS_HOST_IP_: %s", _TMS_HOST_IP_);
//	// MAINLOG_L1("PARAM _TMS_HOST_DOMAIN_: %s", _TMS_HOST_DOMAIN_);
//	// MAINLOG_L1("PARAM _TMS_HOST_PORT_: %s", _TMS_HOST_PORT_);
//
//	return 0;
//}

//int readWIFIParam()
//{
//	int ret, len = 64;
//	char buf[64] = "";
//
//	unsigned char *ptr = NULL;
//
//
//	ret = ReadFile_Api("/ext/tms/param_api.dat", buf, 0, &len);
//	if (ret != 0) {
//		// MAINLOG_L1("!!! Read PARAM Failed(%d) !!!",ret);
//		return ret;
//	}
//
//	ptr = strstr(buf, ",");
//	if (ptr == NULL) {
//		// MAINLOG_L1("PARAM ERROR !!!");
//
//		return -1;
//	}
//
//	memcpy(tms_ip, buf,     strlen(buf) - strlen(ptr));
//	memcpy(tms_domain, ptr + 1, strlen(ptr + 1));
//
//
//	// MAINLOG_L1("PARAM IP %s",tms_ip);
//	// MAINLOG_L1("PARAM DOMAIN %s",tms_domain);
//
//	strcpy(_TMS_HOST_IP_, tms_ip);
//	strcpy(_TMS_HOST_DOMAIN_, tms_domain);
//
//
//	// MAINLOG_L1("PARAM _TMS_HOST_IP_ %s",_TMS_HOST_IP_);
//	// MAINLOG_L1("PARAM _TMS_HOST_DOMAIN_ %s",_TMS_HOST_DOMAIN_);
//	// MAINLOG_L1("PARAM _TMS_HOST_PORT_ %s",_TMS_HOST_PORT_);
//	return 0;
//}
