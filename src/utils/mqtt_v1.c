///*
// * mqtt.c
// *
// *  Created on: 2021��10��14��
// *      Author: vanstone
// */
//
//#include <string.h>
//
//#include <MQTTFreeRTOS.h>
//#include <MQTTClient.h>
//
//#include "def.h"
//
//#include <coredef.h>
//#include <struct.h>
//#include <poslib.h>
//
//#include <cJSON.h>
//#include "httpDownload.h"
//#include "EmvCommon.h"
//
//#define SERVER_IP "103.83.163.69"
//#define SERVER_PORT "4000"
//#define ACCESS_TOKEN "64d55563" //your_api_key
//#define TIMEOUT_MS 1000
//
//volatile int keepRunning = 1;
//#define RECONNECT_DELAY 5  // Reconnect delay in seconds
//volatile int isNetworkConnected = 0;
//
//time_t delete_file(time_t currentTime, time_t startTime) {
//    currentTime = time(NULL);
//
//    // Exit loop if 5 seconds have elapsed
//    long cal_time_second = (long) currentTime - (long) startTime;
//    // MAINLOG_L1("@@@@@@@@ Hello World cal_time_second %ld", cal_time_second);
//    if (cal_time_second >= 20) {
//        // MAINLOG_L1( "@@@@@@@@ Hello World 5 Second Delete %ld sfsddddddsdf %ld\n",currentTime, startTime);
//        startTime = currentTime;
//        fileFilter=0;
//        deleteAll();
//        return startTime;  // Break the loop after logging and return the updated startTime
//    } else {
//        // MAINLOG_L1("@@@@@@@@ Hello World 1 Second breal %ld\n", startTime);
//        return startTime; // Ensure we return startTime in all cases
//    }
//}
//
//// API Key
//
//void initMqttOs(void) {
//	MQTTOS os;
//
//	memset(&os, 0, sizeof(os));
//
//	os.timerSet = (unsigned long (*)()) TimerSet_Api;
//	MQTTClientOSInit(os);
//}
//
//static void onDefaultMessageArrived(MessageData* md)
//{
//	// Topic
//	if (md->topicName->lenstring.data == NULL || md->topicName->lenstring.len <= 0) {
//		LogPrintInfo("!!! DEFAULT MSG TOPIC ERROR !!!");
//		return;
//	}
//
//	char topic[strlen(md->topicName->lenstring.len) + 1];
//	memset(topic, 0, sizeof(topic));
//	memcpy(topic, md->topicName->lenstring.data, md->topicName->lenstring.len);
//
//	LogPrintNoRet("DEFAULT MSG TOPIC: ", topic);
//	memset(topic, 0, sizeof(topic));
//
//	// Data
//	if (md->message->payload == NULL || md->message->payloadlen <= 0) {
//		LogPrintInfo("!!! DEFAULT MSG ERROR !!!");
//		return;
//	}
//
//	char data[md->message->payloadlen + 1];
//	memset(data, 0, sizeof(data));
//	memcpy(data, md->message->payload, md->message->payloadlen);
//
//	LogPrintNoRet("DEFAULT MSG ARRIVED:\n", data);
//	memset(data, 0, sizeof(data));
//}
//
//
//int fileFilter;
//int needUpdate;
//int Device_Type;
//
//unsigned char updateAppName[32];
//
//#define TMS_FILE_DIR  "/ext/tms/"
//#define _DOWN_STATUS_ "/ext/tms/_tms_dinfo_"
//
//void deleteAll(void) {
//
//	int iRet = -1;
//
//	uint8 *rP = NULL;
//	folderFileDisplay((unsigned char*) TMS_FILE_DIR);
//}
//char* convertToUppercase(char *str) {
//	char *orig = str;
//	while (*str) {
//		*str = toupper((unsigned char) *str);
//		str++;
//	}
//	return orig;
//}
//
//char* capitalizeFirstLetter(char *str) {
//	if (str == NULL || *str == '\0') {
//		return str;  // Return if the string is NULL or empty
//	}
//
//	*str = toupper((unsigned char) *str);
//	return str;
//}
//
//char* processString(char *str) {
//	if (str == NULL || *str == '\0') {
//		return str;  // Return if the string is NULL or empty
//	}
//
//	// Check if the string is "USD" or "usd"
//	if (strcasecmp(str, "USD") == 0) {
//		char *orig = str;
//		while (*str) {
//			*str = toupper((unsigned char) *str);
//			str++;
//		}
//		return orig;
//	}
//
//	// Check if the string is "Riels" or "riels"
//	if (strcasecmp(str, "Riels") == 0) {
//		*str = toupper((unsigned char) *str);
//		return str;
//	}
//
//	return str;  // Return the original string if no match
//}
//unsigned char buf_h[12];
//unsigned char tmp[32];
//#define		MAX_LCDWIDTH				21
//char* limit_sound_by_amount() {
//	int ret = 0, amt;
//
//// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
////	GetScanf(MMI_POINT, 1, 12, 10000, 60, LINE4, LINE4, MAX_LCDWIDTH);
//
//// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//	memset(&PosCom, 0, sizeof(PosCom));
//
//	ScrCls_Api();
//	ScrDisp_Api(LINE1, 0, "Limit Sound", CDISP);
//	ScrDisp_Api(LINE3, 0, "input amount:", LDISP);
//	// MAINLOG_L1( "GetAmount @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ size:%s,%ld", PosCom.stTrans.TradeAmount, BcdToLong_Api(PosCom.stTrans.TradeAmount, 6));
//	if (GetAmount(PosCom.stTrans.TradeAmount) != 0) {
//		return 0;
//	}
//	if (ret == 0) //trade success
//			{
//		amt = BcdToLong_Api(PosCom.stTrans.TradeAmount, 6);
//		// MAINLOG_L1("GetAmount After enter : %ld,%ld", PosCom.stTrans.TradeAmount, amt);
//		memset(buf_h, 0, sizeof(buf_h));
//		memset(tmp, 0, sizeof(tmp));
//		sprintf(buf_h, "%d.%02d", amt / 100, amt % 100);
////		sprintf(tmp, "Limited below :%s", buf_h);
//
//		ScrCls_Api();
//		ScrDisp_Api(LINE1, 0, "Limit Sound", CDISP);
//		ScrDisp_Api(LINE5, 0, "Limited below :", CDISP);
//
//		char displayBuf[64]; // Ensure this buffer is large enough to hold the concatenated result
//
//		sprintf(displayBuf, "%s USD", buf_h);
//
//		ScrDisp_Api(LINE6, 0, displayBuf, CDISP);
//		WaitAnyKey_Api(2);
//	}
//	return buf_h;
//}
///*
// * Message comes in the following JSON format:
// * 1. transaction broadcasting		{"type":"TB", "msg":"Hello"}
// * 2. update notification			{"type":"UN"}
// * 3. synchronize time				{"type":"ST", "time":"20211016213200"}
// * 4. play MP3 file					{"type":"MP", "file":"2.mp3+1.mp3+3.mp3+2.mp3"}
// * 5. play amount					{"type":"TS", "amount":"12.33"}
// * Invalid messages are just ignored.
// *
// * Message should be less than 1024 bytes
// */
//time_t currentTime;
//time_t startTimeTemp;
//time_t startTime;
//
//static void onTopicMessageArrived(MessageData *md)
//{
//    /*
//     * Use a local buffer for the JSON payload.
//     * Check that the payload size does not exceed the buffer.
//     */
//    char buf[1024];
//    MQTTMessage *m = md->message;
//    if (m->payloadlen >= sizeof(buf))
//        return;  // Payload too large, bail out.
//
//    memcpy(buf, m->payload, m->payloadlen);
//    buf[m->payloadlen] = '\0';
//
//    /* Parse the JSON payload */
//    cJSON *root = cJSON_Parse(buf);
//    if (!root)
//        return;
//
//    /* Get common JSON fields */
//    cJSON *typeItem     = cJSON_GetObjectItem(root, "type");
//    cJSON *amountItem   = cJSON_GetObjectItem(root, "Amount");
//    cJSON *currencyItem = cJSON_GetObjectItem(root, "Currency");
//    cJSON *serialItem   = cJSON_GetObjectItem(root, "Serial");
//
//    /* --- Process "Serial" branch --- */
//    if (serialItem) {
//        secscrCls_lib();
//
//        /* Get current time and format it */
//        time_t currentTime = time(NULL);
//        char currentTimeStr[20];
//        snprintf(currentTimeStr, sizeof(currentTimeStr), "%ld", (long)currentTime);
//
//        if (!amountItem || !currencyItem) {
//            cJSON_Delete(root);
//            return;
//        }
//
//        char *amountStr   = amountItem->valuestring;
//        char *currencyStr = currencyItem->valuestring;
//        int amountVal     = atoi(amountStr);
//
//        if (amountVal < 0) {  // Negative amount: abnormal
//            PlayMP3File("abnormal.mp3");
//            cJSON_Delete(root);
//            return;
//        }
//
//        /* Prevent sound alert for large amounts */
//        int big_amount = atoi(buf_h);  // buf_h assumed to be defined globally
//        if (big_amount != 0 && amountVal >= big_amount) {
//            PlayMP3File("Success_01.mp3");
//            startTimeTemp = delete_file(currentTime, startTime);
//            startTime = startTimeTemp;
//            cJSON_Delete(root);
//            return;
//        }
//
//        int dB = 0;
//        int globalDB = atoi(global_dB);
//        if (globalDB < 0) {
//            dB = globalDB;
//        } else {
//            dB = (G_sys_param.sound_level == 1) ? -15 : 0;
//        }
//
//        /* Build filenames and paths */
//        char concatStr[100];
//        snprintf(concatStr, sizeof(concatStr), "%s-%s", G_sys_param.sn, currentTimeStr);
//
//        char saveLocation[100];
//        snprintf(saveLocation, sizeof(saveLocation), "/ext/tms/%s.mp3", concatStr);
//
//        char fileName[100];
//        snprintf(fileName, sizeof(fileName), "%s.mp3", concatStr);
//
//        /* Build the URL and download the MP3 file */
//        char *url_Download = make_http_request(SERVER_IP, SERVER_PORT,
//                                               amountStr, currencyStr,
//                                               G_sys_param.sn, currentTimeStr,
//                                               dB, ACCESS_TOKEN);
//
//        int ret = httpDownload(url_Download, METHOD_GET, saveLocation);
//        if (ret >= -10 && ret <= -4) {
//            ttsSetVolume_lib(G_sys_param.sound_level);
//            PlayMP3File("Success_01.mp3");
//            DelFile_Api(fileName);
//            cJSON_Delete(root);
//            return;
//        }
//        if (ret < 0) {
//            cJSON_Delete(root);
//            return;
//        }
//
//        ttsSetVolume_lib(G_sys_param.sound_level);
//        PlayMP3File(fileName);
//        small_screen(amountStr, currencyStr);
//
//        fileFilter = 0;
//        deleteAll();
//
//        cJSON_Delete(root);
//        return;
//    }
//
//    /* --- Process Non-Serial messages --- */
//    if (!typeItem) {
//        cJSON_Delete(root);
//        return;
//    }
//
//    if (strcmp(typeItem->valuestring, "TB") == 0) {
//        /* Transaction Broadcast */
//        cJSON *msgItem = cJSON_GetObjectItem(root, "msg");
//        if (msgItem)
//            AppPlayTip(msgItem->valuestring);
//
//    } else if (strcmp(typeItem->valuestring, "UN") == 0) {
//        /* Update Notification */
//        TmsTest();
//        ScrCls_Api();
//        char saveLocationKHR[256];
//        snprintf(saveLocationKHR, sizeof(saveLocationKHR), "/ext/tms/%s_KHR.jpg", G_sys_param.sn);
//        ScrDispImage_Api(saveLocationKHR, 0, 0);
//
//    } else if (strcmp(typeItem->valuestring, "DLE_QR") == 0) {
//        fileFilter = 2;
//        deleteAll();
//        ScrCls_Api();
//        ScrDisp_Api(LINE5, 0, "Image Deleted files", CDISP);
//        Delay_Api(3000);
//        ScrCls_Api();
//
//    } else if (strcmp(typeItem->valuestring, "DOWN_QR") == 0) {
//        check_exist_files();
//        ScrCls_Api();
//        ScrDisp_Api(LINE5, 0, "Image Downloaded", CDISP);
//        Delay_Api(2000);
//        ScrCls_Api();
//        char showKHR[256];
//        snprintf(showKHR, sizeof(showKHR), "/ext/tms/%s_KHR.jpg", G_sys_param.sn);
//        ScrDispImage_Api(showKHR, 0, 0);
//
//    } else if (strcmp(typeItem->valuestring, "DEL_MP3") == 0) {
//        fileFilter = 1;
//        deleteAll();
//        ScrCls_Api();
//        ScrDisp_Api(LINE5, 0, "MP3 Deleted files", CDISP);
//        Delay_Api(3000);
//        ScrCls_Api();
//
//    } else if (strcmp(typeItem->valuestring, "DOWN_MP3") == 0) {
//        download_mp3();
//
//    } else if (strcmp(typeItem->valuestring, "DEL_SYS_PARAM") == 0) {
//        fileFilter = 5;
//        deleteAll();
//        ScrCls_Api();
//        ScrDisp_Api(LINE5, 0, "DEL_SYS_PARAM", CDISP);
//        Delay_Api(3000);
//        ScrCls_Api();
//
//    } else if (strcmp(typeItem->valuestring, "ST") == 0) {
//        /* Synchronize Time */
//        cJSON *timeItem = cJSON_GetObjectItem(root, "time");
//        if (timeItem) {
//            struct DATE_USER date = {0};
//            struct TIME_USER timeVal = {0};
//            const char *timeStr = timeItem->valuestring;
//
//            date.year  = (short)AscToLong_Api(timeStr, 2);
//            date.mon   = (char)AscToLong_Api(timeStr + 2, 1);
//            date.day   = (char)AscToLong_Api(timeStr + 3, 1);
//            timeVal.hour = (char)AscToLong_Api(timeStr + 4, 1);
//            timeVal.min  = (char)AscToLong_Api(timeStr + 5, 1);
//            timeVal.sec  = (char)AscToLong_Api(timeStr + 6, 1);
//
//            SetTime_Api(&date, &timeVal);
//            unsigned char bcdtime[64] = {0};
//            GetSysTime_Api(bcdtime);
//            CommPrintHex("system time", bcdtime, 8);
//        }
//
//    } else if (strcmp(typeItem->valuestring, "MP") == 0) {
//        /* Play multiple MP3 files listed in a single string separated by '+' */
//        cJSON *fileItem = cJSON_GetObjectItem(root, "file");
//        if (fileItem) {
//            char *fileStr = fileItem->valuestring;
//            int num = 0, countMP3 = 0;
//            /* Process up to 10 file names */
//            while (fileStr[num] != '\0' && countMP3 < 10) {
//                char *plusPos = strchr(fileStr + num, '+');
//                int len = plusPos ? (plusPos - (fileStr + num)) : strlen(fileStr + num);
//                char mp3File[10] = {0};
//                if (len > (int)sizeof(mp3File) - 1)
//                    len = sizeof(mp3File) - 1;
//                memcpy(mp3File, fileStr + num, len);
//                mp3File[len] = '\0';
//                PlayMP3File(mp3File);
//                countMP3++;
//                if (!plusPos)
//                    break;
//                num += len + 1;
//            }
//        }
//
//    } else if (strcmp(typeItem->valuestring, "TS") == 0) {
//        /* Transaction Summary */
//        cJSON *amountTS = cJSON_GetObjectItem(root, "amount");
//        if (amountTS) {
//            char tipMsg[64];
//            snprintf(tipMsg, sizeof(tipMsg), "%s dollars received", amountTS->valuestring);
//            AppPlayTip(tipMsg);
//
//            int amount = 0;
//            char asc[12] = {0};
//            char *dotPos = strchr(amountTS->valuestring, '.');
//            if (dotPos) {
//                int integerLen = dotPos - amountTS->valuestring;
//                if (integerLen > 0 && integerLen < (int)sizeof(asc)) {
//                    memcpy(asc, amountTS->valuestring, integerLen);
//                    asc[integerLen] = '\0';
//                    /* Append up to two digits after the decimal point */
//                    strncat(asc, dotPos + 1, 2);
//                    amount = atoi(asc);
//                }
//            } else {
//                amount = atoi(amountTS->valuestring) * 100;
//            }
//
//            if (Device_Type >= 2) {
//                /* Additional processing for Device_Type >= 2 can be added here */
//            }
//        }
//    }
//
//    cJSON_Delete(root);
//}
//
//static unsigned char pSendBuf[1024];
//static unsigned char pReadBuf[1024];
//
//void mQTTMainThread(void) {
//    int ret, err;
//    Network n;
//    MQTTClient c;
//    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
//    int connection_attempts = 0;
//    int max_retry_delay = 5000; // Maximum retry delay in ms
//    int current_retry_delay = 500; // Start with a shorter delay
//
//MQTT_LOAD_BLOCK:
//    ret = 0;
//    err = 0;
//    connection_attempts = 0;
//    current_retry_delay = 500;
//
//    memset(&c, 0, sizeof(MQTTClient));
//
//    while (1) {
//        // Check internet with shorter retry interval when attempting to connect
//        if (!checkInternetStatus()) {
//            MAINLOG_L1("No internet connection. Retrying in %d ms...", current_retry_delay);
//            Delay_Api(current_retry_delay);
//
//            // Progressive backoff - increase delay up to max_retry_delay
//            if (++connection_attempts > 3) {
//                current_retry_delay = (current_retry_delay * 2 < max_retry_delay) ?
//                                     current_retry_delay * 2 : max_retry_delay;
//            }
//            continue;
//        }
//
//        // Reset connection attempts counter when we have internet
//        connection_attempts = 0;
//        current_retry_delay = 500;
//
//        // Initialize MQTT
//        MQTTClientInit(&c, &n, 20000, pSendBuf, 1024, pReadBuf, 1024);
//        c.defaultMessageHandler = onTopicMessageArrived;
//
//        // Set network functions
//        n.mqttconnect = net_connect;
//        n.mqttclose = net_close;
//        n.mqttread = net_read;
//        n.mqttwrite = net_write;
//
//        // Connect to MQTT Broker
//        MAINLOG_L1("Connecting to MQTT: %s:%s", G_sys_param.mqtt_server, G_sys_param.mqtt_port);
//
//        n.netContext = n.mqttconnect(NULL, G_sys_param.mqtt_server, G_sys_param.mqtt_port,
//                                    10000, G_sys_param.mqtt_ssl, &err);
//
//        if (n.netContext == NULL) {
//            MAINLOG_L1("MQTT Connection failed (err=%d)! Retrying...", err);
//            Delay_Api(current_retry_delay);
//            current_retry_delay = (current_retry_delay * 2 < max_retry_delay) ?
//                                 current_retry_delay * 2 : max_retry_delay;
//            continue; // Continue to next loop iteration instead of goto
//        }
//
//        // Configure MQTT connection
//        data.MQTTVersion = 4;
//        data.clientID.cstring = G_sys_param.mqtt_client_id;
//        data.keepAliveInterval = G_sys_param.mqtt_keepalive;
//        data.cleansession = 1;
//
//        // Connect to MQTT broker
//        ret = MQTTConnect(&c, &data);
//        if (ret != 0) {
//            MAINLOG_L1("MQTTConnect failed: %d", ret);
//            n.mqttclose(n.netContext);
//            Delay_Api(current_retry_delay);
//            current_retry_delay = (current_retry_delay * 2 < max_retry_delay) ?
//                                 current_retry_delay * 2 : max_retry_delay;
//            continue;
//        }
//
//        // Subscribe to Topic
//        ret = MQTTSubscribe(&c, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, onTopicMessageArrived);
//        if (ret != 0) {
//            MAINLOG_L1("MQTT Subscription failed: %d", ret);
//            MQTTDisconnect(&c);
//            n.mqttclose(n.netContext);
//            Delay_Api(current_retry_delay);
//            continue;
//        }
//
//        MAINLOG_L1("MQTT Connected and Subscribed!");
//        isNetworkConnected = 1;
//        PlayMP3File("IoT_Sucess_connect.mp3");
//
//        // Keep MQTT alive - more frequent yield with shorter delay
//        while (checkInternetStatus()) {
//            ret = MQTTYield(&c, 2000); // Shorter yield time
//            Delay_Api(2000); // Shorter delay between yields
//
//            if (ret < 0) {
//                MAINLOG_L1("MQTT Yield failed (%d), resetting connection...", ret);
//                break; // Break out of inner loop to reconnect
//            }
//        }
//
//        // Disconnect when network is lost
//        MAINLOG_L1("Internet lost or MQTT error! Disconnecting...");
//        MQTTDisconnect(&c);
//        n.mqttclose(n.netContext);
//        isNetworkConnected = 0;
//        Delay_Api(200); // Short delay before reconnecting
//    }
//
//    MAINLOG_L1("MQTT Thread Exiting...");
//    fibo_thread_delete(); // Exit thread properly
//}
