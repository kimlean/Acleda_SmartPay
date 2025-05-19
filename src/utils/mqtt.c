/*
 * mqtt.c
 *
 *  Created on: 2021-10-14
 *  SY KIMLEAN: Modified on: 2025-03-24
 */

#include <string.h>
#include <time.h>

#include <MQTTFreeRTOS.h>
#include <MQTTClient.h>

#include "def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#include <cJSON.h>
#include "httpDownload.h"
#include "EmvCommon.h"

// EMQX broker settings
//#define MQTT_BROKER "172.17.199.46"
//#define MQTT_PORT "1883"           // Use "8883" for TLS
//#define MQTT_USE_SSL 0             // Set to 1 or 2 for SSL/TLS
//#define MQTT_USERNAME ""           // Set if your EMQX requires authentication
//#define MQTT_PASSWORD ""           // Set if your EMQX requires authentication

// TTS Server settings
#define SERVER_IP "103.83.163.69"
#define SERVER_PORT "4000"
#define ACCESS_TOKEN "64d55563"
#define TIMEOUT_MS 1000

// TTS SERVER UAT SETTING
#define SERVER_UAT_IP "103.25.92.104"
#define SERVER_UAT_PORT "2000"

// MQTT Session settings
#define HEARTBEAT_INTERVAL 2      // Seconds between heartbeats
#define DELETE_FILE        60
#define MQTT_KEEPALIVE 60          // MQTT keepalive in seconds
#define MAX_RECONNECT_DELAY 5000   // Maximum reconnect delay in ms
#define INITIAL_RECONNECT_DELAY 500 // Initial reconnect delay in ms

// Runtime flags
volatile int mqtt_reconnect_flag = 0;  // Signal to force MQTT reconnection
volatile int isNetworkConnected = 0;   // Indicates if MQTT is connected

// File management
int fileFilter;
int needUpdate;
int Device_Type;
unsigned char updateAppName[32];

// Buffers for MQTT client
static unsigned char pSendBuf[1024];
static unsigned char pReadBuf[1024];

// Time tracking
time_t currentTime;
time_t startTimeTemp;
time_t startTime;

// File paths
#define TMS_FILE_DIR  "/ext/tms/"
#define _DOWN_STATUS_ "/ext/tms/_tms_dinfo_"

// Sound/display parameters
unsigned char buf_h[12];
unsigned char tmp[32];
#define MAX_LCDWIDTH 21

void initMqttOs(void) {
    MQTTOS os;
    memset(&os, 0, sizeof(os));
    os.timerSet = (unsigned long (*)()) TimerSet_Api;
    MQTTClientOSInit(os);
}

time_t delete_file(time_t currentTime, time_t startTime) {
    currentTime = time(NULL);

    // Exit loop if 20 seconds have elapsed
    long cal_time_second = (long) currentTime - (long) startTime;
    if (cal_time_second >= 20) {
        startTime = currentTime;
        fileFilter = 0;
        deleteAll();
        return startTime;
    } else {
        return startTime;
    }
}

void deleteAll(void) {
	MAINLOG_L1("Process delete file");
    uint8 *rP = NULL;
    folderFileDisplay((unsigned char*) TMS_FILE_DIR);
}

static void onDefaultMessageArrived(MessageData* md) {
    // Topic
    if (md->topicName->lenstring.data == NULL || md->topicName->lenstring.len <= 0) {
        LogPrintInfo("!!! DEFAULT MSG TOPIC ERROR !!!");
        return;
    }

    char topic[md->topicName->lenstring.len + 1];
    memset(topic, 0, sizeof(topic));
    memcpy(topic, md->topicName->lenstring.data, md->topicName->lenstring.len);
    LogPrintNoRet("DEFAULT MSG TOPIC: ", topic);

    // Data
    if (md->message->payload == NULL || md->message->payloadlen <= 0) {
        LogPrintInfo("!!! DEFAULT MSG ERROR !!!");
        return;
    }

    char data[md->message->payloadlen + 1];
    memset(data, 0, sizeof(data));
    memcpy(data, md->message->payload, md->message->payloadlen);
    LogPrintNoRet("DEFAULT MSG ARRIVED:\n", data);
}

char* convertToUppercase(char *str) {
    char *orig = str;
    while (*str) {
        *str = toupper((unsigned char) *str);
        str++;
    }
    return orig;
}

char* capitalizeFirstLetter(char *str) {
    if (str == NULL || *str == '\0') {
        return str;
    }
    *str = toupper((unsigned char) *str);
    return str;
}

char* processString(char *str) {
    if (str == NULL || *str == '\0') {
        return str;
    }

    // Handle USD case
    if (strcasecmp(str, "USD") == 0) {
        return convertToUppercase(str);
    }

    // Handle Riels case
    if (strcasecmp(str, "Riels") == 0) {
        return capitalizeFirstLetter(str);
    }

    return str;
}

char* limit_sound_by_amount() {
    int ret = 0, amt;
    memset(&PosCom, 0, sizeof(PosCom));

    ScrCls_Api();
    ScrDisp_Api(LINE1, 0, "Limit Sound", CDISP);
    ScrDisp_Api(LINE3, 0, "input amount:", LDISP);

    if (GetAmount(PosCom.stTrans.TradeAmount) != 0) {
        return 0;
    }

    if (ret == 0) {
        amt = BcdToLong_Api(PosCom.stTrans.TradeAmount, 6);
        memset(buf_h, 0, sizeof(buf_h));
        memset(tmp, 0, sizeof(tmp));
        sprintf(buf_h, "%d.%02d", amt / 100, amt % 100);

        ScrCls_Api();
        ScrDisp_Api(LINE1, 0, "Limit Sound", CDISP);
        ScrDisp_Api(LINE5, 0, "Limited below :", CDISP);

        char displayBuf[64];
        sprintf(displayBuf, "%s USD", buf_h);
        ScrDisp_Api(LINE6, 0, displayBuf, CDISP);
        WaitAnyKey_Api(2);
    }
    return buf_h;
}

static void onCommandMessageArrived(MessageData *md) {
    char cmdBuf[1024];
    int cmdLen = md->message->payloadlen < sizeof(cmdBuf) - 1 ?
                 md->message->payloadlen : sizeof(cmdBuf) - 1;

    memcpy(cmdBuf, md->message->payload, cmdLen);
    cmdBuf[cmdLen] = '\0';

    MAINLOG_L1("Command received: %s", cmdBuf);

    // Parse command JSON
    cJSON *root = cJSON_Parse(cmdBuf);
    if (!root) {
        MAINLOG_L1("Invalid command format");
        return;
    }

    // Handle command types
    cJSON *cmdType = cJSON_GetObjectItem(root, "command");
    if (cmdType) {
        if (strcmp(cmdType->valuestring, "restart") == 0) {
            // Send acknowledgment and restart
            MAINLOG_L1("Restart command received");
            Delay_Api(1000);
            SysPowerReBoot_Api();
        }
        else if (strcmp(cmdType->valuestring, "status") == 0) {
            // Request status update
            MAINLOG_L1("Status request received");
        }
        // Add more command handlers as needed
    }

    cJSON_Delete(root);
}

static void onTopicMessageArrived(MessageData *md) {
    char buf[1024];
    MQTTMessage *m = md->message;

    // Check payload size
    if (m->payloadlen >= sizeof(buf)) {
        MAINLOG_L1("Message too large (%d bytes)", m->payloadlen);
        return;
    }

    // Copy message to buffer
    memcpy(buf, m->payload, m->payloadlen);
    buf[m->payloadlen] = '\0';

    // Parse the JSON message
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        MAINLOG_L1("Invalid JSON format");
        return;
    }

    // Get common fields
    cJSON *typeItem = cJSON_GetObjectItem(root, "type");
    cJSON *amountItem = cJSON_GetObjectItem(root, "Amount");
    cJSON *currencyItem = cJSON_GetObjectItem(root, "Currency");
    cJSON *serialItem = cJSON_GetObjectItem(root, "Serial");

    /* --- Process "Serial" branch --- */
    if (serialItem) {
        secscrCls_lib();

        // Get current time and format it
        time_t currentTime = time(NULL);
        char currentTimeStr[20];
        sprintf(currentTimeStr, "%ld", (long)currentTime);

        MAINLOG_L1("Serial command received at %s", currentTimeStr);

        if (!amountItem || !currencyItem) {
            MAINLOG_L1("Missing Amount or Currency fields");
            cJSON_Delete(root);
            return;
        }

        char *amountStr = amountItem->valuestring;
        char *currencyStr = currencyItem->valuestring;
        int amountVal = atoi(amountStr);

        // Check for negative amount
        if (amountVal < 0) {
            MAINLOG_L1("Negative amount detected: %d", amountVal);
            PlayMP3File("abnormal.mp3");
            cJSON_Delete(root);
            return;
        }

        // Check for large amount limit
        int big_amount = atoi(buf_h);
        MAINLOG_L1("Amount: %d, Limit: %d", amountVal, big_amount);
        if (big_amount != 0 && amountVal >= big_amount) {
            MAINLOG_L1("Amount exceeds limit, playing success tone");
            PlayMP3File("Success_01.mp3");
            startTimeTemp = delete_file(currentTime, startTime);
            startTime = startTimeTemp;
            cJSON_Delete(root);
            return;
        }

        // Set decibel level based on sound settings
        int dB = 0;
        int globalDB = atoi(global_dB);
        if (globalDB < 0) {
            dB = globalDB;
            MAINLOG_L1("Using global dB: %d", dB);
        } else {
            dB = (G_sys_param.sound_level == 1) ? -15 : 0;
            MAINLOG_L1("Using sound level: %d, dB: %d", G_sys_param.sound_level, dB);
        }

        // Build filenames and paths
        char concatStr[100];
        snprintf(concatStr, sizeof(concatStr), "%s-%s", G_sys_param.sn, currentTimeStr);

        char saveLocation[100];
        snprintf(saveLocation, sizeof(saveLocation), "/ext/tms/%s.mp3", concatStr);

        char fileName[100];
        snprintf(fileName, sizeof(fileName), "%s.mp3", concatStr);

        // Generate download URL and fetch MP3
//        char *url_Download = make_http_request(SERVER_IP, SERVER_PORT,
//                                               amountStr, currencyStr,
//                                               G_sys_param.sn, currentTimeStr,
//                                               dB, ACCESS_TOKEN);

        char *url_Download = make_http_request(SERVER_UAT_IP, SERVER_UAT_PORT,
                                                       amountStr, currencyStr,
                                                       G_sys_param.sn, currentTimeStr,
                                                       dB, ACCESS_TOKEN);

        // Add null check for URL before trying to download
		if (url_Download == NULL) {
			MAINLOG_L1("ERROR: Failed to generate download URL");
			ttsSetVolume_lib(G_sys_param.sound_level);
			PlayMP3File("Success_01.mp3");
			cJSON_Delete(root);
			return;
		}

        MAINLOG_L1("Downloading MP3 from: %s", url_Download);
        int ret = httpDownload(url_Download, METHOD_GET, saveLocation);

        // Handle download errors
        if (ret >= -10 && ret <= -4) {
            MAINLOG_L1("Server TTS error: %d", ret);
            ttsSetVolume_lib(G_sys_param.sound_level);
            PlayMP3File("Success_01.mp3");
            DelFile_Api(fileName);
            cJSON_Delete(root);
            return;
        }

        if (ret < 0) {
            MAINLOG_L1("Download failed: %d", ret);
            cJSON_Delete(root);
            return;
        }

        // Play the downloaded MP3
        MAINLOG_L1("Playing MP3 with volume level: %d", G_sys_param.sound_level);
        ttsSetVolume_lib(G_sys_param.sound_level);
        PlayMP3File(fileName);
        small_screen(amountStr, currencyStr);

        // CLOSE CODE : SLOW PERFORMENT DELETE FILE BY FILTER CODE
        // fileFilter = 0;
        // deleteAll();

        cJSON_Delete(root);
        return;
    }

    /* --- Process other message types --- */
    if (!typeItem) {
        MAINLOG_L1("Missing type field");
        cJSON_Delete(root);
        return;
    }

    const char *type = typeItem->valuestring;
    MAINLOG_L1("Processing message type: %s", type);

    if (strcmp(type, "TB") == 0) {
        // Transaction Broadcast
        cJSON *msgItem = cJSON_GetObjectItem(root, "msg");
        if (msgItem) {
            MAINLOG_L1("Broadcasting: %s", msgItem->valuestring);
            AppPlayTip(msgItem->valuestring);
        }
    }
    else if (strcmp(type, "UN") == 0) {
        // Update Notification
        MAINLOG_L1("Update notification received");
        TMSConnection();
        ScrCls_Api();
        char saveLocationKHR[256];
        snprintf(saveLocationKHR, sizeof(saveLocationKHR), "/ext/tms/%s_KHR.jpg", G_sys_param.sn);
        ScrDispImage_Api(saveLocationKHR, 0, 0);
    }
    else if (strcmp(type, "DEL_QR") == 0) {
        // Delete QR
        MAINLOG_L1("Delete QR command received");
        fileFilter = 2;
        deleteAll();
        ScrCls_Api();
        ScrDisp_Api(LINE5, 0, "Deleted Image files", CDISP);
        initQRImage();
        Delay_Api(3000);
        ScrCls_Api();
    }
    else if (strcmp(type, "DOWN_QR") == 0) {
    	MAINLOG_L1("Download QR command received");
		ScrCls_Api();
		ScrDisp_Api(LINE5, 0, "Downloading QR...", CDISP);

		int ret = 0;
		// IMPLEMENT TMS CODE IN HERE
		ret = DownloadMerchantQR();
		if (ret == 0) {
			MAINLOG_L1("QR download succeeded");
			ScrCls_Api();
			ScrDisp_Api(LINE5, 0, "QR Download Success", CDISP);
            Delay_Api(1000);
            ScrBrush_Api();
            initQRImage();
			g_RefreshQRDisplay = 1;

		} else {
			MAINLOG_L1("QR download failed");
			ScrCls_Api();
			ScrDisp_Api(LINE5, 0, "QR Download Failed", CDISP);
		}
    }
    else if (strcmp(type, "DEL_MP3") == 0) {
        // Delete MP3
        MAINLOG_L1("Delete MP3 command received");
        fileFilter = 1;
        deleteAll();
        ScrCls_Api();
        ScrDisp_Api(LINE5, 0, "MP3 Deleted files", CDISP);
        Delay_Api(3000);
        ScrCls_Api();
    }
    else if (strcmp(type, "DOWN_MP3") == 0) {
        // Download MP3
        MAINLOG_L1("Download MP3 command received");
        download_mp3();
    }
    else if (strcmp(type, "DEL_SYS_PARAM") == 0) {
        // Delete System Parameters
        MAINLOG_L1("Delete system parameters command received");
        fileFilter = 5;
        deleteAll();
        ScrCls_Api();
        ScrDisp_Api(LINE5, 0, "DEL_SYS_PARAM", CDISP);
        Delay_Api(3000);
        ScrCls_Api();
    }
    else if (strcmp(type, "ST") == 0) {
        // Synchronize Time
        MAINLOG_L1("Synchronize time command received");
        cJSON *timeItem = cJSON_GetObjectItem(root, "time");
        if (timeItem) {
            struct DATE_USER date = {0};
            struct TIME_USER timeVal = {0};
            const char *timeStr = timeItem->valuestring;

            date.year = (short)AscToLong_Api(timeStr, 2);
            date.mon = (char)AscToLong_Api(timeStr + 2, 1);
            date.day = (char)AscToLong_Api(timeStr + 3, 1);
            timeVal.hour = (char)AscToLong_Api(timeStr + 4, 1);
            timeVal.min = (char)AscToLong_Api(timeStr + 5, 1);
            timeVal.sec = (char)AscToLong_Api(timeStr + 6, 1);

            SetTime_Api(&date, &timeVal);

            unsigned char bcdtime[64] = {0};
            GetSysTime_Api(bcdtime);
            CommPrintHex("system time", bcdtime, 8);
        }
    }
    else if (strcmp(type, "MP") == 0) {
        // Play MP3 Files
        MAINLOG_L1("Play MP3 command received");
        cJSON *fileItem = cJSON_GetObjectItem(root, "file");
        if (fileItem) {
            char *fileList = fileItem->valuestring;
            char *fileStart = fileList;
            char *plusPos = NULL;

            int count = 0;
            while (*fileStart && count < 10) {
                plusPos = strchr(fileStart, '+');

                char mp3File[32] = {0};
                int len = plusPos ? (plusPos - fileStart) : strlen(fileStart);
                if (len > sizeof(mp3File) - 1) {
                    len = sizeof(mp3File) - 1;
                }

                memcpy(mp3File, fileStart, len);
                mp3File[len] = '\0';

                MAINLOG_L1("Playing file %d: %s", count+1, mp3File);
                PlayMP3File(mp3File);
                count++;

                if (!plusPos) {
                    break;
                }
                fileStart = plusPos + 1;
            }
        }
    }
    else if (strcmp(type, "TS") == 0) {
        // Transaction Summary
        MAINLOG_L1("Transaction summary command received");
        cJSON *amountTS = cJSON_GetObjectItem(root, "amount");
        if (amountTS) {
            char tipMsg[64];
            snprintf(tipMsg, sizeof(tipMsg), "%s dollars received", amountTS->valuestring);
            AppPlayTip(tipMsg);

            int amount = 0;
            char asc[12] = {0};
            char *dotPos = strchr(amountTS->valuestring, '.');
            if (dotPos) {
                int integerLen = dotPos - amountTS->valuestring;
                if (integerLen > 0 && integerLen < (int)sizeof(asc)) {
                    memcpy(asc, amountTS->valuestring, integerLen);
                    asc[integerLen] = '\0';
                    strncat(asc, dotPos + 1, 2);
                    amount = atoi(asc);
                }
            } else {
                amount = atoi(amountTS->valuestring) * 100;
            }

            if (Device_Type >= 2) {
                // Additional processing for Device_Type >= 2
            }
        }
    }
    else {
        MAINLOG_L1("Unknown message type: %s", type);
    }

    cJSON_Delete(root);
}

void publishDeviceStatus(MQTTClient* client) {
    char statusMsg[256];
    sprintf(statusMsg,
            "{\"deviceId\":\"%s\",\"status\":\"online\",\"connection\":\"%s\",\"timestamp\":%ld}",
            G_sys_param.sn,
            _IS_WIFI_READY_ ? "wifi" : "gprs",
            time(NULL));

    MQTTMessage message;
    message.qos = 1;
    message.retained = 1;
    message.payload = statusMsg;
    message.payloadlen = strlen(statusMsg);

    MQTTPublish(client, "devices/status", &message);
    MAINLOG_L1("Published device status: %s", statusMsg);
}

void publishHeartbeat(MQTTClient* client, time_t sessionStartTime) {
    char heartbeatMsg[256];
    time_t currentTime = time(NULL);
    int ret, isElect;
    ret = BatChargeProc_Api(0, 0, 0, 0, BATER_WARM, &isElect);
	char tmp[8] = "";
	sprintf(tmp, "%d", ret);
	strcat(tmp, "%");


    sprintf(heartbeatMsg,
            "{\"deviceId\":\"%s\",\"timestamp\":%ld,\"uptime\":%ld,\"connection\":\"%s\",\"volume\":%d}",
            G_sys_param.sn,
            currentTime,
            currentTime - sessionStartTime,
            _IS_WIFI_READY_ ? "wifi" : "gprs",
            		tmp);

    MQTTMessage message;
    message.qos = 1;
    message.retained = 0;
    message.payload = heartbeatMsg;
    message.payloadlen = strlen(heartbeatMsg);

    MQTTPublish(client, "devices/heartbeat", &message);
    MAINLOG_L1("Published heartbeat");
}

// Modified mQTTMainThread function for robust socket management
void mQTTMainThread(void) {
    Network n;
    MQTTClient c;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    int ret, err;
    int connection_attempts = 0;
    int current_retry_delay = INITIAL_RECONNECT_DELAY;
    time_t lastHeartbeat = 0;
    time_t lastDeleteFile = 0;
    time_t sessionStartTime = 0;
    char clientID[64];
    char command_topic[64];

    while (1) {
        // Wait for internet connection
        if (!checkInternetStatus()) {
            MAINLOG_L1("No internet connection. Retrying in %d ms...", current_retry_delay);
            Delay_Api(current_retry_delay);

            // Implement progressive backoff for retries
            if (++connection_attempts > 3) {
                current_retry_delay = (current_retry_delay * 2 < MAX_RECONNECT_DELAY) ?
                                       current_retry_delay * 2 : MAX_RECONNECT_DELAY;
            }
            continue;
        }

        // Reset connection counters when internet is available
        connection_attempts = 0;
        current_retry_delay = INITIAL_RECONNECT_DELAY;

        // Initialize MQTT client
        memset(&c, 0, sizeof(MQTTClient));
        MQTTClientInit(&c, &n, 20000, pSendBuf, 1024, pReadBuf, 1024);

        // Set message handlers
        c.defaultMessageHandler = onDefaultMessageArrived;

        // Set network functions
        n.mqttconnect = net_connect;
        n.mqttclose = net_close;
        n.mqttread = net_read;
        n.mqttwrite = net_write;

        // Generate unique client ID
        sprintf(clientID, "%s", G_sys_param.sn);

        // Check if we can reuse existing socket
        if (g_mqtt_socket != NULL && g_mqtt_socket->valid &&
            strcmp(g_mqtt_socket->host, G_sys_param.mqtt_server) == 0 &&
            strcmp(g_mqtt_socket->port, G_sys_param.mqtt_port) == 0) {

            MAINLOG_L1("Reusing existing MQTT socket connection");
            n.netContext = g_mqtt_socket;
            err = 0;
        } else {
            // If existing socket is invalid or for different host, ensure it's closed
            if (g_mqtt_socket != NULL) {
                MAINLOG_L1("Closing existing invalid MQTT socket before creating new one");
                net_close(g_mqtt_socket);
                g_mqtt_socket = NULL;
            }

            // Connect to MQTT broker with fresh socket
            MAINLOG_L1("Creating new MQTT connection to %s:%s",
                      G_sys_param.mqtt_server, G_sys_param.mqtt_port);

            n.netContext = n.mqttconnect(NULL, G_sys_param.mqtt_server, G_sys_param.mqtt_port,
                                        10000, G_sys_param.mqtt_ssl, &err);
        }

        // Store in global reference if connection successful
        if (n.netContext != NULL && err == 0) {
            g_mqtt_socket = n.netContext;
            MAINLOG_L1("Stored new MQTT socket connection");
        } else {
            MAINLOG_L1("MQTT connection failed (err=%d), retrying in %d ms",
                       err, current_retry_delay);
            // Ensure any partial connection is properly closed
            if (n.netContext != NULL) {
                net_close(n.netContext);
                n.netContext = NULL;
                g_mqtt_socket = NULL;
            }
            Delay_Api(current_retry_delay);
            current_retry_delay = (current_retry_delay * 2 < MAX_RECONNECT_DELAY) ?
                                 current_retry_delay * 2 : MAX_RECONNECT_DELAY;
            continue;
        }

        // Configure MQTT connection data
        data.MQTTVersion = 4;
        data.clientID.cstring = G_sys_param.mqtt_client_id;
        data.keepAliveInterval = G_sys_param.mqtt_keepalive;
        data.cleansession = 1;  // Use persistent sessions

        // Connect to broker
        ret = MQTTConnect(&c, &data);
        if (ret != 0) {
            MAINLOG_L1("MQTT connect failed with error %d", ret);
            MQTTDisconnect(&c);

            // Clean disconnect and reset references
            net_close(n.netContext);
            n.netContext = NULL;
            g_mqtt_socket = NULL;

            Delay_Api(current_retry_delay);
            continue;
        }

        // Subscribe to topics
        ret = MQTTSubscribe(&c, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, onTopicMessageArrived);

        if (ret != 0) {
            MAINLOG_L1("MQTT subscribe failed with error %d", ret);
            isNetworkConnected = 0;
            MQTTDisconnect(&c);

            // Clean disconnect and reset references
            net_close(n.netContext);
            n.netContext = NULL;
            g_mqtt_socket = NULL;

            Delay_Api(current_retry_delay);
            continue;
        }

        // Publish online status
        publishDeviceStatus(&c);

        // Connected successfully
        MAINLOG_L1("MQTT connected successfully");
        isNetworkConnected = 1;
        sessionStartTime = time(NULL);
        lastHeartbeat = sessionStartTime;
        lastDeleteFile = sessionStartTime;
        PlayMP3File("IoT_Sucess_connect.mp3");

        // Reset reconnect flag
        mqtt_reconnect_flag = 0;

        // Main MQTT processing loop
        while (checkInternetStatus() && !mqtt_reconnect_flag) {
            // Process MQTT operations
            ret = MQTTYield(&c, 2000);

            // Send heartbeat
            time_t currentTime = time(NULL);
            if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
                publishHeartbeat(&c, sessionStartTime);
                lastHeartbeat = currentTime;
            }

            if (currentTime - lastDeleteFile >= DELETE_FILE) {
			   // CLEAR ALL SOUND WAS PLAYED EVERY 1MN
			   fileFilter = 0;
			   deleteAll();
			   lastDeleteFile = currentTime;
 		   }

            // Check for errors
            if (ret < 0) {
                MAINLOG_L1("MQTT yield failed with error %d", ret);
                break;
            }

            // Small delay to prevent tight loop
            Delay_Api(2000);
        }

        // Check why we exited the loop
        if (mqtt_reconnect_flag) {
            MAINLOG_L1("MQTT reconnect flag set, reconnecting");
        } else {
            MAINLOG_L1("Network disconnected or error occurred");
        }

        // Clean disconnect
        isNetworkConnected = 0;
        MQTTDisconnect(&c);

        // Properly close socket and reset references
        net_close(n.netContext);
        n.netContext = NULL;
        g_mqtt_socket = NULL;

        // Delay before reconnecting
        Delay_Api(1000);
    }
}
