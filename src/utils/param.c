/*
 * param.c
 *
 *  Created on: 2021��10��14��
 *      Author: vanstone
 */
#include "EnvAcleda.h"
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
    int ret;
    unsigned int len;
    int initialize_defaults = 1; // Flag to track if we need to initialize defaults

    ret = GetFileSize_Api(SYS_PARAM_NAME);

    if (ret == sizeof(SYS_PARAM)) {
        len = ret;
        ret = ReadFile_Api(SYS_PARAM_NAME, (unsigned char*) &G_sys_param, 0, &len);
        if ((ret == 0) && (len == sizeof(SYS_PARAM))) {
            readSN(G_sys_param.sn);

            // Add validation to ensure the file contains valid data
            if (strlen(G_sys_param.mqtt_server) > 0 &&
                strlen(G_sys_param.mqtt_port) > 0) {
                // strcpy(G_sys_param.mqtt_server, ENV_IOT_IP);
				strcpy(G_sys_param.mqtt_server, ENV_DEV_IOT_IP);
                strcpy(G_sys_param.mqtt_port, ENV_IOT_PORT);
                G_sys_param.mqtt_ssl = 0;
                G_sys_param.mqtt_qos = 0;
                G_sys_param.mqtt_keepalive = 60;
                strcpy(G_sys_param.mqtt_topic, G_sys_param.sn);
                strcpy(G_sys_param.mqtt_client_id, G_sys_param.sn);
                saveParam();
                initialize_defaults = 0; // Don't initialize defaults
            }
            // Could add more validation as needed
        }
    }

    // Only initialize defaults if needed
    if (initialize_defaults) {
        // Create default param
        memset(&G_sys_param, 0, sizeof(SYS_PARAM));

        readSN(G_sys_param.sn);
        // strcpy(G_sys_param.mqtt_server, ENV_IOT_IP);
		strcpy(G_sys_param.mqtt_server, ENV_DEV_IOT_IP);
        strcpy(G_sys_param.mqtt_port, ENV_IOT_PORT);
        G_sys_param.mqtt_ssl = 0;
        G_sys_param.mqtt_qos = 0;
        G_sys_param.mqtt_keepalive = 60;
        strcpy(G_sys_param.mqtt_topic, G_sys_param.sn);
        strcpy(G_sys_param.mqtt_client_id, G_sys_param.sn);

        // Add a MAINLOG_L1 statement to confirm defaults are being set
        // MAINLOG_L1("Setting default MQTT parameters");

        saveParam();
    }
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

int readTMSParam()
{
	int ret, len = 64;
	char buf[64] = "";
	char configFilePath[128] = {0};  // Buffer for the full file path



	char *ptr1 = NULL, *ptr2 = NULL;

	// Build the config file path using constants from EnvAcleda.h
	snprintf(configFilePath, sizeof(configFilePath), "%s%s", CONFIG_SAVE_PATH, CONFIG_FILE_NAME);

	ret = ReadFile_Api(configFilePath, buf, 0, &len);
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
	MAINLOG_L1("tms PORTS%s%s%s", _TMS_HOST_IP_,_TMS_HOST_DOMAIN_,_TMS_HOST_PORT_);

	return 0;
}
